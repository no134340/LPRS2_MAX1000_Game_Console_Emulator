
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>
#include "title_screen_idx4.h"
#include "screens_idx4.h"


///////////////////////////////////////////////////////////////////////////////
// HW stuff.

#define WAIT_UNITL_0(x) while(x != 0){}
#define WAIT_UNITL_1(x) while(x != 1){}

#define SCREEN_IDX1_W 640
#define SCREEN_IDX1_H 480
#define SCREEN_IDX4_W 320
#define SCREEN_IDX4_H 240
#define SCREEN_RGB333_W 160
#define SCREEN_RGB333_H 120

#define SCREEN_IDX4_W8 (SCREEN_IDX4_W/8)

#define gpu_p32 ((volatile uint32_t*)LPRS2_GPU_BASE)
#define palette_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x1000))
#define unpack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x400000))
#define pack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x600000))
#define unpack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x800000))
#define pack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xa00000))
#define unpack_rgb333_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xc00000))
#define joypad_p32 ((volatile uint32_t*)LPRS2_JOYPAD_BASE)

typedef struct {
	unsigned a      : 1;
	unsigned b      : 1;
	unsigned z      : 1;
	unsigned start  : 1;
	unsigned up     : 1;
	unsigned down   : 1;
	unsigned left   : 1;
	unsigned right  : 1;
} bf_joypad;
#define joypad (*((volatile bf_joypad*)LPRS2_JOYPAD_BASE))

typedef struct {
	uint32_t m[SCREEN_IDX1_H][SCREEN_IDX1_W];
} bf_unpack_idx1;
#define unpack_idx1 (*((volatile bf_unpack_idx1*)unpack_idx1_p32))



///////////////////////////////////////////////////////////////////////////////
// Game config.

#define Y_PADDING 56 // number of blank rows



///////////////////////////////////////////////////////////////////////////////
// Game data structures.

typedef struct {
	uint16_t x;
	uint16_t y;
} point_t;


typedef enum {
	MAP_A1,
	TITLE_SCREEN
	// posle dodati ostale screenove
	// title screen na kraju.
} current_screen_t;

// pointers to screen "sprites"
uint32_t* screens[] = 
{
	A1__p, title_screen__p 
};

// pointers to screen palletes 
uint32_t* screen_palletes[] = 
{
	// izgleda da je moja izmena u skripti pokupila ime poslednjeg skrina pa je po
	// njemu nadenula ime paleti. it's not a bug it's a feature/
	palette_title_screen, palette_P8
};

typedef struct {
	// trenutno prikazani deo mape / ekran
	current_screen_t current_screen;
} game_state_t;



static inline uint32_t shift_div_with_round_down(uint32_t num, uint32_t shift){
	uint32_t d = num >> shift;
	return d;
}

static inline uint32_t shift_div_with_round_up(uint32_t num, uint32_t shift){
	uint32_t d = num >> shift;
	uint32_t mask = (1<<shift)-1;
	if((num & mask) != 0){
		d++;
	}
	return d;
}



static void draw_sprite(
	uint32_t* src_p,
	uint16_t src_w,
	uint16_t src_h,
	uint16_t dst_x,
	uint16_t dst_y
) {
	
	
	uint16_t dst_x8 = shift_div_with_round_down(dst_x, 3);
	uint16_t src_w8 = shift_div_with_round_up(src_w, 3);
	
	
	
	for(uint16_t y = 0; y < src_h; y++){
		for(uint16_t x8 = 0; x8 < src_w8; x8++){
			uint32_t src_idx = y*src_w8 + x8;
			uint32_t pixels = src_p[src_idx];
			uint32_t dst_idx =
				(dst_y+y)*SCREEN_IDX4_W8 +
				(dst_x8+x8);
			pack_idx4_p32[dst_idx] = pixels;
		}
	}
	
	
}




///////////////////////////////////////////////////////////////////////////////
// Game code.

int main(void) {
	
	// Setup.
	gpu_p32[0] = 2; // IDX4 mode
	gpu_p32[1] = 1; // Packed mode.

	// Copy palette.
	for(uint8_t i = 0; i < 16; i++){
		palette_p32[i] = palette_title_screen[i];
	}

	// Game state.
	game_state_t gs;
	gs.current_screen = TITLE_SCREEN;
	int y_padding = 0;
	
	while(1){
		
		/*
			Za sada samo isprobavamo crtkanje intro screen-a na ekran.
		*/
		
		/////////////////////////////////////
		// Poll controls.
		
		if(joypad.start) {
			// nece sa start screena prelaziti na ovaj deo mape, ovo je samo test
			gs.current_screen = MAP_A1;
			y_padding = Y_PADDING;
		}
		
		/////////////////////////////////////
		// Gameplay.
		

		
		
		
		
		/////////////////////////////////////
		// Drawing.
		
		
		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.
		
		
		
		for(uint16_t r1 = 0; r1 < SCREEN_IDX4_H; r1++){
			for(uint16_t c8 = 0; c8 < SCREEN_IDX4_W8; c8++){
				pack_idx4_p32[r1*SCREEN_IDX4_W8 + c8] = 0x00000000;
			}
		}
		
		
		// draw the background (the current active screen)
		for(uint8_t i = 0; i < 16; i++){
			palette_p32[i] = screen_palletes[gs.current_screen == TITLE_SCREEN ? 0 : 1][i];
		}
		draw_sprite(
			screens[gs.current_screen], title_screen__w, title_screen__h - y_padding, 0, y_padding
		);

		
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
