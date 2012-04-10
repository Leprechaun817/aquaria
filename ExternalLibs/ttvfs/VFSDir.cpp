// VFSDir.cpp - basic directory interface + classes
// For conditions of distribution and use, see copyright notice in VFS.h

#include <set>

#include "VFSInternal.h"
#include "VFSTools.h"
#include "VFSFile.h"
#include "VFSDir.h"

VFS_NAMESPACE_START


VFSDir::VFSDir(const char *fullpath)
{
    _setName(fullpath);
}

VFSDir::~VFSDir()
{
    for(Files::iterator it = _files.begin(); it != _files.end(); ++it)
        it->second->ref--;
    for(Dirs::iterator it = _subdirs.begin(); it != _subdirs.end(); ++it)
        it->second->ref--;
}

unsigned int VFSDir::load(bool recursive)
{
    return 0;
}

VFSDir *VFSDir::createNew(const char *dir) const
{
    return new VFSDir(dir);
}

bool VFSDir::add(VFSFile *f, bool overwrite /* = true */)
{
    if(!f)
        return false;

    VFS_GUARD_OPT(this);

    Files::iterator it = _files.find(f->name());
    
    if(it != _files.end())
    {
        if(overwrite)
        {
            VFSFile *oldf = it->second;
            if(oldf == f)
                return false;

            _files.erase(it);
            oldf->ref--;
        }
        else
            return false;
    }

    f->ref++;
    _files[f->name()] = f;
    return true;
}

bool VFSDir::addRecursive(VFSFile *f, bool overwrite /* = true */)
{
    if(!f)
        return false;

    VFS_GUARD_OPT(this);

    // figure out directory from full file name
    VFSDir *vdir;
    size_t prefixLen = f->fullnameLen() - f->nameLen();
    if(prefixLen)
    {
        char *dirname = (char*)VFS_STACK_ALLOC(prefixLen);
        --prefixLen; // -1 to strip the trailing '/'. That's the position where to put the terminating null byte.
        memcpy(dirname, f->fullname(), prefixLen); // copy trailing null byte
        dirname[prefixLen] = 0;
        vdir = getDir(dirname, true);
        VFS_STACK_FREE(dirname);
    }
    else
        vdir = this;

    return vdir->add(f, true);
}

bool VFSDir::merge(VFSDir *dir, bool overwrite /* = true */)
{
    if(!dir)
        return false;
	if(dir == this)
		return true; // nothing to do then

    bool result = false;
    VFS_GUARD_OPT(this);

    for(Files::iterator it = dir->_files.begin(); it != dir->_files.end(); ++it)
        result = add(it->second, overwrite) || result;

    for(Dirs::iterator it = dir->_subdirs.begin(); it != dir->_subdirs.end(); ++it)
        result = insert(it->second, overwrite) || result;
    return result;
}

bool VFSDir::insert(VFSDir *subdir, bool overwrite /* = true */)
{
    if(!subdir)
        return false;

    VFS_GUARD_OPT(this);

    // With load() cleaning up the tree, it is ok to add subsequent VFSDirs directly.
    // This will be very useful at some point when files can be mounted into a directory
    // belonging to an archive, and this way adding files to the archive.
    Dirs::iterator it = _subdirs.find(subdir->name());
    if(it == _subdirs.end())
    {
        subdir->ref++;
        _subdirs[subdir->name()] = subdir;
        return true;
    }
    else
    {
        it->second->merge(subdir, overwrite);
        return false;
    }
}

VFSFile *VFSDir::getFile(const char *fn)
{
    while(fn[0] == '.' && fn[1] == '/') // skip ./
        fn += 2;

    char *slashpos = (char *)strchr(fn, '/');

    // if there is a '/' in the string, descend into subdir and continue there
    if(slashpos)
    {
        // "" (the empty directory name) is allowed, so we can't return 'this' when hitting an empty string the first time.
        // This whole mess is required for absolute unix-style paths ("/home/foo/..."),
        // which, to integrate into the tree properly, sit in the root directory's ""-subdir.
        // FIXME: Bad paths (double-slashes and the like) need to be normalized elsewhere, currently!

        size_t len = strlen(fn);
        char *dup = (char*)VFS_STACK_ALLOC(len + 1);
        memcpy(dup, fn, len + 1); // also copy the null-byte
        slashpos = dup + (slashpos - fn); // use direct offset, not to have to recount again the first time
        VFSDir *subdir = this;
        const char *ptr = dup;
        Dirs::iterator it;
        VFS_GUARD_OPT(this);

        goto pos_known;
        do
        {
            ptr = slashpos + 1;
            while(ptr[0] == '.' && ptr[1] == '/') // skip ./
                ptr += 2;
            slashpos = (char *)strchr(ptr, '/');
            if(!slashpos)
                break;

        pos_known:
            *slashpos = 0;
            it = subdir->_subdirs.find(ptr);
            if(it != subdir->_subdirs.end())
                subdir = it->second; // found it
            else
                subdir = NULL; // bail out
        }
        while(subdir);
        VFS_STACK_FREE(dup);
        if(!subdir)
            return NULL;
        // restore pointer into original string, by offset
        ptr = fn + (ptr - dup);

        Files::iterator ft = subdir->_files.find(ptr);
        return ft != subdir->_files.end() ? ft->second : NULL;
    }

    // no subdir? file must be in this dir now.
    VFS_GUARD_OPT(this);
    Files::iterator it = _files.find(fn);
    return it != _files.end() ? it->second : NULL;
}

VFSDir *VFSDir::getDir(const char *subdir, bool forceCreate /* = false */)
{
    if(!subdir[0] || (subdir[0] == '.' && (!subdir[1] || subdir[1] == '/'))) // empty string or "." or "./" ? use this.
        return this;

    VFSDir *ret = NULL;
    char *slashpos = (char *)strchr(subdir, '/');

    // if there is a '/' in the string, descend into subdir and continue there
    if(slashpos)
    {
        // from a/b/c, cut out the a, without the trailing '/'.
        const char *sub = slashpos + 1;
        size_t copysize = slashpos - subdir;
        char * const t = (char*)VFS_STACK_ALLOC(copysize + 1);
        memcpy(t, subdir, copysize);
        t[copysize] = 0;
        VFS_GUARD_OPT(this);
        Dirs::iterator it = _subdirs.find(t);
        if(it != _subdirs.end())
        {
            ret = it->second->getDir(sub, forceCreate); // descend into subdirs
        }
        else if(forceCreate)
        {
            // -> newname = fullname() + '/' + t
            size_t fullLen = fullnameLen();
            VFSDir *ins;
            if(fullLen)
            {
                char * const newname = (char*)VFS_STACK_ALLOC(fullLen + copysize + 2);
                char *ptr = newname;
                memcpy(ptr, fullname(), fullLen);
                ptr += fullLen;
                *ptr++ = '/';
                memcpy(ptr, t, copysize);
                ptr[copysize] = 0;
                ins = createNew(newname);
                VFS_STACK_FREE(newname);
            }
            else
                ins = createNew(t);

            _subdirs[ins->name()] = ins;
            ret = ins->getDir(sub, true); // create remaining structure
        }
    }
    else
    {
        VFS_GUARD_OPT(this);
        Dirs::iterator it = _subdirs.find(subdir);
        if(it != _subdirs.end())
            ret = it->second;
        else if(forceCreate)
        {
            size_t fullLen = fullnameLen();
            if(fullLen)
            {
                // -> newname = fullname() + '/' + subdir
                size_t subdirLen = strlen(subdir);
                char * const newname = (char*)VFS_STACK_ALLOC(fullLen + subdirLen + 2);
                char *ptr = newname;
                memcpy(ptr, fullname(), fullLen);
                ptr += fullLen;
                *ptr++ = '/';
                memcpy(ptr, subdir, subdirLen);
                ptr[subdirLen] = 0;

                ret = createNew(newname);
                VFS_STACK_FREE(newname);
            }
            else
            {
                ret = createNew(subdir);
            }

            _subdirs[ret->name()] = ret;
        }
    }

    return ret;
}

void VFSDir::clearFiles(bool recursive)
{
	// FIXME: this should be done using a non-recursive stack/set implementation for safety
	if(recursive)
		for(DirIter it = _subdirs.begin(); it != _subdirs.end(); ++it)
			it->second->clearFiles(true);

	for(FileIter it = _files.begin(); it != _files.end(); ++it)
		it->second->ref--;
	_files.clear();
}



// ----- VFSDirReal start here -----


VFSDirReal::VFSDirReal(const char *dir) : VFSDir(dir)
{
}

VFSDir *VFSDirReal::createNew(const char *dir) const
{
    return new VFSDirReal(dir);
}

unsigned int VFSDirReal::load(bool recursive)
{
    VFS_GUARD_OPT(this);

    Files cpf;
    Dirs cpd;

    StringList li;
    GetFileList(fullname(), li);
    for(StringList::iterator it = li.begin(); it != li.end(); ++it)
    {
        if(VFSFile *oldf = getFile(it->c_str()))
        {
            cpf[oldf->name()] = oldf;
            continue;
        }

        // TODO: use stack alloc
        std::string tmp = fullname();
        tmp += '/';
        tmp += *it;
        VFSFileReal *f = new VFSFileReal(tmp.c_str());
        _files[f->name()] = f;
    }
    unsigned int sum = li.size();

    li.clear();
    GetDirList(fullname(), li, false);
    for(std::deque<std::string>::iterator it = li.begin(); it != li.end(); ++it)
    {
        if(VFSDir *oldd = getDir(it->c_str()))
        {
            if(recursive)
                sum += oldd->load(true);
            ++sum;

            cpd[oldd->name()] = oldd;
            continue;
        }

        // TODO: use stack alloc
        std::string full(fullname());
        full += '/';
        full += *it; // GetDirList() always returns relative paths

        VFSDir *d = createNew(full.c_str());
        if(recursive)
            sum += d->load(true);
        ++sum;
        _subdirs[d->name()] = d;
    }

    // clean up & remove no longer existing files & dirs
    if(cpf.size())
        for(FileIter it = cpf.begin(); it != cpf.end(); ++it)
            if(_files.find(it->first) == _files.end())
                it->second->ref--;
    if(cpd.size())
        for(DirIter it = cpd.begin(); it != cpd.end(); ++it)
            if(_subdirs.find(it->first) == _subdirs.end())
                it->second->ref--;

    return sum;
}

VFS_NAMESPACE_END
