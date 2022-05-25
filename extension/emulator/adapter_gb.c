#include "adapter_gb.h"
#include "common.h"
#include "lib/utility.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define ENABLE_SOUND 1
#define ENABLE_LCD 1
#define PEANUT_GB_HIGH_LCD_ACCURACY 0

#include "emulator/gb/minigb_apu.h"
#include "emulator/gb/peanut_gb.h"

typedef struct _GKGameBoyAdapter {
	struct gb_s gb;
	
	uint32_t* current_frame;
	uint8_t* rom; // Memory for ROM file.
	uint8_t* cart_ram; // Memory for save file.
	char* save_file_name;
	SoundSource* sound_source;
	PDMenuItem* scale_menu;
	PDMenuItem* sound_menu;
	
	float crank_previous;
	int selected_scale;
	int save_timer;

	bool clear_next_frame;
} GKGameBoyAdapter;

#pragma mark -

static void update_joypad(GKGameBoyAdapter* adapter);
static void update_crank(GKGameBoyAdapter* adapter);
static void add_menus(GKGameBoyAdapter* adapter);
static void free_menus(GKGameBoyAdapter* adapter);
static void reset(GKGameBoyAdapter* adapter);
static void save(GKGameBoyAdapter* adapter);
static void load_save(const char* save_file_name, uint8_t** dest, const size_t len);
static uint8_t read_rom_byte(struct gb_s* gb, const uint_fast32_t addr);
static uint8_t read_ram_byte(struct gb_s* gb, const uint_fast32_t addr);
static void write_ram_byte(struct gb_s* gb, const uint_fast32_t addr, const uint8_t val);
static void error(struct gb_s* gb, const enum gb_error_e gb_err, const uint16_t val);
static void update_display_natural(GKGameBoyAdapter* adapter);
static void update_display_doubled(GKGameBoyAdapter* adapter);
static void update_display_fitted(GKGameBoyAdapter* adapter);

#pragma mark -

GKGameBoyAdapter* GKGameBoyAdapterCreate(void) {
	GKGameBoyAdapter* adapter = malloc(sizeof(GKGameBoyAdapter));
	memset(adapter, 0, sizeof(GKGameBoyAdapter));

	adapter->crank_previous = playdate->system->getCrankAngle();
	adapter->clear_next_frame = true;
	adapter->selected_scale = 1;

	return adapter;
}

void GKGameBoyAdapterDestroy(GKGameBoyAdapter* adapter) {
	save(adapter);
	reset(adapter);
	
	playdate->sound->removeSource(adapter->sound_source);
	adapter->sound_source = NULL;
	adapter->gb.direct.sound_enabled = 0;
	
	free_menus(adapter);
	free(adapter);
}

bool GKGameBoyAdapterLoad(GKGameBoyAdapter* adapter, const char* path) {
	memset(&adapter->gb, 0, sizeof(struct gb_s));
	adapter->clear_next_frame = true;
	
	enum gb_init_error_e gb_ret;
	int filename_length = 0;
	const char* filename = GKGetFilename(path, &filename_length);
	// const char rom_file_name[] = "/saves/";
	const char save_dir[] = "/saves/";
	
	// Save and free resources if we have loaded a game previously.
	save(adapter);
	reset(adapter);
	
	// Read ROM into memory.
	if((adapter->rom = GKReadFileContents(path, NULL)) == NULL) {
		GKLog("RAM READ ERROR %d\n", __LINE__);
		return false;
	}
	
	/* If no save file is specified, copy save file (with specific name) to
	 * allocated memory. */
	{
		char* str_replace;
		const char extension[] = ".sav";
	
		/* Allocate enough space for the ROM file name, for the "sav"
		 * extension and for the null terminator. */
		int str_len = strlen(save_dir) + filename_length + strlen(extension) + 1;
		adapter->save_file_name = malloc(str_len);
		memset(adapter->save_file_name, 0, str_len);
	
		if(adapter->save_file_name == NULL) {
			reset(adapter);
			return false;
		}
	
		// Construct save file path: /saves/<filename>.sav
		strcpy(adapter->save_file_name, save_dir);
		strcpy(adapter->save_file_name + strlen(save_dir), filename);
		strcpy(adapter->save_file_name + strlen(save_dir) + filename_length, extension);
	}
	
	gb_ret = gb_init(&adapter->gb, &read_rom_byte, &read_ram_byte, &write_ram_byte, &error, adapter);
	switch(gb_ret) {
	case GB_INIT_NO_ERROR:
		break;
	
	case GB_INIT_CARTRIDGE_UNSUPPORTED:
		GKLog("Unsupported cartridge.");
		return false;
	
	case GB_INIT_INVALID_CHECKSUM:
		GKLog("Invalid ROM: Checksum failure.");
		return false;
	
	default:
		GKLog("Unknown error: %d\n", gb_ret);
		return false;
	}
	
	// Load save file.
	load_save(adapter->save_file_name, &adapter->cart_ram, gb_get_save_size(&adapter->gb));

	// Initialize display.
	gb_init_lcd(&adapter->gb, NULL);
	adapter->gb.direct.frame_skip = 1;
	adapter->gb.direct.joypad = 255;
	
	// Initialize sound.
	if(GKAppGetSoundEnabled()) {
		audio_init();
		playdate->sound->channel->setVolume(playdate->sound->getDefaultChannel(), 0.2f);
		adapter->sound_source = playdate->sound->addSource(GKAudioSourceCallback, NULL, 1);
		adapter->gb.direct.sound_enabled = 1;
	}

	// auto_assign_palette(&priv, gb_colour_hash(&gb));
	
	add_menus(adapter);
	
	return true;
}

void GKGameBoyAdapterUpdate(GKGameBoyAdapter* adapter, unsigned int dt) {
	static unsigned int rtc_timer = 0;
	unsigned int fast_mode = 1;
	bool force_update = adapter->clear_next_frame;
	// const double target_speed_ms = 1000.0 / (VERTICAL_SYNC);
	// double speed_compensation = 0.0;
	
	// Simply return if system unavailable.
	if(adapter->clear_next_frame) {
		playdate->graphics->clear(kColorBlack);
		adapter->clear_next_frame = false;
	}
	
	playdate->graphics->setDrawMode(kDrawModeCopy);
	adapter->current_frame = (uint32_t*)playdate->graphics->getFrame();
	
	update_joypad(adapter);
	update_crank(adapter);
	
	gb_run_frame(&adapter->gb);
	
	if(force_update) {
		memset(adapter->gb.display.changed_rows, 1, sizeof(adapter->gb.display.changed_rows));
		adapter->gb.display.changed_row_count = LCD_HEIGHT;
	}
	
	if(force_update || (adapter->gb.display.changed_row_count > 0 && (!adapter->gb.direct.frame_skip || !adapter->gb.display.frame_skip_count))) {
		if(adapter->selected_scale == 0) {
			update_display_natural(adapter);
		}
		else if(adapter->selected_scale == 1) {
			update_display_fitted(adapter);
		}
		else if(adapter->selected_scale == 2) {
			update_display_doubled(adapter);
		}
	}
	
	// Tick the internal RTC every 1 second.
	rtc_timer += dt;// target_speed_ms / fast_mode;
	if(rtc_timer >= 1000) {
		rtc_timer -= 1000;
		gb_tick_rtc(&adapter->gb);
	}
	
	// Save RAM to disk every 3 seconds.
	adapter->save_timer += dt;
	if(adapter->save_timer >= 3000) {
		adapter->save_timer -= 3000;
		save(adapter);
	}
}

#pragma mark -

static void update_joypad(GKGameBoyAdapter* adapter) {
	PDButtons buttons;
	
	// Get current state of Playdate buttons.
	playdate->system->getButtonState(&buttons, NULL, NULL);
	
	// Map Playdate button states to Gamekid's.
	adapter->gb.direct.joypad_bits.b = (buttons & kButtonB) == 0;
	adapter->gb.direct.joypad_bits.a = (buttons & kButtonA) == 0;
	
	adapter->gb.direct.joypad_bits.up = (buttons & kButtonUp) == 0;
	adapter->gb.direct.joypad_bits.down = (buttons & kButtonDown) == 0;
	adapter->gb.direct.joypad_bits.left = (buttons & kButtonLeft) == 0;
	adapter->gb.direct.joypad_bits.right = (buttons & kButtonRight) == 0;
}

static void update_crank(GKGameBoyAdapter* adapter) {
	float angle = playdate->system->getCrankAngle();
	int direction = 0; // 0 = not moving, 1 = clockwise, -1 = counter clockwise
	
	if(angle < (adapter->crank_previous - 3)) {
		direction = -1;
	}
	else if(angle > (adapter->crank_previous + 3)) {
		direction = 1;
	}
	else {
		adapter->crank_previous = angle;
		return;
	}
	
	adapter->gb.direct.joypad_bits.select = (direction == 1);
	adapter->gb.direct.joypad_bits.start = (direction == -1);
	
	adapter->crank_previous = angle;
}

#pragma mark -

static void menu_item_scale(void* context) {
	GKGameBoyAdapter* adapter = (GKGameBoyAdapter*)context;
	
	adapter->selected_scale = playdate->system->getMenuItemValue(adapter->scale_menu);
	adapter->clear_next_frame = true;
}

static void add_menus(GKGameBoyAdapter* adapter) {
	const char* menu_items[] = {
		"natural",
		"fitted",
		"doubled"
	};
	
	adapter->scale_menu = playdate->system->addOptionsMenuItem("Scale", menu_items, 3, menu_item_scale, adapter);
	
	playdate->system->setMenuItemValue(adapter->scale_menu, adapter->selected_scale);
}

static void free_menus(GKGameBoyAdapter* adapter) {
	if(adapter->scale_menu != NULL) {
		playdate->system->removeMenuItem(adapter->scale_menu);
		adapter->scale_menu = NULL;
	}
	if(adapter->sound_menu != NULL) {
		playdate->system->removeMenuItem(adapter->sound_menu);
		adapter->sound_menu = NULL;
	}
}

static void reset(GKGameBoyAdapter* adapter) {
	if(adapter->sound_source != NULL) {
		playdate->sound->removeSource(adapter->sound_source);
		adapter->sound_source = NULL;
	}

	if(adapter->cart_ram != NULL) {
		free(adapter->cart_ram);
		adapter->cart_ram = NULL;
	}
	
	if(adapter->rom != NULL) {
		free(adapter->rom);
		adapter->rom = NULL;
	}
	
	if(adapter->save_file_name != NULL) {
		free(adapter->save_file_name);
		adapter->save_file_name = NULL;
	}
	
	gb_reset(&adapter->gb);
	
	adapter->crank_previous = playdate->system->getCrankAngle();
	adapter->clear_next_frame = true;
	adapter->save_timer = 0;
}

static void save(GKGameBoyAdapter* adapter) {
	if(adapter->cart_ram == NULL && adapter->save_file_name == NULL) {
		return;
	}
	
	int ram_length = gb_get_save_size(&adapter->gb);
	
	// Make sure there is something to save.
	if(ram_length == 0) {
		return;
	}
	
	GKFile* f;
	if((f = GKFileOpen(adapter->save_file_name, kGKFileWrite)) == NULL) {
		GKLog("Gamekid: Unable to open save file at %s.", adapter->save_file_name);
		return;
	}
	GKFileWrite(adapter->cart_ram, ram_length, f);
	GKFileClose(f);
}

static void load_save(const char* save_file_name, uint8_t** dest, const size_t len) {
	GKFile* f;
	
	/* If save file not required. */
	if(len == 0) {
		*dest = NULL;
		return;
	}
	
	/* Allocate enough memory to hold save file. */
	if((*dest = malloc(len)) == NULL) {
		GKLog("%d\n", __LINE__);
		return;
		// exit(EXIT_FAILURE);
	}
	
	f = GKFileOpen(save_file_name, kGKFileReadData);
	
	/* It doesn't matter if the save file doesn't exist. We initialise the
	* save memory allocated above. The save file will be created on exit. */
	if(f == NULL) {
		GKLog("Failed to open save file");
		memset(*dest, 0xFF, len);
		return;
	}
	
	/* Read save file to allocated memory. */
	GKFileRead(*dest, len, f);
	GKFileClose(f);
}

#pragma mark -

static uint8_t read_rom_byte(struct gb_s* gb, const uint_fast32_t addr) {
	const GKGameBoyAdapter* const adapter = gb->direct.priv;
	return adapter->rom[addr];
}

static uint8_t read_ram_byte(struct gb_s* gb, const uint_fast32_t addr) {
	const GKGameBoyAdapter* const adapter = gb->direct.priv;
	return adapter->cart_ram[addr];
}

static void write_ram_byte(struct gb_s* gb, const uint_fast32_t addr, const uint8_t val) {
	const GKGameBoyAdapter* const adapter = gb->direct.priv;
	adapter->cart_ram[addr] = val;
}

static void error(struct gb_s* gb, const enum gb_error_e gb_err, const uint16_t val) {
	const GKGameBoyAdapter* const adapter = gb->direct.priv;

	switch(gb_err) {
	case GB_INVALID_OPCODE:
		GKLog("Gamekid: Invalid opcode %#04x at PC: %#06x, SP: %#06x", val, gb->cpu_reg.pc - 1, gb->cpu_reg.sp);
		break;

	// Ignoring non fatal errors.
	case GB_INVALID_WRITE:
	case GB_INVALID_READ:
		return;

	default:
		GKLog("Gamekid: Unknown error");
		break;
	}
	
	GKAppGoToLibrary(app);
}

#pragma mark -
#pragma mark Rendering

static const uint32_t GKDisplayPatterns[4][4][4] = {
	{ // White
		{1, 1, 1, 1},
		{1, 1, 1, 1},
		{1, 1, 1, 1},
		{1, 1, 1, 1}
	},
	{ // Light Grey
		{0, 1, 0, 1},
		{1, 1, 1, 1},
		{0, 1, 0, 1},
		{1, 1, 1, 1}
	},
	{ // Dark Grey
		{1, 0, 1, 0},
		{0, 1, 0, 1},
		{1, 0, 1, 0},
		{0, 1, 0, 1}
	},
	{ // Black
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	}
};

static inline uint32_t swap(uint32_t n) {
#if TARGET_PLAYDATE
		uint32_t result;
		__asm volatile ("rev %0, %1" : "=l" (result) : "l" (n));
		return(result);
#else
		return ((n & 0xff000000) >> 24) | ((n & 0xff0000) >> 8) | ((n & 0xff00) << 8) | (n << 24);
#endif
}

static void update_display_natural(GKGameBoyAdapter* adapter) {
	const uint32_t start_x = 24;
	const uint32_t start_y = 48;
	
	uint32_t* display_frame = (uint32_t*)playdate->graphics->getFrame();
	uint32_t* frame = NULL;
	
	for(uint32_t line = 0; line < LCD_HEIGHT; line++) {
		if(adapter->gb.display.changed_rows[line] == 0) {
			continue;
		}
		
		frame = adapter->current_frame + (((LCD_ROWSIZE / 4) * (start_y + line))) + 3;
		const uint8_t* const pixels = !adapter->gb.display.back_fb_enabled ? adapter->gb.display.back_fb[line] : adapter->gb.display.front_fb[line];
	
		uint32_t accumulator = 0x00000000;
		const uint32_t line_mod = GKFastMod4(line);
		
		// Handle first 8 bits of row.
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[0] & 0x3][line_mod][0], 7, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[1] & 0x3][line_mod][1], 6, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[2] & 0x3][line_mod][2], 5, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[3] & 0x3][line_mod][3], 4, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[4] & 0x3][line_mod][0], 3, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[5] & 0x3][line_mod][1], 2, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[6] & 0x3][line_mod][2], 1, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[7] & 0x3][line_mod][3], 0, accumulator);
		*frame = swap(accumulator);
		frame++;
		accumulator = 0x00000000;
		
		uint32_t x = 8;
		
		for(uint32_t i = 0; i < 4; i++) {
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][0], 31, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][1], 30, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][2], 29, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][3], 28, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][0], 27, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][1], 26, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][2], 25, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][3], 24, accumulator);
			
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][0], 23, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][1], 22, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][2], 21, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][3], 20, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][0], 19, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][1], 18, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][2], 17, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][3], 16, accumulator);
		
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][0], 15, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][1], 14, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][2], 13, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][3], 12, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][0], 11, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][1], 10, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][2], 9, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][3], 8, accumulator);
			
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][0], 7, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][1], 6, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][2], 5, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][3], 4, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][0], 3, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][1], 2, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][2], 1, accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 0x3][line_mod][3], 0, accumulator);
		
			*frame = swap(accumulator);
			frame++;
			accumulator = 0x00000000;
		}
		
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[136] & 0x3][line_mod][0], 31, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[137] & 0x3][line_mod][1], 30, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[138] & 0x3][line_mod][2], 29, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[139] & 0x3][line_mod][3], 28, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[140] & 0x3][line_mod][0], 27, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[141] & 0x3][line_mod][1], 26, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[142] & 0x3][line_mod][2], 25, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[143] & 0x3][line_mod][3], 24, accumulator);
		
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[144] & 0x3][line_mod][0], 23, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[145] & 0x3][line_mod][1], 22, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[146] & 0x3][line_mod][2], 21, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[147] & 0x3][line_mod][3], 20, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[148] & 0x3][line_mod][0], 19, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[149] & 0x3][line_mod][1], 18, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[150] & 0x3][line_mod][2], 17, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[151] & 0x3][line_mod][3], 16, accumulator);
		
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[152] & 0x3][line_mod][0], 15, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[153] & 0x3][line_mod][1], 14, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[154] & 0x3][line_mod][2], 13, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[155] & 0x3][line_mod][3], 12, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[156] & 0x3][line_mod][0], 11, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[157] & 0x3][line_mod][1], 10, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[158] & 0x3][line_mod][2], 9, accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[159] & 0x3][line_mod][3], 8, accumulator);
		*frame = swap(accumulator);
		
		playdate->graphics->markUpdatedRows(start_y + line, start_y + line);
	}
}

static void update_display_doubled(GKGameBoyAdapter* adapter) {
	const uint32_t screen_x = GKFastDiv2(LCD_COLUMNS-GKFastMult2(LCD_WIDTH));
	const uint32_t start_x = GKFastMod32(screen_x);
	uint32_t screen_y;
	
	uint32_t* display_frame = (uint32_t*)playdate->graphics->getFrame();
	uint32_t* frame = NULL;
	uint32_t accumulator;
	uint32_t bit;
	
	for(uint32_t line = 12; line < LCD_HEIGHT - 12; line++) {
		if(adapter->gb.display.changed_rows[line] == 0) {
			continue;
		}
		
		screen_y = GKFastMult2(line - 12);
		if((screen_y + 1) >= LCD_ROWS) {
			break;
		}
		
		const uint8_t* const pixels = !adapter->gb.display.back_fb_enabled ? adapter->gb.display.back_fb[line] : adapter->gb.display.front_fb[line];
		
		frame = display_frame + ((GKFastDiv4(LCD_ROWSIZE) * screen_y)) + GKFastDiv32(screen_x);
		accumulator = swap(*frame);
		bit = 0;
		for(uint32_t y = screen_y; y <= GKMin(screen_y + 1, LCD_ROWS); y++) {
			for(uint32_t x = start_x; x < (LCD_WIDTH * 2) + start_x; x++) {
				bit = 31 - GKFastMod32(x);
				GKSetOrClearBitIf(GKDisplayPatterns[pixels[GKFastDiv2(x - start_x)] & 3][GKFastMod4(y)][GKFastMod4(x)], bit, accumulator);
				if(bit == 0) {
					*frame = swap(accumulator);
					frame++;
					accumulator = 0x00000000;
				}
			}
			*frame = swap(accumulator);
			frame = display_frame + ((GKFastDiv4(LCD_ROWSIZE) * (screen_y + 1))) + GKFastDiv32(screen_x);
			accumulator = swap(*frame);
		}
		
		playdate->graphics->markUpdatedRows(screen_y, screen_y+1);
	}
}


static void update_display_fitted(GKGameBoyAdapter* adapter) {
	const uint32_t screen_x = GKFastDiv2(LCD_COLUMNS-GKFastMult2(LCD_WIDTH));
	const uint32_t start_x = GKFastMod32(screen_x);
	uint32_t* display_frame = (uint32_t*)playdate->graphics->getFrame();
	
	for(uint32_t line = 0; line < LCD_HEIGHT; line++) {
		if(adapter->gb.display.changed_rows[line] == 0) {
			continue;
		}
		
		const uint8_t* const pixels = !adapter->gb.display.back_fb_enabled ? adapter->gb.display.back_fb[line] : adapter->gb.display.front_fb[line];
		
		const uint32_t double_line = GKFastMult2(line);
		
		// Screen Y is doubled then we remove every 6th line.
		uint32_t line_one_sy = double_line - floor((float)double_line / 6.0f);
		uint32_t line_two_sy = line_one_sy + 1;
		
		bool skip_line_one = (double_line % 6 == 5);
		bool skip_line_two = ((double_line + 1) % 6 == 5);
		
		uint32_t* line_one = NULL;
		uint32_t* line_two = NULL;
		uint32_t line_one_accumulator = 0x00000000, line_two_accumulator = 0x00000000;
		
		if(!skip_line_one) {
			line_one = display_frame + ((GKFastDiv4(LCD_ROWSIZE) * line_one_sy)) + GKFastDiv32(screen_x);
			line_one_accumulator = swap(*line_one);
		}
		
		if(!skip_line_two) {
			line_two = display_frame + ((GKFastDiv4(LCD_ROWSIZE) * line_two_sy)) + GKFastDiv32(screen_x);
			line_two_accumulator = swap(*line_two);
		}
		
		const uint32_t line_one_mod = GKFastMod4(line_one_sy);
		const uint32_t line_two_mod = GKFastMod4(line_two_sy);
		
		uint32_t bit;
		for(uint32_t x = start_x; x < GKFastMult2(LCD_WIDTH) + start_x; x++) {
			bit = 31 - GKFastMod32(x);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[GKFastDiv2(x - start_x)] & 3][line_one_mod][GKFastMod4(x)], bit, line_one_accumulator);
			GKSetOrClearBitIf(GKDisplayPatterns[pixels[GKFastDiv2(x - start_x)] & 3][line_two_mod][GKFastMod4(x)], bit, line_two_accumulator);
			if(bit == 0) {
				if(line_one != NULL) {
					*line_one = swap(line_one_accumulator);
					line_one++;
					line_one_accumulator = 0x00000000;
				}
				if(line_two != NULL) {
					*line_two = swap(line_two_accumulator);
					line_two++;
					line_two_accumulator = 0x00000000;
				}
			}
		}
		if(line_one != NULL) {
			*line_one = swap(line_one_accumulator);
		}
		if(line_two != NULL) {
			*line_two = swap(line_two_accumulator);
		}
		
		playdate->graphics->markUpdatedRows(line_one_sy, line_one_sy+1);
	}
	
}