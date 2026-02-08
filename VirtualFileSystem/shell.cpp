#include <algorithm>
#include <sstream>
#include <fstream>
#include <iterator>
#include "Shell.h"
#include "types.h"
#include "FileHandle.h"
#include <optional>

Shell::Shell(FileSystem* fs) : fs(fs), currentPath("/"), running(true) {
    initCommands();
}

void Shell::initCommands() {
    commands["pwd"] = [this](auto& args) { cmd_pwd(args); };
    commands["cd"] = [this](auto& args) { cmd_cd(args); };
    commands["ls"] = [this](auto& args) { cmd_ls(args); };
    commands["mkdir"] = [this](auto& args) { cmd_mkdir(args); };
    commands["rm"] = [this](auto& args) { cmd_rm(args); };
    commands["cp"] = [this](auto& args) { cmd_cp(args); };
    commands["mv"] = [this](auto& args) { cmd_mv(args); };
    commands["put"] = [this](auto& args) { cmd_put(args); };
    commands["get"] = [this](auto& args) { cmd_get(args); };
    commands["exit"] = [this](auto& args) { cmd_exit(args); };
    commands["theme"] = [this](auto& args) { cmd_theme(args); };
}

void Shell::run() {
    std::string line;
    while (running) {
        std::cout << "VFS:" << currentPath << "> ";
        if (!std::getline(std::cin, line) || line == "exit") break;

        auto args = tokenize(line);
        if (args.empty()) continue;
        size_t startIndex = 0;
        if (args[0] == ROOT_ACCESS) startIndex = 1;
        if (commands.count(args[startIndex]))
            commands[args[0]](args);
        else {
            print("Unkown command: ", USER_ERROR); print(args[0], SPESIAL);
            std::cout << std::endl;
        }
    }
}

// seprating words based on espace
std::vector<std::string> Shell::tokenize(std::string line) {
    std::stringstream ss(line);
    std::istream_iterator<std::string> begin(ss), end;
    return std::vector<std::string>(begin, end);
}

void Shell::cmd_pwd(const std::vector<std::string>& args) {
    print("PATH :", NORMAL); std::cout << std::endl; print("-----", NORMAL); std::cout << std::endl;
}

void Shell::cmd_cd(const std::vector<std::string>& args) {
    size_t startIndex = 0;
    bool rootAccess = false;

    if (!args.empty() && args[0] == ROOT_ACCESS) {
        startIndex = 1;
        rootAccess = true;
    }

    if (args.size() < startIndex + 2) {
        print("Usage: cd <directory_path>", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    std::string targetPath = args[startIndex + 1];

    if (!fs->exists(targetPath)) {
        print("Directory does not exist: " + targetPath, USER_ERROR);
        std::cout << std::endl;
        return;
    }

    if (!fs->is_dir(targetPath)) {
        print("Path is not a directory: " + targetPath, USER_ERROR);
        std::cout << std::endl;
        return;
    }

    if (fs->cd(targetPath)) {
        print("Current directory changed to: " + fs->get_current_path(), SUCCESS);
        currentPath = fs->get_current_path();
        std::cout << std::endl;
    }
    else {
        print("Failed to change directory. Permission denied or internal error.", SYSTEM_ERROR);
        std::cout << std::endl;
    }
}

void Shell::cmd_ls(const std::vector<std::string>& args) {
    std::string path = (args.size() > 1) ? args[1] : currentPath;
    auto entries = fs->ls(path);
    std::sort(entries.begin(), entries.end());
    std::string msg = (path != currentPath) ? "Enteries in :\n---------\n\n"+path : "Enteries here :\n---------\n\n";
    print(msg, NORMAL);
    for (auto entry : entries) {
        print(entry, SPESIAL); std::cout << std::endl;
    }
}

void Shell::cmd_mkdir(const std::vector<std::string>& args) {
    size_t startIndex = 0;
    bool rootAccess = false;

    // Handle root access flag
    if (!args.empty() && args[0] == ROOT_ACCESS) {
        startIndex = 1;
        rootAccess = true;
    }

    // Validation: mkdir <path> [flags]
    if (args.size() < startIndex + 2) {
        print("Usage: mkdir <path> [permissions_flag]", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    std::string path = args[startIndex + 1];
    std::string flagStr = (args.size() > startIndex + 2) ? args[startIndex + 2] : "";

    // Check if path already exists
    if (fs->exists(path)) {
        print("The given path already exists: " + path, USER_ERROR);
        std::cout << std::endl;
        return;
    }

    // Updated: Using reference-based parsing (handles empty string internally)
    inodeFlags permissions;
    if (!parseToInodeFlags(flagStr, permissions)) {
        print("Invalid flag: " + flagStr, USER_ERROR);
        std::cout << std::endl;
        return;
    }

    // Call the file system to create the directory
    if (fs->mkdir(path, permissions)) {
        print("Directory created successfully: " + path, SUCCESS);
        std::cout << std::endl;
    }
    else {
        print("Failed to create directory. Check parent path or permissions.", SYSTEM_ERROR);
        std::cout << std::endl;
    }
}

void Shell::cmd_rm(const std::vector<std::string>& args) {
    size_t startIndex = 0;
    bool rootAccess = false;

    // Handle root access flag
    if (!args.empty() && args[0] == ROOT_ACCESS) {
        startIndex = 1;
        rootAccess = true;
    }

    // Validation: rm <path>
    if (args.size() < startIndex + 2) {
        print("Usage: rm <path>", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    std::string path = args[startIndex + 1];

    // Check if path exists
    if (!fs->exists(path)) {
        print("The given path does not exist: " + path, USER_ERROR);
        std::cout << std::endl;
        return;
    }

    bool success = false;

    // Determine if it is a directory or a file
    if (fs->is_dir(path)) {
        // Use rmall for recursive directory deletion as per project requirements
        success = fs->rmall(path);
    }
    else {
        // Use unlink for regular files
        success = fs->unlink(path);
    }

    // Handle results
    if (success) {
        print("Successfully removed: " + path, SUCCESS);
        std::cout << std::endl;
    }
    else {
        print("Failed to remove: " + path + ". Check permissions or if the directory is locked.", SYSTEM_ERROR);
        std::cout << std::endl;
    }
}

void Shell::cmd_cp(const std::vector<std::string>& args) {
    size_t startIndex = 0;
    bool rootAccess = false;

    // Handle root access flag
    if (!args.empty() && args[0] == ROOT_ACCESS) {
        startIndex = 1;
        rootAccess = true;
    }

    // Validation: cp <source> <destination>
    if (args.size() < startIndex + 3) {
        print("Usage: cp <source_path> <destination_path>", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    std::string srcPath = args[startIndex + 1];
    std::string dstPath = args[startIndex + 2];

    // 1. Check if source exists
    if (!fs->exists(srcPath)) {
        print("Source path does not exist: " + srcPath, USER_ERROR);
        std::cout << std::endl;
        return;
    }

    // 2. Perform the copy operation
    // Note: Using the signature from your FileSystem.h: copy(destination, source)
    if (fs->copy(dstPath, srcPath)) {
        print("Successfully copied " + srcPath + " to " + dstPath, SUCCESS);
        std::cout << std::endl;
    }
    else {
        print("Failed to copy. Destination might be invalid or disk is full.", SYSTEM_ERROR);
        std::cout << std::endl;
    }
}

void Shell::cmd_mv(const std::vector<std::string>& args) {
    size_t startIndex = 0;
    bool rootAccess = false;

    // Handle root access flag
    if (!args.empty() && args[0] == ROOT_ACCESS) {
        startIndex = 1;
        rootAccess = true;
    }

    // Validation: mv <source> <destination>
    if (args.size() < startIndex + 3) {
        print("Usage: mv <source_path> <destination_path>", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    std::string srcPath = args[startIndex + 1];
    std::string dstPath = args[startIndex + 2];

    // 1. Check if source exists
    if (!fs->exists(srcPath)) {
        print("Source path does not exist: " + srcPath, USER_ERROR);
        std::cout << std::endl;
        return;
    }

    // 2. Perform the move operation
    // Using your FileSystem.h signature: move(old_path, new_path)
    if (fs->move(srcPath, dstPath)) {
        print("Successfully moved " + srcPath + " to " + dstPath, SUCCESS);
        std::cout << std::endl;
    }
    else {
        print("Failed to move. Ensure destination path is valid and permissions are correct.", SYSTEM_ERROR);
        std::cout << std::endl;
    }
}

void Shell::cmd_put(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        print("Usage: put <real_file_path> <vfs_directory_path>", USER_ERROR); std::cout << std::endl;
        return;
    }

    size_t startIndex = 0;
    bool rootAccess = false;

    if (args[0] == ROOT_ACCESS) {
        startIndex = 1;
        rootAccess = true;
    }

    std::string realPath = args[1 + startIndex];
    std::string vfsPath = args[2 + startIndex];
    std::string flag = (args.size() > 3 + startIndex) ? args[3 + startIndex] : "";

    if (!fs->exists(vfsPath)) {
        print("The Given Directory Path Not Exist", USER_ERROR);
        std::cout << std::endl;
        return;
    }
    if (!fs->is_dir(vfsPath)) {
        print("The Given Directory Is Not Directory", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    std::ifstream file(realPath, std::ios::binary | std::ios::ate);
    if (!file) {
        print("Failed to open file!\n", SYSTEM_ERROR);
        return;
    }

    std::streamsize size = file.tellg();
    if (size <= 0) {
        print("File is empty or error occurred!\n", SYSTEM_ERROR);
        return;
    }

    file.seekg(0, std::ios::beg);

    byte* buffer = new byte[size];

    if (!file.read(reinterpret_cast<char*>(buffer), size)) {
        print("Error while reading file!\n", SYSTEM_ERROR);
        delete[] buffer;
        return;
    }

    // Updated: Using reference-based parsing
    inodeFlags permission;
    if (!parseToInodeFlags(flag, permission)) {
        print("Given Flag is invalid: " + flag + "\n", USER_ERROR);
        delete[] buffer;
        return;
    }

    if (!fs->touch(vfsPath, permission)) {
        print("An Error hapend with making new file in vfs for the given file! in this path: " + vfsPath + "\n", SYSTEM_ERROR);
        delete[] buffer;
        return;
    }
    try {
        FileHandle fh(vfsPath, inodeFlags::OwnerWrite, rootAccess);
        fh.write(buffer, static_cast<size_t>(size));
        delete[] buffer; // free memory
        print("File Uploded to VFS succesfully", SUCCESS);
        std::cout << std::endl;
    }
    catch (...) {
        print("An Error hapend with making new file in vfs for the given file! in this path: " + vfsPath + "\n", SYSTEM_ERROR);
        delete[] buffer;
        return;
    }

}

void Shell::cmd_get(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        print("Usage: get <vfs_file_path> <real_destination_path>", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    size_t startIndex = 0;
    bool rootAccess = false;

    if (args[0] == ROOT_ACCESS) {
        startIndex = 1;
        rootAccess = true;
    }

    std::string vfsPath = args[1 + startIndex];
    std::string realPath = args[2 + startIndex];

    if (!fs->exists(vfsPath)) {
        print("The Given VFS Path Does Not Exist", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    if (fs->is_dir(vfsPath)) {
        print("The Given Path Is A Directory, Cannot Download A Directory", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    try {
        FileHandle fh(vfsPath, inodeFlags::OwnerRead, rootAccess);

        size_t fileSize = fh.size();

        if (fileSize == 0) {
            print("Warning: VFS file is empty.", SPESIAL);
            std::cout << std::endl;
        }

        byte* buffer = new byte[fileSize];

        if (fh.read(buffer, fileSize) != fileSize) {
            print("Error while reading from VFS!\n", SYSTEM_ERROR);
            delete[] buffer;
            return;
        }

        std::ofstream realFile(realPath, std::ios::binary);
        if (!realFile) {
            print("Failed to create/open real file at: " + realPath + "\n", SYSTEM_ERROR);
            delete[] buffer;
            return;
        }

        realFile.write(reinterpret_cast<const char*>(buffer), fileSize);
        realFile.close();

        delete[] buffer;
    }
    catch (const std::exception& e) {
        print("FileHandle Error: " + std::string(e.what()) + "\n", SYSTEM_ERROR);
        return;
    }

    print("File Downloaded from VFS to Local OS NORMALfully", NORMAL);
    std::cout << std::endl;
}

void Shell::cmd_exit(const std::vector<std::string>& args) {
    running = false;
}

void Shell::cmd_theme(const std::vector<std::string>& args)
{
    size_t startIndex = 0;

    if (!args.empty() && args[0] == ROOT_ACCESS) {
        startIndex = 1;
    }

    if (args.size() < 2+startIndex) {
        print("Usage: theme <theme_name>", USER_ERROR);
        std::cout << std::endl;
        return;
    }

    std::string themeName = args[1 + startIndex];

    if (Themes.count(themeName)) {
        currentThem = Themes[themeName];
        print("Theme chaged!", SUCCESS);
        std::cout << std::endl;
    }
    else {
        print("this theme dosen't exist : " + themeName, USER_ERROR);
        std::cout << std::endl;
    }
}

void const Shell::print(std::string msg, Mode mode)
{
    std::string colorCode;
    switch (mode) {

    case 1:
        colorCode = currentThem.NORMAL;
        break;

    case 2:
        colorCode = currentThem.SPESIAL;
        break;

    case 3:
        colorCode = currentThem.SYSTEM_ERROR;
        break;

    case 4:
        colorCode = currentThem.USER_ERROR;
        break;

    case 5:
        colorCode = currentThem.SYSTEM_ERROR;

        std::cout << colorCode << msg << "\033[0m";
    }
}

// Convert string flag to inode flag using Reference
bool Shell::parseToInodeFlags(std::string modeStr, inodeFlags& outFlags) {
    outFlags = inodeFlags::None;

    if (modeStr.empty()) {
        outFlags = inodeFlags::OwnerRead | inodeFlags::OwnerWrite;
        return true;
    }

    if (modeStr.size() > 2) return false;

    bool hasR = false, hasW = false;
    inodeFlags tempFlags = inodeFlags::None;

    for (char c : modeStr) {
        switch (c) {
        case 'r':
            if (hasR) return false;
            tempFlags |= inodeFlags::OwnerRead;
            hasR = true;
            break;
        case 'w':
            if (hasW) return false;
            tempFlags |= inodeFlags::OwnerWrite;
            hasW = true;
            break;
        default:
            return false;
        }
    }

    outFlags = tempFlags;
    return true;
}

// ====================== defult Theme's ========================
std::unordered_map<std::string, Theme> Tems = {
    {"hacker", {
    "\033[0;32;40m",     // Green on Black
    "\033[1;92;40m",     // Bright Green Bold on Black
    "\033[1;33;40m",     // Yellow Bold on Black
    "\033[1;31;40m",      // Red Bold on Black
    "\033[1;32;40m"    // NORMAL -> Green Bold
    }},
    {"girly", {
    "\033[0;95;40m",     // Pink on Black
    "\033[1;35;45m",     // Bold Magenta on Purple BG
    "\033[1;95;45m",     // Light Pink on Purple BG
    "\033[1;97;41m",      // White on Red BG
    "\033[1;35;40m"    // NORMAL -> Soft Magenta
    }},
    {"normal", {
    "\033[0;37;40m",     // White on Black
    "\033[1;36;40m",     // Cyan Bold on Black
    "\033[0;33;40m",     // Yellow on Black
    "\033[0;31;40m",      // Red on Black
    "\033[0;32;40m"    // NORMAL -> Green
    }}
};