#ifndef VFSDIR_ZIP_H
#define VFSDIR_ZIP_H

#include "VFSDir.h"

#include "miniz.h"

VFS_NAMESPACE_START

class VFSFile;

class VFSDirZip : public VFSDir
{
public:
    VFSDirZip(VFSFile *zf, bool asSubdir);
    virtual ~VFSDirZip();
    virtual unsigned int load(bool recusive);
    virtual VFSDir *createNew(const char *dir) const;
    virtual const char *getType(void) const { return "VFSDirZip"; }

    inline mz_zip_archive *getZip(void) { return &_zip; }

protected:
    VFSFile *_zf;
    mz_zip_archive _zip;
    bool _isOpen;
    std::string zipfilename;
};

VFS_NAMESPACE_END

#endif
