#ifndef FILE_API_H
#define FILE_API_H

// TODO: need VFS output functions?

#ifdef BBGE_BUILD_VFS

#include "ttvfs/VFS.h"
#include <sstream>

extern ttvfs::VFSHelper vfs;
typedef ttvfs::VFSFile VFILE;


VFILE *vfopen(const char *fn, const char *mode);
size_t vfread(void *ptr, size_t size, size_t count, VFILE *vf);
int vfclose(VFILE *vf);
size_t vfwrite(const void *ptr, size_t size, size_t count, VFILE *vf);
int vfseek(VFILE *vf, long int offset, int origin);
char *vfgets(char *str, int num, VFILE *vf);
void vfclear(VFILE *vf);
long int vftell(VFILE *vf);

// This class is a minimal adapter to support STL-like read-only file streams for VFS files, using std::istringstream.
class InStream : public std::istringstream
{
public:
    InStream(const char *fn);
    InStream(const std::string& fn);
    bool open(const char *fn);
    inline bool is_open() { return good(); }
    inline void close() {}
private:
    void _init(const char *fn);
};

#else // BBGE_BUILD_VFS

#include <stdio.h>
#include <fstream>
typedef std::ifstream InStream;
typedef FILE VFILE;
#define vfopen(a, b)            fopen((a), (b))
#define vfread(a, b, c, d)      fread((a), (b), (c), (d))
#define vfclose(a)              fclose((a))
#define vfwrite(a, b, c, d)     fwrite((a), (b), (c), (d))
#define vfseek(a, b, c)         fseek((a), (b), (c))
#define vfgets(a, b, c)         fgets((a), (b), (c))
#define vftell(a)               ftell(a)
#define vfclear(a)

#endif // BBGE_BUILD_VFS


#endif // FILE_API_H
