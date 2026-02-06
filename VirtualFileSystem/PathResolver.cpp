#include "PathResolver.h"
#include "FileSystem.h"
#include "DirectoryManager.h"

std::unordered_map<DentryKey, Dentry*, DentryHasher> PathResolver::dcache;
std::list<DentryKey> PathResolver::lru_list;

PathResolver::PathResolver(std::string path, FileSystem* fs)
    : fs(fs), status(ResolverStatus::SUCCESS) {
    if (path.empty()) { status = ResolverStatus::EMPTY_PATH; return; }

    PathSplitList splitList(path);
    size_t startIndex = 0;

    // Detect Root vs Current Working Directory
    if (splitList[0] == "/" || splitList[0] == "\\") {
        start_node_dentry = fs->getRootDentry(); // Assumes fs returns Dentry*
        startIndex = 1;
    }
    else {
        start_node_dentry = fs->getCurrentDentry();
        startIndex = 0;
    }

    resolve(splitList, startIndex);
}

// Resolve the given path
void PathResolver::resolve(PathSplitList& splitList, size_t startIndex) {
    Dentry* current_parent = start_node_dentry;

    for (size_t i = startIndex; true; ++i) {
        std::string part = splitList[i];
        if (part.empty()) break;

        // Prevent escaping root
        if (part == "..") {
            Dentry* next_node = (current_parent->parent) ? current_parent->parent : current_parent;
            internal_components.push_back(next_node);
            next_node->pin();
            current_parent = next_node;
            continue;
        }

        if (current_parent->type == NodeType::FILE) {
            status = ResolverStatus::BLOCKED_BY_FILE;
            break;
        }

        DentryKey key = { current_parent->inode, part };
        Dentry* next_node = nullptr;

        // Check cache with DNA Test
        if (dcache.count(key)) {
            Dentry* cached = dcache[key];
            // DNA Test: Check if the cached dentry's parent object is exactly our current_parent
            if (cached->parent == current_parent) {
                next_node = cached;
            }
        }

        if (!next_node) {
            inodeID tid = DirectoryManager(current_parent->inode, fs).findInode(part);
            if (tid == NULL_INODE) {
                status = ResolverStatus::NOT_FOUND;
                // Create transient ghost dentry for the missing component
                next_node = new Dentry(part, NULL_INODE, current_parent, NodeType::UNKNOWN);
                internal_components.push_back(next_node);
                next_node->pin();
                break;
            }
            NodeType realType = fs->get_node_type(tid);
            next_node = getOrCreateDentry(current_parent->inode, part, tid, current_parent, realType);
        }

        internal_components.push_back(next_node);
        next_node->pin();
        current_parent = next_node;
    }
}

Dentry* PathResolver::getOrCreateDentry(inodeID pID, const std::string& name, inodeID tID, Dentry* pPtr, NodeType type) {
    DentryKey key = { pID, name };
    if (dcache.count(key)) {
        lru_list.remove(key);
        lru_list.push_front(key);
        return dcache[key];
    }

    pruneCache();
    Dentry* newNode = new Dentry(name, tID, pPtr, type);
    dcache[key] = newNode;
    lru_list.push_front(key);
    return newNode;
}

void PathResolver::pruneCache() {
    if (dcache.size() < MAX_CACHE_SIZE) return;

    auto it = lru_list.end();
    while (it != lru_list.begin() && dcache.size() >= MAX_CACHE_SIZE) {
        --it;
        Dentry* node = dcache[*it];

        // Only delete if it's not pinned AND has no children in cache
        if (node->pin_count == 0 && node->child_in_cache_count == 0) {
            delete node;
            dcache.erase(*it);
            it = lru_list.erase(it);
        }
    }
}

// cleanup
PathResolver::~PathResolver() {
    for (auto d : internal_components) {
        // If it's a ghost dentry, delete it; otherwise just unpin it
        if (d->inode == NULL_INODE) delete d;
        else d->unPin();
    }
}

// Getters
Dentry* PathResolver::get_target_dentry() const {
    return internal_components.empty() ? start_node_dentry : internal_components.back();
}

PathComponent PathResolver::get_target() const {
    Dentry* t = get_target_dentry();
    return { t->name, t->inode };
}

PathComponent PathResolver::get_parent() const {
    Dentry* t = get_target_dentry();
    if (t->parent) return { t->parent->name, t->parent->inode };
    return { t->name, t->inode };
}

NodeType PathResolver::get_target_type() const { return get_target_dentry()->type; }

void PathResolver::pinPath(Dentry* node) {
    while (node) { node->pin(); node = node->parent; }
}

void PathResolver::unpinPath(Dentry* node) {
    while (node) { node->unPin(); node = node->parent; }
}

// Returns the full string path of the cwd
std::string PathResolver::getCurrentPath(FileSystem* fs) {
    Dentry* current = fs->getCurrentDentry();
    if (!current) return "/";

    // If we are at root
    if (current->parent == nullptr) return "/";

    std::vector<std::string> path_parts;
    Dentry* temp = current;

    // Traverse upwards until we hit the root (where parent is nullptr)
    while (temp != nullptr && temp->parent != nullptr) {
        path_parts.push_back(temp->name);
        temp = temp->parent;
    }

    // Build the path string from the collected parts
    std::string full_path = "";
    for (int i = path_parts.size() - 1; i >= 0; --i) {
        full_path += "/" + path_parts[i];
    }

    return full_path.empty() ? "/" : full_path;
}


// =================== Syncers ====================

// Removes a dentry from cache
void PathResolver::syncRemove(inodeID pId, std::string name) {
    DentryKey key = { pId, name };
    auto it = dcache.find(key);

    if (it != dcache.end()) {
        Dentry* node = it->second;

        // Remove from search map immediately so no new resolvers can find it
        dcache.erase(it);
        lru_list.remove(key);

        // If no one is using it and it has no children, free memory
        if (node->pin_count == 0 && node->child_in_cache_count == 0) {
            delete node; // Destructor will update parent's child_in_cache_count
        }
        else {
            node->inode = NULL_INODE;
            node->parent = nullptr; // Detach from parent to allow parent to be pruned
        }
    }
}

// Updates a dentry's location and parent linkage after a rename/move
void PathResolver::syncMove(inodeID pId1, std::string name1, inodeID pId2, std::string name2) {
    DentryKey oldKey = { pId1, name1 };
    auto it = dcache.find(oldKey);

    if (it != dcache.end()) {
        Dentry* node = it->second;

        // Update child counts on old and new parents
        if (node->parent) node->parent->child_in_cache_count--;

        // Find the new parent object in cache
        Dentry* newParentPtr = nullptr;
        for (auto const& [key, val] : dcache) {
            if (val->inode == pId2) {
                newParentPtr = val;
                break;
            }
        }

        node->parent = newParentPtr;
        node->name = name2;
        if (newParentPtr) newParentPtr->child_in_cache_count++;

        // Update the cache maps with the new key
        dcache.erase(it);
        lru_list.remove(oldKey);

        DentryKey newKey = { pId2, name2 };
        dcache[newKey] = node;
        lru_list.push_front(newKey);
    }
}

// Converts a "Ghost" dentry into a real one after file creation
void PathResolver::syncMakeNode(inodeID pId, std::string name, inodeID tId, NodeType type) {
    DentryKey key = { pId, name };
    auto it = dcache.find(key);

    if (it != dcache.end()) {
        Dentry* node = it->second;
        // If it was a Ghost (NULL_INODE), now it gets its "soul" (real Inode ID)
        if (node->inode == NULL_INODE) {
            node->inode = tId;
            node->type = type; // Update type from UNKNOWN to real type
        }
    }
}