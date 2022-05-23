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

void GKAppRun(void);
void GKAppDestroy(GKApp* app);

GKAppScene GKAppGetCurrentScene(GKApp* app);
void GKAppGoToGame(GKApp* app, const char* path);
void GKAppGoToLibrary(GKApp* app);

void GKAppSetFPSEnabled(bool enabled);
bool GKAppGetFPSEnabled(void);

#endif