#include "PathResolver.h"
#include "FileSystem.h"
#include "DirectoryManager.h"

std::unordered_map<DentryKey, Dentry*, DentryHasher> PathResolver::dcache;
std::list<DentryKey> PathResolver::lru_list;

PathResolver::PathResolver(std::string path, FileSystem* fs)
    : fs(fs), status(ResolverStatus::SUCCESS), ghost_stored_in_cache(false) {

    if (path.empty()) {
        status = ResolverStatus::EMPTY_PATH;
        return;
    }

    PathSplitList splitList(path);
    Dentry* cwd = fs->getCurrentDentry();
    size_t startIndex = 0;

    if (splitList[0] == "/" || splitList[0] == "\\") {
        start_node_dentry = fs->getRootDentry();
        startIndex = 1;
    }
    else {
        start_node_dentry = cwd;
        startIndex = 0;
    }

    resolve(splitList, startIndex);
}

void PathResolver::resolve(PathSplitList& splitList, size_t startIndex) {
    Dentry* current_parent = start_node_dentry;

    for (size_t i = startIndex; true; ++i) {
        std::string part = splitList[i];
        if (part.empty()) break;

        inodeID targetID = NULL_INODE;
        Dentry* next_node = nullptr;

        if (part == "..") {
            next_node = (current_parent->parent != nullptr) ? current_parent->parent : current_parent;
        }
        else {
            if (current_parent->inode != NULL_INODE) {
                DentryKey key = { current_parent->inode, part };
                if (!dcache.count(key))
                    targetID = DirectoryManager(current_parent->inode, fs).findInode(part);
                else {
                    next_node = dcache[key];
                    targetID = next_node->inode;
                }
            }

            if (targetID != NULL_INODE && next_node == nullptr) {
                next_node = getOrCreateDentry(current_parent->inode, part, targetID, current_parent);
            }
            else if (targetID == NULL_INODE) {
                status = ResolverStatus::NOT_FOUND;
                if (!ghost_stored_in_cache) {
                    next_node = getOrCreateDentry(current_parent->inode, part, NULL_INODE, current_parent); // here next node will add to dcache
                    ghost_stored_in_cache = true;
                }
                else {
                    next_node = new Dentry(part, NULL_INODE, current_parent);
                }
            }
        }

        internal_components.push_back(next_node);
        next_node->pin();
        current_parent = next_node;
    }
}

Dentry* PathResolver::getOrCreateDentry(inodeID pID, const std::string& name, inodeID tID, Dentry* pPtr) {
    DentryKey key = { pID, name };

    if (dcache.count(key)) {
        Dentry* cached = dcache[key];
        if (cached->inode != tID) {
            cached->inode = tID;
        }
        lru_list.remove(key);
        lru_list.push_front(key);
        return cached;
    }

    pruneCache();
    Dentry* newNode = new Dentry(name, tID, pPtr);
    dcache[key] = newNode;
    lru_list.push_front(key);
    return newNode;
}

std::vector<PathComponent> PathResolver::get_components() const {
    std::vector<PathComponent> result;
    result.push_back({ start_node_dentry->name, start_node_dentry->inode });
    for (Dentry* d : internal_components) {
        result.push_back({ d->name, d->inode });
    }
    return result;
}

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

void PathResolver::pruneCache() {
    if (dcache.size() < MAX_CACHE_SIZE) return;
    auto it = lru_list.end();
    while (it != lru_list.begin() && dcache.size() >= MAX_CACHE_SIZE) {
        --it;
        DentryKey key = *it;
        if (dcache[key]->pin_count == 0) {
            delete dcache[key];
            dcache.erase(key);
            it = lru_list.erase(it);
        }
    }
}

PathResolver::~PathResolver() {
    for (Dentry* d : internal_components) {
        d->unPin();
        DentryKey key = { (d->parent ? d->parent->inode : 0), d->name };
        if (dcache.find(key) == dcache.end() || dcache[key] != d) {
            delete d;
        }
    }
}