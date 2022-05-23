#include "adapter_gw.h"
#include "common.h"
#include "lib/utility.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

const unsigned char *ROM_DATA = NULL;
unsigned ROM_DATA_LENGTH;
const char *ROM_EXT = NULL;

typedef struct _GKGameWatchAdapter {
} GKGameWatchAdapter;

#pragma mark -

GKGameWatchAdapter* GKGameWatchAdapterCreate(void) {
	GKGameWatchAdapter* adapter = malloc(sizeof(GKGameWatchAdapter));
	memset(adapter, 0, sizeof(GKGameWatchAdapter));
	return adapter;
}

void GKGameWatchAdapterDestroy(GKGameWatchAdapter* adapter) {
	free(adapter);
}

bool GKGameWatchAdapterLoad(GKGameWatchAdapter* adapter, const char* path) {
	return true;
}

void GKGameWatchAdapterUpdate(GKGameWatchAdapter* adapter, unsigned int dt) {
	
}

#pragma mark -

unsigned int gw_get_buttons() {
	unsigned int hw_buttons = 0;
	// hw_buttons |= joystick.values[ODROID_INPUT_LEFT];
	// hw_buttons |= joystick.values[ODROID_INPUT_UP] << 1;
	// hw_buttons |= joystick.values[ODROID_INPUT_RIGHT] << 2;
	// hw_buttons |= joystick.values[ODROID_INPUT_DOWN] << 3;
	// hw_buttons |= joystick.values[ODROID_INPUT_A] << 4;
	// hw_buttons |= joystick.values[ODROID_INPUT_B] << 5;
	// hw_buttons |= joystick.values[ODROID_INPUT_SELECT] << 6;
	// hw_buttons |= joystick.values[ODROID_INPUT_START] << 7;
	// hw_buttons |= joystick.values[ODROID_INPUT_VOLUME] << 8;
	// hw_buttons |= joystick.values[ODROID_INPUT_POWER] << 9;
	return hw_buttons;
}