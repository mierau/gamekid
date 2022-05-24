// library.c
// Gamekid by Dustin Mierau

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "libraryview.h"
#include "pd_api.h"
#include "list.h"
#include "app.h"

typedef struct _GKLibraryView {
	GKApp* app;
	int selection;
	List* list;
	int list_length;
	int scroll_y;
	LCDBitmap* icon_image;
	LCDFont* font;
	uint32_t keyrepeat_timer;
	SamplePlayer* up_sound;
	SamplePlayer* down_sound;
	PDMenuItem* fps_menu;
} GKLibraryView;

static void menu_item_fps(void* context);

GKLibraryView* GKLibraryViewCreate(GKApp* app) {
	GKLibraryView* view = malloc(sizeof(GKLibraryView));
	memset(view, 0, sizeof(GKLibraryView));
	view->app = app;
	view->selection = 0;
	view->list = ListCreate();
	view->list_length = 0;
	view->scroll_y = 0;
	view->icon_image = playdate->graphics->loadBitmap("images/browser-icon", NULL);
	view->font = playdate->graphics->loadFont("fonts/Asheville-Sans-14-Bold", NULL);
	
	AudioSample* down_sample = playdate->sound->sample->load("sound/listdown");
	view->down_sound = playdate->sound->sampleplayer->newPlayer();
	playdate->sound->sampleplayer->setSample(view->down_sound, down_sample);
	
	AudioSample* up_sample = playdate->sound->sample->load("sound/listup");
	view->up_sound = playdate->sound->sampleplayer->newPlayer();
	playdate->sound->sampleplayer->setSample(view->up_sound, up_sample);
		
	return view;
}

void GKLibraryViewDestroy(GKLibraryView* view) {
	if(view->list != NULL) {
		ListDestroy(view->list);
	}
	if(view->icon_image != NULL) {
		playdate->graphics->freeBitmap(view->icon_image);
	}
	if(view->font != NULL) {
		free(view->font);
	}
	if(view->down_sound != NULL) {
		playdate->sound->sampleplayer->freePlayer(view->down_sound);
		view->down_sound = NULL;
	}
	if(view->up_sound != NULL) {
		playdate->sound->sampleplayer->freePlayer(view->up_sound);
		view->up_sound = NULL;
	}
	
	free(view);
}

#pragma mark -

void listFilesCallback(const char* filename, void* context) {
	GKLibraryView* view = (GKLibraryView*)context;
	
	if(filename[0] == '.') {
		return;
	}
	
	int filename_length = strlen(filename);
	const char* file_extension = strrchr(filename, '.');
	bool file_allowed = false;
	
	if(file_extension != NULL && file_extension != filename) {
#if GAMEBOY_ENABLED
		// Allow Game Boy files.
		if(strcasecmp(file_extension, ".gb") == 0) {
			file_allowed = true;
		}
		else
#endif
		{}
	}
	
	if(!file_allowed) {
		return;
	}
	
	void* filename_data = malloc(strlen(filename) + 1);
	memset(filename_data, 0, strlen(filename) + 1);
	strcpy(filename_data, filename);
	ListAppend(view->list, filename_data);
	view->list_length++;
}

void GKLibraryViewShow(GKLibraryView* view) {
	ListDestroyAll(view->list);
	view->list = ListCreate();
	view->list_length = 0;
	view->scroll_y = 0;
	view->selection = 0;
	
	if(view->fps_menu == NULL) {
		view->fps_menu = playdate->system->addCheckmarkMenuItem("FPS", GKAppGetFPSEnabled(), menu_item_fps, view);
	}
	
	playdate->file->listfiles("/games", listFilesCallback, view);
}

void GKLibraryViewHide(GKLibraryView* view) {
	if(view->fps_menu != NULL) {
		playdate->system->removeMenuItem(view->fps_menu);
		view->fps_menu = NULL;
	}
}

static void menu_item_fps(void* context) {
	GKLibraryView* view = (GKLibraryView*)context;
	GKAppSetFPSEnabled(playdate->system->getMenuItemValue(view->fps_menu));
}

#define LIST_ROW_HEIGHT 36
#define LIST_X 0
#define LIST_HEIGHT 240
#define LIST_WIDTH (400)

void GKLibraryDraw(GKLibraryView* view) {
	// playdate->graphics->fillRect(LIST_X, 0, LIST_WIDTH, LIST_HEIGHT, kColorWhite);
	playdate->graphics->clear(kColorWhite);
	playdate->graphics->setFont(view->font);
	
	if(view->list_length > 0) {
		ListNode* node = view->list->first;
		int i = 0;
		while(node->next != NULL) {
			if(i == view->selection) {
				playdate->graphics->fillRect(LIST_X, -view->scroll_y + LIST_ROW_HEIGHT * i, LIST_WIDTH, LIST_ROW_HEIGHT, kColorBlack);
				playdate->graphics->setDrawMode(kDrawModeFillWhite);
			}
			else {
				playdate->graphics->setDrawMode(kDrawModeFillBlack);
			}
			
			playdate->graphics->drawBitmap(view->icon_image, LIST_X + 10, -view->scroll_y + 10 + LIST_ROW_HEIGHT * i, kBitmapUnflipped);
			playdate->graphics->drawText(node->data, strlen((char*)node->data), kASCIIEncoding, LIST_X + 10 + 23, -view->scroll_y + 10 + LIST_ROW_HEIGHT * i);
			
			i++;
			node = node->next;
		}
	}
	else {
		const char* message = "Place your .gb files in\nGamekid's games folder\non your Playdate's Data Disk.\n\nThis is still very beta, enjoy!";
		playdate->graphics->setDrawMode(kDrawModeFillBlack);
		playdate->graphics->drawText(message, strlen(message), kASCIIEncoding, 30, 30);
	}

	playdate->graphics->setDrawMode(kDrawModeCopy);
}

void GKLibraryViewUpdate(GKLibraryView* view, unsigned int dt) {
	PDButtons buttons_just_pushed;
	PDButtons buttons_down;

	playdate->system->getButtonState(&buttons_down, &buttons_just_pushed, NULL);
	
	if((buttons_just_pushed & kButtonA) && (view->list_length > 0)) {
		void* filename = ListGet(view->list, view->selection);
		char path[256];
		
		memset(path, 0, 256);
		strcpy(path, "games/");
		strcpy(path + 6, filename);
		
		GKAppGoToGame(view->app, path);
		return;
	}
	
	int selection_delta = 0;
	
	if(buttons_just_pushed & kButtonDown) {
		view->keyrepeat_timer = 0;
		selection_delta = 1;
	}
	if(buttons_just_pushed & kButtonUp) {
		view->keyrepeat_timer = 0;
		selection_delta = -1;
	}
	
	view->keyrepeat_timer += dt;
	if(view->keyrepeat_timer >= 180) {
		view->keyrepeat_timer = 0;
		
		if(buttons_down & kButtonDown) {
			selection_delta = 1;
		}
		
		if(buttons_down & kButtonUp) {
			selection_delta = -1;
		}
	}
	
	if(selection_delta != 0) {
		int new_selection = GKMin(GKMax(view->selection+selection_delta, 0), view->list_length-1);
		
		playdate->sound->sampleplayer->stop(view->down_sound);
		playdate->sound->sampleplayer->stop(view->up_sound);
		if(new_selection > view->selection) {
			playdate->sound->sampleplayer->play(view->down_sound, 1, 1.0);
		}
		else if(new_selection < view->selection) {
			playdate->sound->sampleplayer->play(view->up_sound, 1, 1.0);
		}
		
		view->selection = new_selection;
	}
	
	// Update scroll.
	int scroll_bottom = view->scroll_y + LIST_HEIGHT;
	int line_y = 0;
	
	ListNode* node = view->list->first;
	int i = 0;
	while(node->next != NULL) {
		
		if(i == view->selection) {
			int line_height = LIST_ROW_HEIGHT;
			int line_bottom = line_y + line_height;
			
			if(view->scroll_y > line_y) {
				view->scroll_y = line_y - 0;
			}
			else if(scroll_bottom < line_bottom) {
				view->scroll_y += (line_bottom - scroll_bottom);
			}
		}
		
		line_y += LIST_ROW_HEIGHT + 0;
		node = node->next;
		i++;
	}

	GKLibraryDraw(view);
}