// Gamekid
// GKPlatform.h

#ifndef common_h
#define common_h

#include <string.h>
#include "pd_api.h"

//#define GK_FORCEINLINE inline __attribute__((always_inline))
#define GK_FORCEINLINE __attribute__((always_inline)) inline

// Playdate extras
extern PlaydateAPI* playdate;

// Debug
#define GKLog(...) playdate->system->logToConsole(__VA_ARGS__)

// File
#define GKFile SDFile
#define kGKFileRead kFileRead
#define kGKFileReadData (kFileRead | kFileReadData)
#define kGKFileWrite kFileWrite
#define kGKFileAppend kFileAppend
#define kGKSeekSet 0
#define kGKSeekCurrent 1
#define kGKSeekEnd 2

#define GKFileOpen(path, mode) playdate->file->open((path), (mode))
#define GKFileClose(f) playdate->file->close((f))
#define GKFileSeek(f, position, whence) playdate->file->seek((f), (position), (whence))
#define GKFileRead(buffer, size, f) playdate->file->read((f), (buffer), (size))
#define GKFileWrite(buffer, size, f) playdate->file->write((f), (buffer), (size))
#define GKFileTell(f) playdate->file->tell((f))

// Get the min of two values without branching.
#define GKMin(x, y) ((y) ^ (((x) ^ (y)) & -((x) < (y))))

// Get the max of two values without branching.
#define GKMax(x, y) ((x) ^ (((x) ^ (y)) & -((x) < (y))))

#define kGKDisplayWidth 160
#define kGKDisplayHeight 144

#define GKFastDiv2(value) ((value) >> 1)
#define GKFastDiv4(value) ((value) >> 2)
#define GKFastDiv8(value) ((value) >> 3)
#define GKFastDiv16(value) ((value) >> 4)
#define GKFastDiv32(value) ((value) >> 5)

#define GKFastMod2(value) ((value) & 1)
#define GKFastMod4(value) ((value) & 3)
#define GKFastMod8(value) ((value) & 7)
#define GKFastMod16(value) ((value) & 15)
#define GKFastMod32(value) ((value) & 31)

#define GKFastMult2(value) ((value) << 1)
#define GKFastMult4(value) ((value) << 2)
#define GKFastMult8(value) ((value) << 3)
#define GKFastMult16(value) ((value) << 4)
#define GKFastMult32(value) ((value) << 5)

// Bit set or clear without branching.
#define GKSetOrClearBitIf(setCondition, bit, value) value ^= ((-(setCondition) ^ (value)) & (1 << (bit)))

// Negate a value without branching if a condition is true.
#define GKNegateIf(condition, value) ((value) ^ -(condition)) + (condition)

#endif
