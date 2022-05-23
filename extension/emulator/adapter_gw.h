#ifndef adapter_gw_h
#define adapter_gw_h

#include <stdbool.h>

typedef struct _GKGameWatchAdapter GKGameWatchAdapter;

extern const unsigned char* ROM_DATA;
extern const char* ROM_EXT;
extern unsigned ROM_DATA_LENGTH;

GKGameWatchAdapter* GKGameWatchAdapterCreate(void);
void GKGameWatchAdapterDestroy(GKGameWatchAdapter* adapter);
bool GKGameWatchAdapterLoad(GKGameWatchAdapter* adapter, const char* path);
void GKGameWatchAdapterUpdate(GKGameWatchAdapter* adapter, unsigned int dt);

#endif