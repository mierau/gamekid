// libraryview.h
// Gamekid by Dustin Mierau

#ifndef libraryview_h
#define libraryview_h

#include <stdbool.h>

typedef struct _GKApp GKApp;
typedef struct _GKLibraryView GKLibraryView;

GKLibraryView* GKLibraryViewCreate(GKApp* app);
void GKLibraryViewDestroy(GKLibraryView* view);
void GKLibraryViewShow(GKLibraryView* view);
void GKLibraryViewHide(GKLibraryView* view);
void GKLibraryViewUpdate(GKLibraryView* view, unsigned int dt);

#endif