#pragma once

#include <unordered_map>
#include "types.h"
#include "PathSplitList.h"


class PathResolver {
public:
	PathResolver(FileSystem& fs);

	inodeID getInode(PathSplitList& stack , uint32 index = -1);
	list<string>::const_iterator getFirstNotExistingIterator(PathSplitList& stack);
	size_t getFirstNotExistingIndex(PathSplitList& stack);

	list<string>::const_iterator getLastExistingIterator(PathSplitList& stack);
	size_t getFirstExistingIndex(PathSplitList& stack);

	PathSplitList& getPathSplitList(inodeID inode);

private:
	list<Dentry> dentries;
	unordered_map<DentryKey, Dentry*, DentryHasher> DentryCache;
};