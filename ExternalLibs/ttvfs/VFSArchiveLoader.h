#ifndef VFS_ARCHIVE_LOADER_H
#define VFS_ARCHIVE_LOADER_H

#include "VFSDefines.h"

VFS_NAMESPACE_START

class VFSDir;
class VFSFile;
class VFSLoader;

class VFSArchiveLoader
{
public:
    virtual ~VFSArchiveLoader() {}
    virtual VFSDir *Load(VFSFile *arch, bool asSubdir, VFSLoader **ldr) = 0;
};

VFS_NAMESPACE_END

#endif
