// libraryview.h
// Gamekid by Dustin Mierau

#ifndef libraryview_h
#define libraryview_h

#include <stdbool.h>

typedef struct _GKApp GKApp;
typedef struct _GKLibraryView GKLibraryView;

extern GKLibraryView* GKLibraryViewCreate(GKApp* app);
extern void GKLibraryViewDestroy(GKLibraryView* view);
extern void GKLibraryViewShow(GKLibraryView* view);
extern void GKLibraryViewUpdate(GKLibraryView* view, unsigned int dt);

#endif