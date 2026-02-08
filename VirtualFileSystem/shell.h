#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "FileSystem.h"
#include "colors.h"
#include <unordered_map>

constexpr auto ROOT_ACCESS = "god";

enum Mode {
    NORMAL = 1,
    SPESIAL,
    USER_ERROR,
    SYSTEM_ERROR,
    SUCCESS
};

struct Theme {
    std::string NORMAL;
    std::string SPESIAL;
    std::string USER_ERROR;
    std::string SYSTEM_ERROR;
    std::string SUCCESS;
};

class Shell {
public:
    explicit Shell(FileSystem* fs);
    void run(); 

private:
    FileSystem* fs;
    std::string currentPath;
    bool running;

    using CommandFunc = std::function<void(const std::vector<std::string>&)>;
    std::map<std::string, CommandFunc> commands;

    void initCommands();
    std::vector<std::string> tokenize(std::string line);

    // helper funs
    void cmd_pwd(const std::vector<std::string>& args);
    void cmd_cd(const std::vector<std::string>& args);
    void cmd_ls(const std::vector<std::string>& args);
    void cmd_mkdir(const std::vector<std::string>& args);
    void cmd_rm(const std::vector<std::string>& args);
    void cmd_cp(const std::vector<std::string>& args);
    void cmd_mv(const std::vector<std::string>& args);
    void cmd_put(const std::vector<std::string>& args);
    void cmd_get(const std::vector<std::string>& args);
    void cmd_exit(const std::vector<std::string>& args);
    void cmd_theme(const std::vector<std::string>& args);
    
    const void print(std::string msg, Mode mode);
    std::optional<inodeFlags> parseToInodeFlags(std::string modeStr);
    Theme currentThem = Themes["normal"]; // defult theme mode
    static std::unordered_map<std::string, Theme> Themes;
};