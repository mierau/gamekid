// gameview.c
// Gamekid by Dustin Mierau

#include "app.h"
#include "gameview.h"
#include "common.h"
#include "utility.h"
#include <ctype.h>
#if GAMEBOY_ENABLED
#include "emulator/adapter_gb.h"
#endif
#if GAMEWATCH_ENABLED
#include "emulator/adapter_gw.h"
#endif

typedef struct _GKGameView {
	PDMenuItem* library_menu;
#if GAMEBOY_ENABLED
	GKGameBoyAdapter* gameboy;
#endif
#if GAMEWATCH_ENABLED
	GKGameWatchAdapter* gamewatch;
#endif
} GKGameView;

static void free_adapters(GKGameView* view);
static void menu_item_library(void* context);

GKGameView* GKGameViewCreate(void) {
	GKGameView* view = malloc(sizeof(GKGameView));
	memset(view, 0, sizeof(GKGameView));
	return view;
}

void GKGameViewDestroy(GKGameView* view) {
	if(view->library_menu != NULL) {
		playdate->system->removeMenuItem(view->library_menu);
		view->library_menu = NULL;
	}
	free_adapters(view);
	free(view);
}

bool GKGameViewShow(GKGameView* view, const char* path) {
	free_adapters(view);
	
	if(view->library_menu == NULL) {
		view->library_menu = playdate->system->addMenuItem("Library", menu_item_library, app);
	}
	
	// Check file type.
	// TODO
	const char* file_extension = GKGetFileExtension(path);
	if(file_extension == NULL) {
		return false;
	}
	
	// Load appropriate emulator.
#if GAMEBOY_ENABLED
	if(strcasecmp(file_extension, ".gb") == 0) {
		view->gameboy = GKGameBoyAdapterCreate();
		if(GKGameBoyAdapterLoad(view->gameboy, path)) {
			return true;
		}
	}
	else
#endif
#if GAMEWATCH_ENABLED
	if(strcasecmp(file_extension, ".mgw") == 0) {
		view->gamewatch = GKGameWatchAdapterCreate();
		if(GKGameWatchAdapterLoad(view->gamewatch, path)) {
			return true;
		}
	}
	else
#endif
	{}
	
	return false;
}

void GKGameViewHide(GKGameView* view) {
	free_adapters(view);
	
	if(view->library_menu != NULL) {
		playdate->system->removeMenuItem(view->library_menu);
		view->library_menu = NULL;
	}
}

void GKGameViewUpdate(GKGameView* view, unsigned int dt) {
#if GAMEBOY_ENABLED
	if(view->gameboy != NULL) {
		GKGameBoyAdapterUpdate(view->gameboy, dt);
	}
	else
#endif
#if GAMEWATCH_ENABLED
	if(view->gamewatch != NULL) {
		GKGameWatchAdapterUpdate(view->gamewatch, dt);
	}
#endif
	{}
}

#pragma mark -

static void free_adapters(GKGameView* view) {
#if GAMEBOY_ENABLED
	if(view->gameboy != NULL) {
		GKGameBoyAdapterDestroy(view->gameboy);
		view->gameboy = NULL;
	}
#endif
#if GAMEWATCH_ENABLED
	if(view->gamewatch != NULL) {
		GKGameWatchAdapterDestroy(view->gamewatch);
		view->gamewatch = NULL;
	}
#endif
}

static void menu_item_library(void* context) {
	GKAppGoToLibrary(app);
}
