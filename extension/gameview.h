// gameview.h
// Gamekid by Dustin Mierau

#ifndef gameview_h
#define gameview_h

#include <stdbool.h>

typedef struct _GKGameView GKGameView;

GKGameView* GKGameViewCreate(void);
void GKGameViewDestroy(GKGameView* view);
bool GKGameViewShow(GKGameView* view, const char* path);
void GKGameViewHide(GKGameView* view);
void GKGameViewUpdate(GKGameView* view, unsigned int dt);

#endif