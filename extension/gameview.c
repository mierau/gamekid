// gameview.c
// Gamekid by Dustin Mierau

#include "app.h"
#include "gameview.h"
#include "common.h"
#include "emulator/adapter_gb.h"

typedef struct _GKGameView {
	PDMenuItem* library_menu;
	GKGameBoyAdapter* gameboy;
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
	
	view->gameboy = GKGameBoyAdapterCreate();
	if(!GKGameBoyAdapterLoad(view->gameboy, path)) {
		return false;
	}
	
	return true;
}

void GKGameViewHide(GKGameView* view) {
	free_adapters(view);
	
	if(view->library_menu != NULL) {
		playdate->system->removeMenuItem(view->library_menu);
		view->library_menu = NULL;
	}
}

void GKGameViewUpdate(GKGameView* view, unsigned int dt) {
	GKGameBoyAdapterUpdate(view->gameboy, dt);
}

#pragma mark -

static void free_adapters(GKGameView* view) {
	if(view->gameboy != NULL) {
		GKGameBoyAdapterDestroy(view->gameboy);
		view->gameboy = NULL;
	}
}

static void menu_item_library(void* context) {
	GKAppGoToLibrary(app);
}
