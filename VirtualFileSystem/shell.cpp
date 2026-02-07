#include "Shell.h"
#include "FileHandle.h"
#include <sstream>
#include <algorithm>
#include <fstream>
#include <iomanip>

Shell::Shell(FileSystem* _fs) : fs(_fs), running(true) {}

void Shell::run() {
    std::string line;
    while (running) {
        std::cout << "FS_Shell:" << fs->get_current_path() << "> ";
        if (!std::getline(std::cin, line) || line == "exit") break;
        if (line.empty()) continue;
        execute(line);
    }
}

std::vector<std::string> Shell::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string temp;
    while (ss >> temp) tokens.push_back(temp);
    return tokens;
}

void Shell::execute(const std::string& line) {
    auto args = tokenize(line);
    if (args.empty()) return;

    std::string cmd = args[0];

    if (cmd == "pwd") {
        std::cout << fs->get_current_path() << std::endl; //
    }
    else if (cmd == "cd" && args.size() > 1) {
        if (!fs->cd(args[1])) std::cerr << "Error: Directory not found.\n"; //
    }
    else if (cmd == "ls") {
        handle_ls(args); //
    }
    else if (cmd == "mkdir" && args.size() > 1) {
        fs->mkdir(args[1], inodeFlags::OwnerRead | inodeFlags::OwnerWrite); //
    }
    else if (cmd == "rm" && args.size() > 1) {
        // ??? ???? ??? rmall ? ??? ???? ??? unlink
        if (!fs->rmall(args[1])) fs->unlink(args[1]);
    }
    else if (cmd == "cp" && args.size() > 2) {
        fs->copy(args[2], args[1]); //
    }
    else if (cmd == "mv" && args.size() > 2) {
        fs->move(args[1], args[2]); //
    }
    else if (cmd == "put" && args.size() > 2) {
        handle_put(args[1], args[2]); //
    }
    else if (cmd == "get" && args.size() > 2) {
        handle_get(args[1], args[2]); //
    }
    else {
        std::cerr << "Unknown command or missing arguments.\n";
    }
}

void Shell::handle_ls(const std::vector<std::string>& args) {
    // ??? ??? ls ???? ???? ?????? ?? ????? ???? ?????
    if (args.size() == 1) {
        auto list = fs->ls(".");
        std::sort(list.begin(), list.end());
        for (const auto& name : list) std::cout << name << "  ";
        std::cout << std::endl;
    }
    // ??? ls <PATH> ???? ????? ??????? ???? ????
    else {
        if (fs->exists(args[1])) {
            std::cout << "Name: " << args[1] << "\n"
                << "Size: " << fs->get_size(args[1]) << " bytes\n"
                << "Type: " << (fs->is_dir(args[1]) ? "Directory" : "File") << std::endl;
        }
        else {
            std::cerr << "Path does not exist.\n";
        }
    }
}

void Shell::handle_put(const std::string& hostPath, const std::string& virtualPath) {
    std::ifstream hostFile(hostPath, std::ios::binary);
    if (!hostFile) { std::cerr << "Host file not found.\n"; return; }

    fs->touch(virtualPath, inodeFlags::OwnerRead | inodeFlags::OwnerWrite);
    int fd = fs->open(virtualPath, 0);
    if (fd < 0) return;

    FileHandle vFile(fs, fd);
    char buffer[1024];
    while (hostFile.read(buffer, sizeof(buffer))) {
        vFile.write((byte*)buffer, hostFile.gcount());
    }
    vFile.write((byte*)buffer, hostFile.gcount()); // ????? ??????????
    std::cout << "File uploaded successfully.\n";
}

void Shell::handle_get(const std::string& virtualPath, const std::string& hostPath) {
    int fd = fs->open(virtualPath, 0);
    if (fd < 0) { std::cerr << "Virtual file not found.\n"; return; }

    std::ofstream hostFile(hostPath, std::ios::binary);
    FileHandle vFile(fs, fd);
    byte buffer[1024];
    size_t bytesRead;
    while ((bytesRead = vFile.read(buffer, sizeof(buffer))) > 0) {
        hostFile.write((char*)buffer, bytesRead);
    }
    std::cout << "File downloaded successfully.\n"; //
}