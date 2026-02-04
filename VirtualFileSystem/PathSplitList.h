#pragma once
#include <list>
#include <string>
using std::string;
using std::list;

class PathSplitList {
public:
    PathSplitList(string& path) { addPath(path); }

    PathSplitList() {}

    ~PathSplitList() { delete splitedPath; }

    void addPath(string& path);

    const list<string>* const getList() const { return splitedPath; }

    void removeWord(string word) { splitedPath->remove(word); }

    void popBack() { splitedPath->pop_back(); }

    void popFront() { splitedPath->pop_front(); }

private:
    void addWord(string word);
    list<string>* splitedPath = new list<string>;
};