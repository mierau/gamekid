HEAP_SIZE      = 8388208
STACK_SIZE     = 61800

PRODUCT = gamekid.pdx

# Locate the SDK
SDK = $(shell egrep '^\s*SDKRoot' ~/.Playdate/config | head -n 1 | cut -c9-)

VPATH += extension
VPATH += extension/lib

# List C source files here
MAIN_SRC = \
extension/main.c \
extension/app.c \
extension/lib/utility.c \
extension/lib/list.c \
extension/libraryview.c \
extension/gameview.c \

GAMEBOY_SRC = \
extension/emulator/gb/minigb_apu.c \
extension/emulator/adapter_gb.c

SRC = $(MAIN_SRC) $(GAMEBOY_SRC)

# List all user directories here
UINCDIR = extension extension/lib extension/emulator extension/emulator/gb

# List user asm files
UASRC = 

# List all user C define here, like -D_DEBUG=1
UDEFS = 

# Define ASM defines here
UADEFS = 

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

include $(SDK)/C_API/buildsupport/common.mk