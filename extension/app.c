// app.c
// Gamekid by Dustin Mierau

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "app.h"
#include "libraryview.h"
#include "gameview.h"
#include "pd_api.h"

typedef struct _GKApp {
	GKAppScene scene;
	GKGameView* gameview;
	GKLibraryView* libraryview;
	unsigned int last_time;
	bool display_fps;
} GKApp;

static int GKAppRunloop(void* context);

GKAppScene GKAppGetCurrentScene(GKApp* app) {
	return app->scene;
}

void GKAppRun(void) {
	// Check for games folder, and create if necessary.
	if(playdate->file->stat("games", NULL) == -1) {
		playdate->file->mkdir("games");
	}
	
	// Check for saves folder, and create if necessary.
	if(playdate->file->stat("saves", NULL) == -1) {
		playdate->file->mkdir("saves");
	}

	app = malloc(sizeof(GKApp));
	memset(app, 0, sizeof(GKApp));
	app->libraryview = GKLibraryViewCreate(app);
	app->gameview = GKGameViewCreate();
	app->scene = kGKAppSceneBooting;
	app->last_time = playdate->system->getCurrentTimeMilliseconds();
		
	playdate->display->setRefreshRate(50);
	playdate->system->setUpdateCallback(GKAppRunloop, app);
}

void GKAppDestroy(GKApp* app) {
	playdate->system->setUpdateCallback(NULL, NULL);
	free(app);
}

#pragma mark -

void GKAppGoToGame(GKApp* app, const char* path) {
	if(app->scene == kGKAppSceneGame) {
		return;
	}
	
	if(app->scene == kGKAppSceneLibrary) {
		GKLibraryViewHide(app->libraryview);
	}
	
	app->scene = kGKAppSceneGame;
	if(!GKGameViewShow(app->gameview, path)) {
		app->scene = kGKAppSceneBooting;
	}
}

void GKAppGoToLibrary(GKApp* app) {
	if(app->scene == kGKAppSceneLibrary) {
		return;
	}
	
	if(app->scene == kGKAppSceneGame) {
		GKGameViewHide(app->gameview);
	}
	
	app->scene = kGKAppSceneLibrary;
	GKLibraryViewShow(app->libraryview);
}

static int GKAppRunloop(void* context) {
	GKApp* app = (GKApp*)context;
	
	// Calculate time since last frame.
	unsigned int t = playdate->system->getCurrentTimeMilliseconds();
	unsigned int dt = t - app->last_time;
	app->last_time = t;
	
	// Load cartridge and turn on Gamekid system.
	if(app->scene != kGKAppSceneGame) {
		if(app->scene == kGKAppSceneBooting) {
			playdate->graphics->clear(kColorWhite);
			GKAppGoToLibrary(app);
		}
		else if(app->scene == kGKAppSceneLibrary) {
			GKLibraryViewUpdate(app->libraryview, dt);
		}
	}
	else {
		GKGameViewUpdate(app->gameview, dt);
	}

	if(app->display_fps) {
		playdate->system->drawFPS(380, 5);
	}
	
	return 1;
}

void GKAppSetFPSEnabled(bool enabled) {
	app->display_fps = enabled;
}

bool GKAppGetFPSEnabled(void) {
	return app->display_fps;
}