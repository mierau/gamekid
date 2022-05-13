#include "utility.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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