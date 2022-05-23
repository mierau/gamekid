#ifndef adapter_gb_h
#define adapter_gb_h

#include <stdbool.h>

typedef struct _GKGameBoyAdapter GKGameBoyAdapter;

GKGameBoyAdapter* GKGameBoyAdapterCreate(void);
void GKGameBoyAdapterDestroy(GKGameBoyAdapter* adapter);
bool GKGameBoyAdapterLoad(GKGameBoyAdapter* adapter, const char* path);
void GKGameBoyAdapterUpdate(GKGameBoyAdapter* adapter, unsigned int dt);

#endif