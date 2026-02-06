#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include "PathSplitList.h"
#include "types.h"

class FileSystem;

enum class NodeType { DIRECTORY, FILE, UNKNOWN };
enum class ResolverStatus { SUCCESS, NOT_FOUND, BLOCKED_BY_FILE, EMPTY_PATH };

struct Dentry {
    std::string name;
    inodeID inode;
    Dentry* parent;
    NodeType type;
    int pin_count;
    int child_in_cache_count; // how many children exist in dcache

    Dentry(std::string n, inodeID i, Dentry* p, NodeType t)
        : name(n), inode(i), parent(p), type(t), pin_count(0), child_in_cache_count(0) {
        if (parent) parent->child_in_cache_count++;
    }

    ~Dentry() {
        // when deleted, notify parent that one cached-child removed
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
    NodeType get_target_type() const;
    PathComponent get_target() const;
    PathComponent get_parent() const;
    Dentry* get_target_dentry() const;

    static void pinPath(Dentry* node);
    static void unpinPath(Dentry* node);

private:
    void resolve(PathSplitList& splitList, size_t startIndex);
    void pruneCache();
    Dentry* getOrCreateDentry(inodeID pID, const std::string& name, inodeID tID, Dentry* pPtr, NodeType type);

    FileSystem* fs;
    std::vector<Dentry*> internal_components;
    Dentry* start_node_dentry;
    ResolverStatus status;

    // get the full path via cwd
    static std::string getCurrentPath(FileSystem* fs);

    // Data Center
    static const size_t MAX_CACHE_SIZE = 1000;
    static std::unordered_map<DentryKey, Dentry*, DentryHasher> dcache;
    static std::list<DentryKey> lru_list;

    // Syncers
    static void syncRemove(inodeID pId, std::string name);
    static void syncMove(inodeID pId1, std::string name1, inodeID pId2, std::string name2);
    static void syncMakeNode(inodeID pId, std::string name, inodeID tId, NodeType type);
};