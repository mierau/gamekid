// gameview.h
// Gamekid by Dustin Mierau

#ifndef gameview_h
#define gameview_h

#include <stdbool.h>

typedef struct _GKGameView GKGameView;

extern GKGameView* GKGameViewCreate(void);
extern void GKGameViewDestroy(GKGameView* view);
extern bool GKGameViewShow(GKGameView* view, const char* path);
extern void GKGameViewHide(GKGameView* view);
extern void GKGameViewUpdate(GKGameView* view, unsigned int dt);

#endif