#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include "PathSplitList.h"
#include "types.h"

class FileSystem;

enum class ResolverStatus {
    SUCCESS,
    NOT_FOUND,
    INVALID_START_NODE,
    EMPTY_PATH,
    FLOATING_PATH
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

class PathResolver {
public:
    PathResolver(std::string path, FileSystem* fs);
    ~PathResolver();

    bool exists() const { return status == ResolverStatus::SUCCESS; }
    ResolverStatus getStatus() const { return status; }

    PathComponent get_target() const;
    PathComponent get_parent() const;
    Dentry* get_target_dentry() const;
    std::vector<PathComponent> get_components() const;

    static void pinPath(Dentry* node);
    static void unpinPath(Dentry* node);

private:
    void resolve(PathSplitList& splitList, size_t startIndex);
    static void pruneCache();
    Dentry* getOrCreateDentry(inodeID pID, const std::string& name, inodeID tID, Dentry* pPtr);

    FileSystem* fs;
    std::vector<Dentry*> internal_components;
    Dentry* start_node_dentry;
    ResolverStatus status;
    bool ghost_stored_in_cache;

    static const size_t MAX_CACHE_SIZE = 1000;
    static std::unordered_map<DentryKey, Dentry*, DentryHasher> dcache;
    static std::list<DentryKey> lru_list;
};