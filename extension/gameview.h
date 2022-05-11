// gameview.h
// Gamekid by Dustin Mierau

#ifndef gameview_h
#define gameview_h

#include <stdbool.h>

typedef struct _GKApp GKApp;
typedef struct _GKGameView GKGameView;

extern GKGameView* GKGameViewCreate(GKApp* app);
extern void GKGameViewDestroy(GKGameView* view);
extern void GKGameViewReset(GKGameView* view);
extern void GKGameViewSave(GKGameView* view);
extern bool GKGameViewShow(GKGameView* view, const char* path);
extern void GKGameViewUpdate(GKGameView* view, unsigned int dt);
extern void GKGameViewSetScale(GKGameView* view, int scale);
extern void GKGameViewSetInterlaced(GKGameView* view, bool enabled);

#endif