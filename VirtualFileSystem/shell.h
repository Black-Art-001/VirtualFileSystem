#pragma once

#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "FileSystem.h"

class Shell {
public:
    Shell(FileSystem* _fs);
    void run();

private:
    FileSystem* fs;
    bool running;

    void execute(const std::string& line);
    std::vector<std::string> tokenize(const std::string& line);

    void handle_ls(const std::vector<std::string>& args);
    void handle_put(const std::string& hostPath, const std::string& virtualPath);
    void handle_get(const std::string& virtualPath, const std::string& hostPath);
};