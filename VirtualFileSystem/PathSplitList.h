#pragma once
#include <vector>
#include <string>
using std::string;
using std::vector;

class PathSplitList {
public:
    PathSplitList(string& path) { addPath(path); }

    PathSplitList() {}

    ~PathSplitList() { delete splitedPath; }

    void addPath(string& path);

    string operator[](size_t index) { return (*splitedPath)[index]; }

private:
    void addWord(string word);
    vector<string>* splitedPath = new vector<string>;
};