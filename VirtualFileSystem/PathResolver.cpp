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

    if (splitList[0] == "/" || splitList[0] == "\\") {
        start_node_dentry = fs->getRootDentry();
        startIndex = 1;
    }
    else {
        start_node_dentry = fs->getCurrentDentry();
        startIndex = 0;
    }

    resolve(splitList, startIndex);
}

void PathResolver::resolve(PathSplitList& splitList, size_t startIndex) {
    Dentry* current_parent = start_node_dentry;

    for (size_t i = startIndex; i < splitList.size(); ++i) {
        std::string part = splitList[i];

        // 1. Navigation Upwards
        if (part == "..") {
            if (current_parent->parent != nullptr) {
                current_parent = current_parent->parent;

                internal_components.push_back(current_parent);
                current_parent->pin();
            }
            continue;
        }

        if (current_parent->type == NodeType::FILE) {
            status = ResolverStatus::BLOCKED_BY_FILE;
            break;
        }

        DentryKey key = { current_parent->inode, part };
        Dentry* next_node = nullptr;

        auto it = dcache.find(key);
        if (it != dcache.end()) {
            next_node = it->second;
        }

        if (!next_node) {
            inodeID tid = DirectoryManager(current_parent->inode, fs).findInode(part);

            if (tid == NULL_INODE) {
                status = ResolverStatus::NOT_FOUND;
                next_node = new Dentry(part, NULL_INODE, current_parent, NodeType::UNKNOWN);
                dcache[key] = next_node;
                lru_list.push_front(key);

                internal_components.push_back(next_node);
                next_node->pin();
                break;
            }

            NodeType realType = NodeType::DIRECTORY; // = fs->get_node_type(tid);

            next_node = getOrCreateDentry(current_parent->inode, part, tid, current_parent, realType);
        }

        internal_components.push_back(next_node);
        next_node->pin();
        current_parent = next_node;
    }
}

Dentry* PathResolver::getOrCreateDentry(inodeID pID, const std::string& name, inodeID tID, Dentry* pPtr, NodeType type) {
    DentryKey key = { pID, name };
    auto it = dcache.find(key);
    if (it != dcache.end()) {
        lru_list.remove(key);
        lru_list.push_front(key);
        return it->second;
    }

    pruneCache();
    Dentry* newNode = new Dentry(name, tID, pPtr, type);
    dcache[key] = newNode;
    lru_list.push_front(key);
    return newNode;
}

NodeType PathResolver::getNodeType(inodeID id) const
{
	InodeManager target(fs, id);
	return (target.getType() == inodeType::DireMode) ? NodeType::DIRECTORY : NodeType::FILE;
}

void PathResolver::pruneCache() {
    if (dcache.size() < MAX_CACHE_SIZE) return;
    auto it = lru_list.end();
    while (it != lru_list.begin() && dcache.size() >= MAX_CACHE_SIZE) {
        --it;
        Dentry* node = dcache[*it];
        // Prune only leaves that aren't pinned
        if (node->pin_count == 0 && node->child_in_cache_count == 0) {
            delete node;
            dcache.erase(*it);
            it = lru_list.erase(it);
        }
    }
}

PathResolver::~PathResolver() {
    for (auto d : internal_components) d->unPin();
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

std::string PathResolver::getCurrentPath(FileSystem* fs) {
    Dentry* current = fs->getCurrentDentry();
    if (!current || !current->parent) return "/";
    std::vector<std::string> parts;
    while (current && current->parent) {
        parts.push_back(current->name);
        current = current->parent;
    }
    std::string full = "";
    for (int i = parts.size() - 1; i >= 0; --i) full += "/" + parts[i];
    return full.empty() ? "/" : full;
}

// =================== Syncers ====================

void PathResolver::syncRemove(inodeID pId, std::string name) {
    DentryKey key = { pId, name };
    auto it = dcache.find(key);

    if (it != dcache.end()) {
        Dentry* node = it->second;

        dcache.erase(it);
        lru_list.remove(key);

        if (node->pin_count == 0 && node->child_in_cache_count == 0) {
            delete node;
        }
        else {
            node->inode = NULL_INODE;
            node->parent = nullptr;
        }
    }
}

void PathResolver::syncMove(inodeID pId1, std::string name1, inodeID pId2, std::string name2) {
    DentryKey sourceKey = { pId1, name1 };
    DentryKey targetParentKey = { pId2, name2 };

    auto itSource = dcache.find(sourceKey);
    auto itTargetParent = dcache.find(targetParentKey);

    if (itSource == dcache.end()) return;

    Dentry* sourceNode = itSource->second;
    Dentry* newParentNode = (itTargetParent != dcache.end()) ? itTargetParent->second : nullptr;

    if (sourceNode->parent) {
        sourceNode->parent->child_in_cache_count--;
    }

    sourceNode->parent = newParentNode;
    if (newParentNode) {
        newParentNode->child_in_cache_count++;
    }

    dcache.erase(itSource);
    lru_list.remove(sourceKey);

    inodeID newParentInode = (newParentNode) ? newParentNode->inode : 0;
    DentryKey newKey = { newParentInode, sourceNode->name };

    dcache[newKey] = sourceNode;
    lru_list.push_front(newKey);
}

void PathResolver::syncMakeNode(inodeID pId, std::string name, inodeID tId, NodeType type) {
    DentryKey key = { pId, name };
    auto it = dcache.find(key);

    if (it != dcache.end()) {
        Dentry* node = it->second;

        if (node->inode == NULL_INODE) {
            node->inode = tId;
            node->type = type;
        }
    }
}

void PathResolver::initCache()
{
    if (dcache.bucket_count() < 5000) {
        dcache.reserve(5000);
    }
}
