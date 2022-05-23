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

#if ENABLE_SOUND
#include "emulator/gb/minigb_apu.h"
#endif

#include "emulator/gb/peanut_gb.h"

typedef struct _GKGameBoyAdapter {
	struct gb_s gb;
	
	uint32_t* current_frame;
	uint8_t* rom; // Memory for ROM file.
	uint8_t* cart_ram; // Memory for save file.
	char* save_file_name;
#if ENABLE_SOUND
	SoundSource* sound_source;
#endif
	PDMenuItem* scale_menu;
	PDMenuItem* interlace_menu;

	
	float crank_previous;
	int selected_scale;
	int save_timer;

	bool clear_next_frame;
	bool interlace_enabled;
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
static void lcd_draw_line_sliced(struct gb_s* gb, const uint8_t pixels[160], const uint_fast8_t line);
static void lcd_draw_line_fitted(struct gb_s* gb, const uint8_t pixels[160], const uint_fast8_t line);
static void lcd_draw_line_doubled(struct gb_s* gb, const uint8_t pixels[160], const uint_fast8_t line);
static void lcd_draw_line_natural(struct gb_s* gb, const uint8_t pixels[160], const uint_fast8_t line);

#pragma mark -

GKGameBoyAdapter* GKGameBoyAdapterCreate(void) {
	GKGameBoyAdapter* adapter = malloc(sizeof(GKGameBoyAdapter));
	memset(adapter, 0, sizeof(GKGameBoyAdapter));

	adapter->crank_previous = playdate->system->getCrankAngle();
	adapter->clear_next_frame = true;
	adapter->selected_scale = 1;
	adapter->interlace_enabled = true;

	return adapter;
}

void GKGameBoyAdapterDestroy(GKGameBoyAdapter* adapter) {
	save(adapter);
	reset(adapter);
	free_menus(adapter);
	free(adapter);
}

bool GKGameBoyAdapterLoad(GKGameBoyAdapter* adapter, const char* path) {
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
	if((adapter->rom = GKReadFileContents(path)) == NULL) {
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
	
	// Initialize sound.
#if ENABLE_SOUND
	playdate->sound->channel->setVolume(playdate->sound->getDefaultChannel(), 0.2f);
	adapter->sound_source = playdate->sound->addSource(GKAudioSourceCallback, NULL, 1);
	audio_init();
#endif

	// Initialize display.
	void* lcd_func = NULL;
	
	if(adapter->selected_scale == 0) {
		lcd_func = lcd_draw_line_natural;
	}
	else if(adapter->selected_scale == 1) {
		lcd_func = lcd_draw_line_fitted;
	}
	else if(adapter->selected_scale == 2) {
		lcd_func = lcd_draw_line_sliced;
	}
	else if(adapter->selected_scale == 3) {
		lcd_func = lcd_draw_line_doubled;
	}
	
	gb_init_lcd(&adapter->gb, NULL);
	adapter->gb.display.lcd_draw_line = lcd_func;
	adapter->gb.direct.frame_skip = 1;
	adapter->gb.direct.joypad = 255;
	adapter->gb.direct.interlace = adapter->interlace_enabled;

	// auto_assign_palette(&priv, gb_colour_hash(&gb));
	
	add_menus(adapter);
	
	return true;
}

void GKGameBoyAdapterUpdate(GKGameBoyAdapter* adapter, unsigned int dt) {
	static unsigned int rtc_timer = 0;
	unsigned int fast_mode = 1;
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
	
	// view->updated_row_top = LCD_ROWS;
	// view->updated_row_bottom = 0;
	gb_run_frame(&adapter->gb);
	
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
	
	void* fn = NULL;
	
	if(adapter->selected_scale == 0) {
		fn = lcd_draw_line_natural;
	}
	else if(adapter->selected_scale == 1) {
		fn = lcd_draw_line_fitted;
	}
	else if(adapter->selected_scale == 2) {
		fn = lcd_draw_line_sliced;
	}
	else if(adapter->selected_scale == 3) {
		fn = lcd_draw_line_doubled;
	}
	
	if(fn != NULL) {
		adapter->gb.display.lcd_draw_line = fn;
		adapter->clear_next_frame = true;
	}
}

static void menu_item_interlace(void* context) {
	GKGameBoyAdapter* adapter = (GKGameBoyAdapter*)context;	
	adapter->interlace_enabled = playdate->system->getMenuItemValue(adapter->interlace_menu);
	adapter->gb.direct.interlace = adapter->interlace_enabled;
}

static void add_menus(GKGameBoyAdapter* adapter) {
	const char* menu_items[] = {
		"natural",
		"fitted",
		"sliced",
		"doubled"
	};
	
	adapter->scale_menu = playdate->system->addOptionsMenuItem("Scale", menu_items, 4, menu_item_scale, adapter);
	adapter->interlace_menu = playdate->system->addCheckmarkMenuItem("Interlace", 1, menu_item_interlace, adapter);
	
	playdate->system->setMenuItemValue(adapter->scale_menu, 1);
}

static void free_menus(GKGameBoyAdapter* adapter) {
	if(adapter->scale_menu != NULL) {
		playdate->system->removeMenuItem(adapter->scale_menu);
		adapter->scale_menu = NULL;
	}
	if(adapter->interlace_menu != NULL) {
		playdate->system->removeMenuItem(adapter->interlace_menu);
		adapter->interlace_menu = NULL;
	}
}

static void reset(GKGameBoyAdapter* adapter) {
#if ENABLE_SOUND
	if(adapter->sound_source != NULL) {
		playdate->sound->removeSource(adapter->sound_source);
		adapter->sound_source = NULL;
	}
#endif

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

static const int GKDisplayPatterns[4][4][4] = {
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

static void lcd_draw_line_sliced(struct gb_s* gb, const uint8_t pixels[160], const uint_fast8_t line) {
	#define INTERLACE_MULTIPLE 1.5
	const uint32_t screen_x = GKFastDiv2(400 - 240); // Where it's going.
	const uint32_t start_x = 11; // Bit to start on.
	const uint32_t start_y = GKFastDiv2(240 - (uint32_t)(LCD_HEIGHT * INTERLACE_MULTIPLE)) + (line * INTERLACE_MULTIPLE);
	const uint32_t line_mod = GKFastMod4(line);
	GKGameBoyAdapter* adapter = gb->direct.priv;
	
	if(start_y < 0 || start_y >= LCD_ROWS) {
		return;
	}
	
	uint32_t* frame = adapter->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * start_y)) + 1;
	uint32_t accumulator = swap(*frame);
	uint32_t bit;
	uint32_t last_bit = 0;
	
	for(uint32_t x = start_x; x < LCD_WIDTH + start_x; x++) {
		bit = 31 - GKFastMod32((uint32_t)(x * INTERLACE_MULTIPLE));
		if(last_bit < bit) {
			*frame = swap(accumulator);
			frame++;
			accumulator = 0x00000000;
		}
		last_bit = bit;
		GKSetOrClearBitIf(GKDisplayPatterns[GKFastMod4(pixels[x - start_x])][line_mod][GKFastMod4(x)], bit, accumulator);
	}
	*frame = swap(accumulator);
	
	playdate->graphics->markUpdatedRows(start_y, start_y);
}

static void lcd_draw_line_fitted(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line) {
	GKGameBoyAdapter* adapter = gb->direct.priv;
	
	const uint32_t screen_x = (400-(160*2))/2;
	const uint32_t start_x = GKFastMod32(screen_x);
	const uint32_t double_line = line * 2;
	
	// Screen Y is doubled then we remove every 6th line.
	uint32_t line_one_sy = double_line - floor((float)double_line / 6.0f);
	uint32_t line_two_sy = line_one_sy + 1;
	
	bool skip_line_one = (double_line % 6 == 5);
	bool skip_line_two = ((double_line + 1) % 6 == 5);
	
	uint32_t* line_one = NULL;
	uint32_t* line_two = NULL;
	uint32_t line_one_accumulator = 0x00000000, line_two_accumulator = 0x00000000;
	
	if(!skip_line_one) {
		line_one = adapter->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * line_one_sy)) + GKFastDiv32(screen_x);
		line_one_accumulator = swap(*line_one);
	}
	
	if(!skip_line_two) {
		line_two = adapter->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * line_two_sy)) + GKFastDiv32(screen_x);
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

static void lcd_draw_line_doubled(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line) {
	const uint32_t screen_x = GKFastDiv2(400-GKFastMult2(160));
	const uint32_t start_x = GKFastMod32(screen_x);
	const uint32_t line_offset = 12;
	const int start_y = GKFastMult2(line - line_offset);
	GKGameBoyAdapter* adapter = gb->direct.priv;
	
	if(start_y < 0 || start_y >= LCD_ROWS) {
		return;
	}

	uint32_t* frame = adapter->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * start_y)) + GKFastDiv32(screen_x);
	uint32_t accumulator = swap(*frame);
	uint32_t bit;
	
	for(uint32_t y = start_y; y <= start_y + 1; y++) {
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
		frame = adapter->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * (start_y + 1))) + GKFastDiv32(screen_x);
		accumulator = swap(*frame);
	}
	
	playdate->graphics->markUpdatedRows(start_y, start_y+1);
}

static void lcd_draw_line_natural(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line) {
	const uint32_t start_x = 24;
	const uint32_t start_y = line + 48;
	const uint32_t line_mod = GKFastMod4(line);
	GKGameBoyAdapter* adapter = gb->direct.priv;
	
	uint32_t* frame = adapter->current_frame + (((LCD_ROWSIZE / 4) * start_y)) + 3;
	uint32_t accumulator = 0x00000000;// swap(*frame);
	uint32_t bit;
	
	
	// Handle first 8 bits
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[0] & 3][line_mod][0], 31 - GKFastMod32(24), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[1] & 3][line_mod][1], 31 - GKFastMod32(24 + 1), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[2] & 3][line_mod][2], 31 - GKFastMod32(24 + 2), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[3] & 3][line_mod][3], 31 - GKFastMod32(24 + 3), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[4] & 3][line_mod][0], 31 - GKFastMod32(24 + 4), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[5] & 3][line_mod][1], 31 - GKFastMod32(24 + 5), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[6] & 3][line_mod][2], 31 - GKFastMod32(24 + 6), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[7] & 3][line_mod][3], 31 - GKFastMod32(24 + 7), accumulator);
	*frame = swap(accumulator);
	frame++;
	accumulator = 0x00000000;
	
	uint32_t x = 8;
	
	for(uint32_t i = 0; i < 4; i++) {
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][0], 31 - GKFastMod32(0), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][1], 31 - GKFastMod32(1), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][2], 31 - GKFastMod32(2), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][3], 31 - GKFastMod32(3), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][0], 31 - GKFastMod32(4), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][1], 31 - GKFastMod32(5), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][2], 31 - GKFastMod32(6), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][3], 31 - GKFastMod32(7), accumulator);
		
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][0], 31 - GKFastMod32(8), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][1], 31 - GKFastMod32(9), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][2], 31 - GKFastMod32(10), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][3], 31 - GKFastMod32(11), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][0], 31 - GKFastMod32(12), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][1], 31 - GKFastMod32(13), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][2], 31 - GKFastMod32(14), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][3], 31 - GKFastMod32(15), accumulator);
	
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][0], 31 - GKFastMod32(16), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][1], 31 - GKFastMod32(17), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][2], 31 - GKFastMod32(18), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][3], 31 - GKFastMod32(19), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][0], 31 - GKFastMod32(20), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][1], 31 - GKFastMod32(21), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][2], 31 - GKFastMod32(22), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][3], 31 - GKFastMod32(23), accumulator);
		
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][0], 31 - GKFastMod32(24), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][1], 31 - GKFastMod32(25), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][2], 31 - GKFastMod32(26), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][3], 31 - GKFastMod32(27), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][0], 31 - GKFastMod32(28), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][1], 31 - GKFastMod32(29), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][2], 31 - GKFastMod32(30), accumulator);
		GKSetOrClearBitIf(GKDisplayPatterns[pixels[x++] & 3][line_mod][3], 31 - GKFastMod32(31), accumulator);
	
		*frame = swap(accumulator);
		frame++;
		accumulator = 0x00000000;
	}
	
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[136] & 3][line_mod][0], 31 - GKFastMod32(0), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[137] & 3][line_mod][1], 31 - GKFastMod32(1), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[138] & 3][line_mod][2], 31 - GKFastMod32(2), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[139] & 3][line_mod][3], 31 - GKFastMod32(3), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[140] & 3][line_mod][0], 31 - GKFastMod32(4), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[141] & 3][line_mod][1], 31 - GKFastMod32(5), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[142] & 3][line_mod][2], 31 - GKFastMod32(6), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[143] & 3][line_mod][3], 31 - GKFastMod32(7), accumulator);
	
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[144] & 3][line_mod][0], 31 - GKFastMod32(8), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[145] & 3][line_mod][1], 31 - GKFastMod32(9), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[146] & 3][line_mod][2], 31 - GKFastMod32(10), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[147] & 3][line_mod][3], 31 - GKFastMod32(11), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[148] & 3][line_mod][0], 31 - GKFastMod32(12), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[149] & 3][line_mod][1], 31 - GKFastMod32(13), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[150] & 3][line_mod][2], 31 - GKFastMod32(14), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[151] & 3][line_mod][3], 31 - GKFastMod32(15), accumulator);
	
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[152] & 3][line_mod][0], 31 - GKFastMod32(16), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[153] & 3][line_mod][1], 31 - GKFastMod32(17), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[154] & 3][line_mod][2], 31 - GKFastMod32(18), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[155] & 3][line_mod][3], 31 - GKFastMod32(19), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[156] & 3][line_mod][0], 31 - GKFastMod32(20), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[157] & 3][line_mod][1], 31 - GKFastMod32(21), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[158] & 3][line_mod][2], 31 - GKFastMod32(22), accumulator);
	GKSetOrClearBitIf(GKDisplayPatterns[pixels[159] & 3][line_mod][3], 31 - GKFastMod32(23), accumulator);
	*frame = swap(accumulator);
	
	playdate->graphics->markUpdatedRows(start_y, start_y);
}