#include "VFSInternal.h"
#include "VFSZipArchiveLoader.h"
#include "VFSDirZip.h"

VFS_NAMESPACE_START

VFSDir *VFSZipArchiveLoader::Load(VFSFile *arch, bool asSubdir, VFSLoader ** /*unused*/)
{
    VFSDirZip *vd = new VFSDirZip(arch, asSubdir);
    if(vd->load(true))
        return vd;

    vd->ref--;
    return NULL;
}

VFS_NAMESPACE_END
