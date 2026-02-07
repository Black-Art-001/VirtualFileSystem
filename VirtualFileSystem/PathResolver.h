#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include "PathSplitList.h"
#include "types.h"

class FileSystem;
class InodeManager;

enum class NodeType { DIRECTORY, FILE, UNKNOWN };
enum class ResolverStatus { SUCCESS, NOT_FOUND, BLOCKED_BY_FILE, EMPTY_PATH };

struct Dentry {
    std::string name;
    inodeID inode;
    Dentry* parent;
    NodeType type;
    int pin_count;
    int child_in_cache_count;

    Dentry(std::string n, inodeID i, Dentry* p, NodeType t)
        : name(n), inode(i), parent(p), type(t), pin_count(0), child_in_cache_count(0) {
        if (parent) parent->child_in_cache_count++;
    }

    ~Dentry() {
        if (parent) parent->child_in_cache_count--;
    }

    void pin() { pin_count++; }
    void unPin() { if (pin_count > 0) pin_count--; }
};

struct DentryKey {
    inodeID parentID;
    std::string name;
    bool operator==(const DentryKey& other) const {
        return parentID == other.parentID && name == other.name;
    }
};

struct DentryHasher {
    std::size_t operator()(const DentryKey& k) const {
        return std::hash<inodeID>{}(k.parentID) ^ (std::hash<std::string>{}(k.name) << 1);
    }
};

struct PathComponent {
    std::string name;
    inodeID id;
};

class PathResolver {
public:
    PathResolver(std::string path, FileSystem* fs);
    ~PathResolver();

    ResolverStatus getStatus() const { return status; }

    // Getters that you needed
    NodeType get_target_type() const { return get_target_dentry()->type; }
    NodeType get_parent_type() const {
        return (get_target_dentry()->parent) ? get_target_dentry()->parent->type : NodeType::UNKNOWN;
    }

    PathComponent get_target() const;
    PathComponent get_parent() const;
    Dentry* get_target_dentry() const;

	static void pinDentry(Dentry* node) { if (node) node->pin(); }
	static void unpinDentry(Dentry* node) { if (node) node->unPin(); }
    static std::string getCurrentPath(FileSystem* fs);

    // Syncers
    static void syncRemove(inodeID pId, std::string name);
    static void syncMove(inodeID pId1, std::string name1, inodeID pId2, std::string name2);
    static void syncMakeNode(inodeID pId, std::string name, inodeID tId, NodeType type);

private:
    void resolve(PathSplitList& splitList, size_t startIndex);
    void pruneCache();
    Dentry* getOrCreateDentry(inodeID pID, const std::string& name, inodeID tID, Dentry* pPtr, NodeType type);
	NodeType getNodeType(inodeID id) const;

    FileSystem* fs;
    std::vector<Dentry*> internal_components;
    Dentry* start_node_dentry;
    ResolverStatus status;

    static const size_t MAX_CACHE_SIZE = 4096;
    static std::unordered_map<DentryKey, Dentry*, DentryHasher> dcache;
    static std::list<DentryKey> lru_list;
};