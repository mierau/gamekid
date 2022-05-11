// gameview.c
// Gamekid by Dustin Mierau

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "app.h"
#include "gameview.h"
#include "pd_api.h"
#include "common.h"

#define ENABLE_SOUND 0
#define ENABLE_LCD 1
#define PEANUT_GB_HIGH_LCD_ACCURACY 0
#include "peanut_gb.h"

// Emulator callbacks
static uint8_t* read_rom_to_ram(const char* file_name);
static void read_cart_ram_file(const char *save_file_name, uint8_t** dest, const size_t len);
static void write_cart_ram_file(const char* save_file_name, uint8_t* src, const size_t len);
static uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr);
static uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr);
static void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val);
static void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t val);

// Rendering methods
static void fitted_lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line);
static void natural_lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line);
static void double_lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line);
static void sliced_lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line);

static inline uint32_t swap(uint32_t n) {
#if TARGET_PLAYDATE
		uint32_t result;
		__asm volatile ("rev %0, %1" : "=l" (result) : "l" (n));
		return(result);
#else
		return ((n & 0xff000000) >> 24) | ((n & 0xff0000) >> 8) | ((n & 0xff00) << 8) | (n << 24);
#endif
}

const char* GKGetFilename(const char* path, int* len) {
	char* base = strrchr(path, '/');
	path = base ? base + 1 : path;
	
	char* dot = strrchr(path, '.');
	if(dot != NULL) {
		*len = (dot - 1) - path;
	} else {
		*len = strlen(path);
	}
	
	return path;
}

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

typedef struct _GKGameView {
	GKApp* app;
	
	struct gb_s gb;
	float crank_previous;
	bool renderedFrame;
	bool clear_next_frame;
	int selected_scale;
	bool interlace_enabled;
	uint32_t* current_frame;
	
	char* save_file_name;
	
	/* Pointer to allocated memory holding GB file. */
	uint8_t* rom;
	
	/* Pointer to allocated memory holding save file. */
	uint8_t* cart_ram;
	
	int save_timer;
	
	uint32_t updated_row_top;
	uint32_t updated_row_bottom;
} GKGameView;

static void updateJoypad(GKGameView* view);
static void updateCrank(GKGameView* view);

GKGameView* GKGameViewCreate(GKApp* app) {
	GKGameView* view = malloc(sizeof(GKGameView));
	memset(view, 0, sizeof(GKGameView));
	view->app = app;
	view->crank_previous = playdate->system->getCrankAngle();
	view->clear_next_frame = true;
	view->selected_scale = 1;
	view->interlace_enabled = true;
	view->rom = NULL;
	view->cart_ram = NULL;
	view->save_file_name = NULL;
	return view;
}

void GKGameViewDestroy(GKGameView* view) {
	GKGameViewReset(view);
	free(view);
}

#pragma mark -

void GKGameViewReset(GKGameView* view) {
	if(view->cart_ram != NULL) {
		free(view->cart_ram);
		view->cart_ram = NULL;
	}
	
	if(view->rom != NULL) {
		free(view->rom);
		view->rom = NULL;
	}
	
	if(view->save_file_name != NULL) {
		free(view->save_file_name);
		view->save_file_name = NULL;
	}
	
	view->crank_previous = playdate->system->getCrankAngle();
	view->clear_next_frame = true;
	view->save_timer = 0;
}

void GKGameViewSave(GKGameView* view) {
	if(view->cart_ram == NULL && view->save_file_name == NULL) {
		return;
	}
	
	write_cart_ram_file(view->save_file_name, view->cart_ram, gb_get_save_size(&view->gb));
}

bool GKGameViewShow(GKGameView* view, const char* path) {
	view->clear_next_frame = true;
	
	enum gb_init_error_e gb_ret;
	int filename_length = 0;
	const char* filename = GKGetFilename(path, &filename_length);
	// const char rom_file_name[] = "/saves/";
	const char save_dir[] = "/saves/";
	
	// Save and free resources if we have loaded a game previously.
	GKGameViewSave(view);
	GKGameViewReset(view);
	
	// Read ROM into memory.
	if((view->rom = read_rom_to_ram(path)) == NULL) {
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
		view->save_file_name = malloc(str_len);
		memset(view->save_file_name, 0, str_len);
	
		if(view->save_file_name == NULL) {
			GKGameViewReset(view);
			return false;
		}
	
		// Construct save file path: /saves/<filename>.sav
		strcpy(view->save_file_name, save_dir);
		strcpy(view->save_file_name + strlen(save_dir), filename);
		strcpy(view->save_file_name + strlen(save_dir) + filename_length, extension);
	}
	
	gb_ret = gb_init(&view->gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write, &gb_error, view);
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
	
	/* Load Save File. */
	read_cart_ram_file(view->save_file_name, &view->cart_ram, gb_get_save_size(&view->gb));
	
	void* lcd_func = NULL;
	
	if(view->selected_scale == 0) {
		lcd_func = natural_lcd_draw_line;
	}
	else if(view->selected_scale == 1) {
		lcd_func = fitted_lcd_draw_line;
	}
	else if(view->selected_scale == 2) {
		lcd_func = sliced_lcd_draw_line;
	}
	else if(view->selected_scale == 3) {
		lcd_func = double_lcd_draw_line;
	}
	
	gb_init_lcd(&view->gb, NULL);
	view->gb.display.lcd_draw_line = lcd_func;
	view->gb.direct.frame_skip = 1;
	view->gb.direct.interlace = view->interlace_enabled;
	
	// auto_assign_palette(&priv, gb_colour_hash(&gb));
	
	return true;
}

void GKGameViewUpdate(GKGameView* view, unsigned int dt) {
	static unsigned int rtc_timer = 0;
	unsigned int fast_mode = 1;
	// const double target_speed_ms = 1000.0 / (VERTICAL_SYNC);
	// double speed_compensation = 0.0;
	
	// Simply return if system unavailable.
	if(view->clear_next_frame) {
		playdate->graphics->clear(kColorBlack);
		view->clear_next_frame = false;
	}
	
	playdate->graphics->setDrawMode(kDrawModeCopy);
	view->current_frame = (uint32_t*)playdate->graphics->getFrame();
	
	updateJoypad(view);
	updateCrank(view);
	
	// view->updated_row_top = LCD_ROWS;
	// view->updated_row_bottom = 0;
	gb_run_frame(&view->gb);
	
	// if(view->updated_row_top <= view->updated_row_bottom) {
	// 	playdate->graphics->markUpdatedRows(view->updated_row_top, view->updated_row_bottom);
	// }
	
	// Tick the internal RTC every 1 second.
	rtc_timer += dt;// target_speed_ms / fast_mode;
	if(rtc_timer >= 1000) {
		rtc_timer = 0;
		gb_tick_rtc(&view->gb);
	}
	
	// Save RAM to disk every 3 seconds.
	view->save_timer += dt;
	if(view->save_timer >= 3000) {
		view->save_timer = 0;
		GKGameViewSave(view);
	}
	
// #if TARGET_PLAYDATE
	// playdate->system->drawFPS(5, 5);
// #endif
}

#pragma mark -

static void write_cart_ram_file(const char* save_file_name, uint8_t* src, const size_t len) {
	GKFile* f;
	
	if(len == 0 || src == NULL) {
		GKLog("FAIL TO SAVE FILE %i %p", len, src);
		return;
	}
	
	if((f = GKFileOpen(save_file_name, kGKFileWrite)) == NULL) {
		GKLog("Unable to open save file.");
		// GKLog("%d\n", __LINE__);
		// exit(EXIT_FAILURE);
		return;
	}
	
	/* Record save file. */
	GKFileWrite(src, len, f);
	GKFileClose(f);
}

static uint8_t* read_rom_to_ram(const char* file_name) {
	GKFile* rom_file = GKFileOpen(file_name, kGKFileReadData);
	size_t rom_size;
	uint8_t* rom = NULL;

	if(rom_file == NULL)
		return NULL;

	GKFileSeek(rom_file, 0, kGKSeekEnd);
	rom_size = GKFileTell(rom_file);
	GKFileSeek(rom_file, 0, kGKSeekSet);
	rom = malloc(rom_size);

	if(GKFileRead(rom, rom_size, rom_file) != rom_size) {
		free(rom);
		GKFileClose(rom_file);
		return NULL;
	}

	GKFileClose(rom_file);
	return rom;
}

static void read_cart_ram_file(const char* save_file_name, uint8_t** dest, const size_t len) {
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

static uint8_t gb_rom_read(struct gb_s* gb, const uint_fast32_t addr) {
	const GKGameView* const p = gb->direct.priv;
	return p->rom[addr];
}

static uint8_t gb_cart_ram_read(struct gb_s* gb, const uint_fast32_t addr) {
	const GKGameView* const p = gb->direct.priv;
	return p->cart_ram[addr];
}

static void gb_cart_ram_write(struct gb_s* gb, const uint_fast32_t addr, const uint8_t val) {
	const GKGameView* const p = gb->direct.priv;
	p->cart_ram[addr] = val;
}

static void gb_error(struct gb_s* gb, const enum gb_error_e gb_err, const uint16_t val) {
	const GKGameView* const p = gb->direct.priv;

// 	switch(gb_err) {
// 	case GB_INVALID_OPCODE:
// 		/* We compensate for the post-increment in the __gb_step_cpu
// 		 * function. */
// 		fprintf(stdout, "Invalid opcode %#04x at PC: %#06x, SP: %#06x\n",
// 			val,
// 			gb->cpu_reg.pc - 1,
// 			gb->cpu_reg.sp);
// 		break;
// 
// 	/* Ignoring non fatal errors. */
// 	case GB_INVALID_WRITE:
// 	case GB_INVALID_READ:
// 		return;
// 
// 	default:
// 		printf("Unknown error");
// 		break;
// 	}
}
 
static void sliced_lcd_draw_line(struct gb_s* gb, const uint8_t pixels[160], const uint_fast8_t line) {
	#define INTERLACE_MULTIPLE 1.5
	const uint32_t screen_x = GKFastDiv2(400 - 240); // Where it's going.
	const uint32_t start_x = 11; // Bit to start on.
	const uint32_t start_y = GKFastDiv2(240 - (uint32_t)(LCD_HEIGHT * INTERLACE_MULTIPLE)) + (line * INTERLACE_MULTIPLE);
	const uint32_t line_mod = GKFastMod4(line);
	GKGameView* view = gb->direct.priv;
	
	if(start_y < 0 || start_y >= LCD_ROWS) {
		return;
	}
	
	uint32_t* frame = view->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * start_y)) + 1;
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
	
	// view->updated_row_top = GKMin(start_y, view->updated_row_top);
	// view->updated_row_bottom = GKMax(start_y, view->updated_row_bottom);
	playdate->graphics->markUpdatedRows(start_y, start_y);
}

static void fitted_lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line) {
	GKGameView* view = gb->direct.priv;
	
	const uint32_t screen_x = (400-(160*2))/2;
	const uint32_t start_x = GKFastMod32(screen_x);
	const uint32_t double_line = line * 2;
	
	// Screen Y is doubled then we remove every 6th line.
	uint32_t line_one_sy = double_line - floor((float)double_line / 6.0);
	uint32_t line_two_sy = line_one_sy + 1;
	
	bool skip_line_one = (double_line % 6 == 5);
	bool skip_line_two = ((double_line + 1) % 6 == 5);
	
	uint32_t* line_one = NULL;
	uint32_t* line_two = NULL;
	uint32_t line_one_accumulator = 0x00000000, line_two_accumulator = 0x00000000;
	
	if(!skip_line_one) {
		line_one = view->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * line_one_sy)) + GKFastDiv32(screen_x);
		line_one_accumulator = swap(*line_one);
	}
	
	if(!skip_line_two) {
		line_two = view->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * line_two_sy)) + GKFastDiv32(screen_x);
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

static void double_lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line) {
	const uint32_t screen_x = GKFastDiv2(400-GKFastMult2(160));
	const uint32_t start_x = GKFastMod32(screen_x);
	const uint32_t line_offset = 12;
	const int start_y = GKFastMult2(line - line_offset);
	GKGameView* view = gb->direct.priv;
	
	if(start_y < 0 || start_y >= LCD_ROWS) {
		return;
	}

	uint32_t* frame = view->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * start_y)) + GKFastDiv32(screen_x);
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
		frame = view->current_frame + ((GKFastDiv4(LCD_ROWSIZE) * (start_y + 1))) + GKFastDiv32(screen_x);
		accumulator = swap(*frame);
	}
	
	// view->updated_row_top = GKMin(start_y, view->updated_row_top);
	// view->updated_row_bottom = GKMax(start_y+1, view->updated_row_bottom);
	playdate->graphics->markUpdatedRows(start_y, start_y+1);
}

static void natural_lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line) {
	const uint32_t start_x = 24;
	const uint32_t start_y = line + 48;
	const uint32_t line_mod = GKFastMod4(line);
	GKGameView* view = gb->direct.priv;
	
	uint32_t* frame = view->current_frame + (((LCD_ROWSIZE / 4) * start_y)) + 3;
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
	
	// view->updated_row_top = GKMin(start_y, view->updated_row_top);
	// view->updated_row_bottom = GKMax(start_y, view->updated_row_bottom);
	playdate->graphics->markUpdatedRows(start_y, start_y);
}

#pragma mark -

static void updateJoypad(GKGameView* view) {
	PDButtons buttons;
	
	// Get current state of Playdate buttons.
	playdate->system->getButtonState(&buttons, NULL, NULL);
	
	// Map Playdate button states to Gamekid's.
	view->gb.direct.joypad_bits.b = (buttons & kButtonB) == 0;
	view->gb.direct.joypad_bits.a = (buttons & kButtonA) == 0;
	
	view->gb.direct.joypad_bits.up = (buttons & kButtonUp) == 0;
	view->gb.direct.joypad_bits.down = (buttons & kButtonDown) == 0;
	view->gb.direct.joypad_bits.left = (buttons & kButtonLeft) == 0;
	view->gb.direct.joypad_bits.right = (buttons & kButtonRight) == 0;
}

static void updateCrank(GKGameView* view) {
	float angle = playdate->system->getCrankAngle();
	int direction = 0; // 0 = not moving, 1 = clockwise, -1 = counter clockwise
	
	if(angle < (view->crank_previous - 3)) {
		direction = -1;
	}
	else if(angle > (view->crank_previous + 3)) {
		direction = 1;
	}
	else {
		view->crank_previous = angle;
		return;
	}
	
	view->gb.direct.joypad_bits.select = (direction == 1);
	view->gb.direct.joypad_bits.start = (direction == -1);
	
	view->crank_previous = angle;
}

void GKGameViewSetScale(GKGameView* view, int scale) {
	view->selected_scale = scale;

	void* fn = NULL;
	
	if(scale == 0) {
		fn = natural_lcd_draw_line;
	}
	else if(scale == 1) {
		fn = fitted_lcd_draw_line;
	}
	else if(scale == 2) {
		fn = sliced_lcd_draw_line;
	}
	else if(scale == 3) {
		fn = double_lcd_draw_line;
	}
	
	if(fn != NULL) {
		view->gb.display.lcd_draw_line = fn;
		view->clear_next_frame = true;
	}
}

extern void GKGameViewSetInterlaced(GKGameView* view, bool enabled) {
	view->interlace_enabled = enabled;
	view->gb.direct.interlace = enabled;
}