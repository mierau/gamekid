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
	PDMenuItem* library_menu;
	PDMenuItem* scale_menu;
	PDMenuItem* crank_menu;
	PDMenuItem* debug_menu;
	unsigned int last_time;
} GKApp;

static int GKAppRunloop(void* context);
static void libraryMenuItemCallback(void* context);
static void scaleMenuItemCallback(void* context);
static void crankMenuItemCallback(void* context);
static void debugMenuItemCallback(void* context);

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

	GKApp* app = malloc(sizeof(GKApp));
	memset(app, 0x00, sizeof(GKApp));
	
	app->libraryview = GKLibraryViewCreate(app);
	app->gameview = GKGameViewCreate(app);
	app->scene = kGKAppSceneBooting;
	
	playdate->display->setRefreshRate(60);
	playdate->system->setUpdateCallback(GKAppRunloop, app);
	
	app->last_time = playdate->system->getCurrentTimeMilliseconds();
	
	const char* menu_items[] = {
		"natural",
		"fitted",
		"sliced",
		"doubled"
	};
	
	const char* crank_menu_items[] = {
		"start/sel",
		"position"
	};
	
	app->library_menu = playdate->system->addMenuItem("Library", libraryMenuItemCallback, app);
	app->scale_menu = playdate->system->addOptionsMenuItem("Scale", menu_items, 4, scaleMenuItemCallback, app);
	// app->crank_menu = playdate->system->addOptionsMenuItem("Crank", crank_menu_items, 2, crankMenuItemCallback, app);
	app->debug_menu = playdate->system->addCheckmarkMenuItem("Interlace", 1, debugMenuItemCallback, app);
	
	playdate->system->setMenuItemValue(app->scale_menu, 1);
}

void GKAppDestroy(GKApp* app) {
	playdate->system->setUpdateCallback(NULL, NULL);
	if(app->library_menu != NULL) {
		playdate->system->removeMenuItem(app->library_menu);
	}
	if(app->scale_menu != NULL) {
		playdate->system->removeMenuItem(app->scale_menu);
	}
	if(app->crank_menu != NULL) {
		playdate->system->removeMenuItem(app->crank_menu);
	}
	free(app);
}

#pragma mark -

void GKAppGoToGame(GKApp* app, const char* path) {
	if(app->scene == kGKAppSceneGame) {
		return;
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
		GKGameViewSave(app->gameview);
		GKGameViewReset(app->gameview);
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
		return 1;
	}
	
	GKGameViewUpdate(app->gameview, dt);
	
	return 1;
}

static void libraryMenuItemCallback(void* context) {
	GKApp* app = (GKApp*)context;
	GKAppGoToLibrary(app);
}

static void scaleMenuItemCallback(void* context) {
	GKApp* app = (GKApp*)context;
	GKGameViewSetScale(app->gameview, playdate->system->getMenuItemValue(app->scale_menu));
}

static void crankMenuItemCallback(void* context) {
	GKApp* app = (GKApp*)context;
	// GKAppGoToLibrary(app);
}

static void debugMenuItemCallback(void* context) {
	GKApp* app = (GKApp*)context;
	GKGameViewSetInterlaced(app->gameview, playdate->system->getMenuItemValue(app->debug_menu));
}
