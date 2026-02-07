#pragma once

#define DEFAULT_ROOT_INODE_ID 0
#define DEFAULT_CACHE_SIZE 4096 

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "types.h"
#include "PathResolver.h"
#include "InodeManager.h"
#include "FileDescriptor.h"
#include "PointerMapManager.h"
#include "DirectoryManager.h"

// Forward declarations to reduce header coupling
class BufferCache;
class InodePageManager;
class IndirectBlockManager;
class PointerMapManager;

/**
 * @class FileSystem
 * @brief The central orchestrator of the file system logic.
 * Connects high-level path resolution with low-level disk management.
 */
class FileSystem {
private:
    // --- System Resources (Owned by FileSystem) ---
    BufferCache* cache = nullptr;
    InodePageManager* page_mgr = nullptr;
    IndirectBlockManager* indirect_mgr = nullptr;
    PointerMapManager* pointer_map = nullptr;

    // --- State Tracking ---
    Dentry* root_dentry; // Permanently pinned
    Dentry* cwd_dentry;  // Always pinned; updated on 'cd'

    // --- Open File Registry ---
    // Maps FD integers to their respective FileDescriptor objects
    std::unordered_map<int, std::unique_ptr<FileDescriptor>> fd_table;
    int next_fd_counter = 3; // Starts at 3 (Standard POSIX behavior)

    // --- Private Utilities ---
    int assign_fd(std::unique_ptr<FileDescriptor> fd);
    void setup_root(); // Initializes or loads the root directory on boot
    
    inline bool transfer_ownership(PathComponent& oldParent, PathComponent& newParent, PathComponent& oldChill, string& new_name);
    // make new directory 
    bool dirGenerator(const std::string& path, inodeFlags permissions);

    bool remove_Directory(inodeID target , string& name); 
    bool removeEmptyDir(inodeID target, string& name); 
    bool remove_File(inodeID target , string& name);
    size_t getSize(PathComponent com); 
public:
    // ==================== ## Lifecycle & Initialization ====================

    // Injects BufferCache to allow flexible storage backends (Mock, Disk, or RAM)
    FileSystem(BlockDevice * device);
    ~FileSystem();

    // ==================== ## Path & Navigation ====================

    // Changes current working directory (CWD) and manages Dentry pinning
    bool cd(const std::string& path);

    // Reconstructs and returns the absolute path string of the CWD
    std::string get_current_path();

    // ==================== ## Directory Operations ====================
    
    // make new directory 
    bool mkdir( std::string& path, inodeFlags permissions);
    // make some directories 
    bool mkdirs( std::string& path, inodeFlags premisions); 
    // remove directory if it is empty
    bool rmdir( std::string& path);  
    // remove all directory 
    bool rmall( std::string& path); 
    // make hard link of src to dst 
    bool mklink(const std::string& src_path, const std::string dst_path);
    // Returns a list of file/directory names within the specified path
    std::vector<std::string> ls(const std::string& path = ".");

    // ==================== ## File Lifecycle ====================

    // Creates an empty file at the given path
    bool touch(const std::string& path, inodeFlags permissions);

    // Removes a file link from the directory (deletes if link count reaches 0)
    bool unlink(const std::string& path);

    bool rename(const std::string& old_path, const std::string& new_path);
    bool move(const std::string& old_path, const std::string& new_path);
    bool copy(const std::string& dst, const std::string& src); 
    

    // ==================== ## I/O & File Descriptor Management ====================

    // Resolves path, creates a FileDescriptor, and returns a unique integer ID (FD)
    int open(const std::string& path, int flags);

    // Removes the FD from the table and triggers cleanup
    bool close(int fd);

    // Provides access to the underlying FD object for FileHandle wrapper
    FileDescriptor* get_fd_object(int fd);

    // ==================== ## Metadata Queries ====================

    bool exists(const std::string& path);
    uint64 get_size(const std::string& path);
    bool is_dir(const std::string& path);
    bool set_perms(const std::string& path, inodeFlags perms);
	NodeType get_node_type(const std::string& path);
    // ==================== ## Core Component Getters ====================

    BufferCache* getBufferCache() const { return cache; }
    InodePageManager* getInodePageManager() const { return page_mgr; }
    IndirectBlockManager* getIndirectBlockManager() const { return indirect_mgr; }
    PointerMapManager* getPointerMapManager() const { return pointer_map; }

    // Returns root and current dentry (usually for PathResolver or debugging)
    Dentry* getRootDentry() const { return root_dentry; }
    Dentry* getCurrentDentry() const { return cwd_dentry; }
};