// app.h
// Gamekid by Dustin Mierau

#ifndef app_h
#define app_h

#include <stdbool.h>

typedef struct _GKApp GKApp;

enum {
	kGKAppSceneBooting = 0,
	kGKAppSceneLibrary = 1,
	kGKAppSceneGame = 2
};
typedef unsigned char GKAppScene;

extern void GKAppRun(void);
extern void GKAppDestroy(GKApp* app);

extern GKAppScene GKAppGetCurrentScene(GKApp* app);
extern void GKAppGoToGame(GKApp* app, const char* path);
extern void GKAppGoToLibrary(GKApp* app);

#endif