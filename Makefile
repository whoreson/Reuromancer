CC = gcc
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
UNAME_M := $(shell uname -p)
ifeq ($(UNAME_M), powerpc)
OBJ_DIR = obj_mac_ppc
TARGET = sdlmancer_reu_mac
else
OBJ_DIR = obj_mac_x86
TARGET = sdlmancer_reu_mac
endif
else
OBJ_DIR = obj
TARGET = sdlmancer_reu
endif
CFLAGS ?= -O2
CFLAGS += -Wall -I/usr/local/include/SDL \
         -I$(CURDIR)/NeuromancerWin64 \
         -I$(CURDIR)/LibNeuroRoutines \
         -I$(CURDIR) \
         -std=c99 -D_LIBNEURO_INLINE
LDFLAGS = -L/usr/local/lib

REU = NeuromancerWin64
LIB = LibNeuroRoutines

OBJS = \
    $(CURDIR)/$(OBJ_DIR)/sfml_stub.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/huffman_decompression.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/decompression.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/cp437.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/drawing.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/animation.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/asm_audio.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/sdl_audio.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/seg7_data.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/audio.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/sdl_audio.o \
    $(CURDIR)/$(OBJ_DIR)/$(LIB)/resources_lists.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/address_translator.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/bg_animation_control.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/character_control.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/data.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/drawing_control.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/items.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/neuro86.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/neuro_menu_control.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/neuro_window_control.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/not_implemented.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/resource_manager.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/rw_state_body_parts_shop.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/rw_state_dialog.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/rw_state_disk_options.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/rw_state_inventory.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/rw_state_pax.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/rw_state_rom.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/rw_state_skills.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/save_load.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/scene_control.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/scene_main_menu.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/scene_real_world.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/utilities.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/window_animation.o \
    $(CURDIR)/$(OBJ_DIR)/$(REU)/main.o

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
LDLIBS = -lSDLmain -lSDL -lm -Wl,-framework,Cocoa
else
LDLIBS = -lSDL -lpthread -lm
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# sfml_stub is in parent dir
$(CURDIR)/$(OBJ_DIR)/sfml_stub.o: $(CURDIR)/../sfml_stub.c
	@mkdir -p $(CURDIR)/$(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# LibNeuroRoutines
$(CURDIR)/$(OBJ_DIR)/$(LIB)/%.o: $(LIB)/%.c
	@mkdir -p $(CURDIR)/$(OBJ_DIR)/$(LIB)
	$(CC) $(CFLAGS) -c -o $@ $<

# NeuromancerWin64
$(CURDIR)/$(OBJ_DIR)/$(REU)/%.o: $(REU)/%.c
	@mkdir -p $(CURDIR)/$(OBJ_DIR)/$(REU)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf obj obj_mac_ppc obj_mac_x86 sdlmancer_reu sdlmancer_reu_mac
