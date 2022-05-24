// Gamekid
// utility.h

#ifndef utility_h
#define utility_h

#include <stddef.h>

const char* GKGetFilename(const char* path, int* len);
const char* GKGetFileExtension(const char* path);
unsigned char* GKReadFileContents(const char* path, size_t* out_len);

#endif