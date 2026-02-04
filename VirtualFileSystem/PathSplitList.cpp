#include "PathSplitList.h"

void PathSplitList::addPath(string& path)
{
    if (path[0] == '\\' or path[0] == '/') {
        path = path.substr(1, -1);
        splitedPath->push_back("\\");
    }

    string word;
    for (char c : path)
    {
        if (c == '\\' || c == '/') {
            addWord(word);
            word = ""; // reset word
        }
        else {
            word += c;
        }
    }
    if (word != "\\" && word != "/")
    {
        addWord(word);
    }
}

void PathSplitList::addWord(string word)
{
    if (word == "." || word == "") return;

    if (word == "..")
    {
        if (splitedPath->size() == 1 && ((*splitedPath)[0] == "/" || (*splitedPath)[0] == "\\")) return;

        if (splitedPath->back() == ".." || splitedPath->empty()) {
            splitedPath->push_back("..");
        }
        else
        {
            splitedPath->pop_back();
        }
    }
    else
    {
        splitedPath->push_back(word);
    }
}
