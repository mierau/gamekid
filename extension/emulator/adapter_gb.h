#ifndef adapter_gb_h
#define adapter_gb_h

#include <stdbool.h>

typedef struct _GKGameBoyAdapter GKGameBoyAdapter;

extern GKGameBoyAdapter* GKGameBoyAdapterCreate(void);
extern void GKGameBoyAdapterDestroy(GKGameBoyAdapter* adapter);
extern bool GKGameBoyAdapterLoad(GKGameBoyAdapter* adapter, const char* path);
extern void GKGameBoyAdapterUpdate(GKGameBoyAdapter* adapter, unsigned int dt);

#endif