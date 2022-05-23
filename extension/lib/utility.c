#include "utility.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common.h"

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

unsigned char* GKReadFileContents(const char* path) {
	GKFile* rom_file = GKFileOpen(path, kGKFileReadData);
	size_t rom_size;
	unsigned char* rom = NULL;
	
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