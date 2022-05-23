// Gamekid
// utility.h

#ifndef utility_h
#define utility_h

const char* GKGetFilename(const char* path, int* len);
const char* GKGetFileExtension(const char* path);
unsigned char* GKReadFileContents(const char* path);

#endif