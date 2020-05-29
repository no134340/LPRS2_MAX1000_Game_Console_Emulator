
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

#define Y_PADDING 56 // number of blank rows (treba nam crno iznad ekrana za HUD)

#define LINK_ORIENATION_OFFSET 24 // every link sprite in the sheet is 24px apart (ja sam tako nacrtala u gimp-u da bude lakše da odsecamo linkića iz sheet-a, oni bez mača)
#define LINK_SPRITE_DIM 16 // sprites are 16x16px (oni mali bez mača. oni s mačem su 32px)
						   // nismo toliko uznapredovali u razvoju igrice da pišem i konstante za taj poslednji red gde su linkići s mačem
						   // ukratko 16px link + 8px prazno + 32px link + 8px prazno + 16px link + 8px prazno + 32px link



///////////////////////////////////////////////////////////////////////////////
// Game data structures.


typedef struct {
	uint16_t x;
	uint16_t y;
} point_t;


// Orijentacije linka redom kako su na sprite sheet-u
// Svaka orijentacija ima 2 sprajta
// Ovako možemo indeksirati gornji levi ugao svakog od sprajtova na sledeći način:
// LEFT 0:
// link_sheet__p[(0*LINK_ORIENTATION_OFFSET)*link_sheet__w +  LEFT*LINK_ORIENATION_OFFSET]
// UP 1:
// link_sheet__p[(1*LINK_ORIENTATION_OFFSET)*link_sheet__w +  UP*LINK_ORIENATION_OFFSET]
// nije kao da ćemo ovako indeksirati, ali da znate kako su raspoređeni
typedef enum {
	DOWN,
	LEFT,
	UP,
	RIGHT
} link_orientation_t;

typedef struct {
	link_orientation_t orientation;
	int orientation_state;
	uint8_t delay;
} link_anim_t;

typedef struct {	
	// upper left corner of the link sprite
	point_t pos;
	link_anim_t anim;
	// ovo iskoristiti pri crtanju dela pozadine gde je bio link. Kada to proradi.
	point_t old_pos;
} link_t;



// pokazivači na svaki od delova mapa
// 2D INDEKSI SCREEN-OVA:
// A1 - (0, 0), B1 - (0, 1) ITD.
// 1D INDEKS ZA SCREEN SA 2D INDEKSOM (i, j):
// j*16 + i
// 16 zato sto ima 16 screenova u jednom redu, redova ima 8
// (ima 16*8 = 128 screenova ukupno)
// + jedan title screen
// ovo koristimo kada prosleđujemo funkciji za crtkanje pozadine šta da crta
uint32_t* screens[] = 
{
	A1__p, B1__p, C1__p, D1__p, E1__p, F1__p, G1__p, H1__p, I1__p, J1__p, K1__p, L1__p, M1__p, N1__p, O1__p, P1__p,
	A2__p, B2__p, C2__p, D2__p, E2__p, F2__p, G2__p, H2__p, I2__p, J2__p, K2__p, L2__p, M2__p, N2__p, O2__p, P2__p,
	A3__p, B3__p, C3__p, D3__p, E3__p, F3__p, G3__p, H3__p, I3__p, J3__p, K3__p, L3__p, M3__p, N3__p, O3__p, P3__p,
	A4__p, B4__p, C4__p, D4__p, E4__p, F4__p, G4__p, H4__p, I4__p, J4__p, K4__p, L4__p, M4__p, N4__p, O4__p, P4__p,
	A5__p, B5__p, C5__p, D5__p, E5__p, F5__p, G5__p, H5__p, I5__p, J5__p, K5__p, L5__p, M5__p, N5__p, O5__p, P5__p,
	A6__p, B6__p, C6__p, D6__p, E6__p, F6__p, G6__p, H6__p, I6__p, J6__p, K6__p, L6__p, M6__p, N6__p, O6__p, P6__p,
	A7__p, B7__p, C7__p, D7__p, E7__p, F7__p, G7__p, H7__p, I7__p, J7__p, K7__p, L7__p, M7__p, N7__p, O7__p, P7__p,
	A8__p, B8__p, C8__p, D8__p, E8__p, F8__p, G8__p, H8__p, I8__p, J8__p, K8__p, L8__p, M8__p, N8__p, O8__p, P8__p,
	title_screen__p 
};

// pokazivači na palete
// Intro screen i redovni screenovi sa mape i link imaju zajedno
// 17 boja. Tužno. Zato smo razdvojili posebnu paletu za intro screen
// i mapu i linka. 
uint32_t* screen_palletes[] = 
{
	// izgleda da je moja izmena u img_to_src.py skripti pokupila ime poslednje slike pa je po
	// njoj nadenula ime paleti. it's not a bug it's a feature
	palette_title_screen, palette_link_sheet
};

typedef struct {
	// trenutno prikazani deo mape / ekran
	int current_screen;
	// linkić
	link_t link;
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


static void draw_background(
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
				(dst_y+y)*SCREEN_IDX4_W +
				(dst_x8+8*x8);
			for(int i = 0; i < 8; i++){
				unpack_idx4_p32[dst_idx + i] = (pixels>>(4*i))&0b1111;
			}
			
		}
	}
}

static void draw_sprite_from_atlas(
	uint32_t* sprite_atlas,
	uint16_t atlas_w,
	uint16_t src_x,
	uint16_t src_y,
	uint16_t w,
	uint16_t h,
	uint16_t dst_x,
	uint16_t dst_y,
	int sprite
) {
	uint16_t src_w8 = shift_div_with_round_up(w, 3);
	uint16_t atlas_w8 = shift_div_with_round_up(atlas_w, 3);
	uint16_t src_x8 = shift_div_with_round_down(src_x, 3);
	

	for(uint16_t y = 0; y < h; y++){
		for(uint16_t x8 = 0; x8 < src_w8; x8++){
			uint32_t src_idx = 
				(src_y+y)*atlas_w8 +
				(src_x8+x8);
			uint32_t dst_idx = 
				(dst_y+y)*SCREEN_IDX4_W +
				(dst_x+8*x8);
			uint32_t pixel = sprite_atlas[src_idx];
			for(uint8_t i = 0; i < 8; i++) {
				// provera svakog piksela da li je crn (tj. da li je index 0)
				uint32_t px = (pixel>>(4*i))&0b1111;
				// ako jeste, ne crtaj ga!!
				if(px == 0 && sprite == 1) {
					continue;
				}
				unpack_idx4_p32[dst_idx + i] = px;
			}
		}
	}
}


static void update_background (
	uint32_t* sprite_atlas,
	uint16_t atlas_w,
	uint16_t src_x,
	uint16_t src_y,
	uint16_t w,
	uint16_t h,
	uint16_t dst_x,
	uint16_t dst_y
) {
	// indeksi u matrici sa sprajtom moraju biti deljivi sa 8
	if((src_x % 8) != 0) {
		uint16_t rem = src_x % 8;
		
		draw_sprite_from_atlas(sprite_atlas, atlas_w, src_x - rem, src_y, w, h, dst_x - rem, dst_y, 0);
		if(src_x + 16 - rem + w > title_screen__w) {
			w -= src_x + 16 - rem + w - title_screen__w;
		}
		draw_sprite_from_atlas(sprite_atlas, atlas_w, src_x + 16 - rem, src_y, w, h, dst_x + 16 - rem, dst_y, 0);
	} 
	else {
			draw_sprite_from_atlas(sprite_atlas, atlas_w, src_x, src_y, w, h, dst_x, dst_y, 0);
	}

}


///////////////////////////////////////////////////////////////////////////////
// Game code.

int main(void) {
	
	// Setup.
	gpu_p32[0] = 2; // IDX4 mode
	gpu_p32[1] = 0; // Unpacked mode.

	// Copy palette.
	for(uint8_t i = 0; i < 16; i++){
		palette_p32[i] = palette_title_screen[i];
	}

	// Game state.
	game_state_t gs;
	// redovni screenovi su od 0-127, 128 je title screen
	gs.current_screen = 128;
	int y_padding = 0;

	gs.link.anim.orientation = LEFT;
	gs.link.anim.orientation_state = 0;
	gs.link.anim.delay = 0;
	gs.link.pos.x = 128;
	gs.link.pos.y = 128;
	gs.link.old_pos.x = gs.link.pos.x;
	gs.link.old_pos.y = gs.link.pos.y;

	volatile int draw_link = 0;
	volatile int draw_bg = 1;
	int started = 0;

	// spreci linkica da se crta po title-screen-u

	while(1){
		
		/*
			Za sada samo:
			Crtkanje intro screen-a na ekran.
			I kada korisnik pritisne s kao start, da se otvori mapa A1 sa velike mape.
			I da se nacrta Link na njoj. Za sada Link samo može da lebdi okolo.
		*/
		
		/////////////////////////////////////
		// Poll controls.

		int mov_x = 0;
		int mov_y = 0;
	
		// btw izmenila sam Lošmijev kod u emulatoru jer kako je bilo
		// start ostane zakucan na 1 nakon što ga pritisneš
		// pa sam tamo izmenila da se vrati na 0 nakon što se otpusti
		// postoji rešenje za ovo a da se ne menja njegov kod, ali...
		if(joypad.start) {
			// neće sa start screena prelaziti na ovaj deo mape, ovo je samo test
			gs.current_screen = 0;
			y_padding = Y_PADDING;
			draw_link = 1;
			// prebaci se na paletu za ekrane i linka
			for(uint8_t i = 0; i < 16; i++){
				palette_p32[i] = palette_link_sheet[i];
			}
			draw_bg = 1;
			started = 1;
		}
		/////////////////////////////////////
		// Gameplay.
		
		// Da ne prođe kroz ivice ekrana
		// Link lepo lebdi po svim osama sada.
		

		if(started) {
			if(joypad.left) {
			mov_x = -1;
			draw_link = 1;
			}
			else if(joypad.right) {
				mov_x = +1;
				draw_link = 1;
			}
			if(joypad.up) {
				mov_y = -1;
				draw_link = 1;
			}
			else if(joypad.down) {
				mov_y = +1;
				draw_link = 1;
			}

			if(mov_x + gs.link.pos.x < 0) {
			gs.link.pos.x = 0;
			}
			else if (mov_x + gs.link.pos.x >= title_screen__w - LINK_SPRITE_DIM) {
				gs.link.pos.x = title_screen__w - LINK_SPRITE_DIM;
			}
			else {
				gs.link.pos.x += mov_x;
			}
			if(mov_y + gs.link.pos.y < Y_PADDING) {
				gs.link.pos.y = Y_PADDING;
			}
			else if (mov_y + gs.link.pos.y >= title_screen__h - LINK_SPRITE_DIM) {
				gs.link.pos.y = title_screen__h - LINK_SPRITE_DIM;
			}
			else {
				gs.link.pos.y += mov_y;
			}

		}
		
		
		
		/////////////////////////////////////
		// Drawing.
		
		
		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.
		
		
		if(draw_bg == 1){
			for(uint16_t r1 = 0; r1 < SCREEN_IDX4_H; r1++){
				for(uint16_t c8 = 0; c8 < SCREEN_IDX4_W; c8++){
					unpack_idx4_p32[r1*SCREEN_IDX4_W + c8] = 0x00000000;
				}
			}
		}
		
		if(draw_bg == 1) {
			draw_background(
			screens[gs.current_screen], title_screen__w, title_screen__h - y_padding, 0, y_padding
			);
			draw_bg = 0;
		}
		

		if(draw_link == 1) {
			// Apdejtuj samo pozadinu oko Linka.
			update_background(
				screens[gs.current_screen], title_screen__w,
				gs.link.old_pos.x,
				gs.link.old_pos.y - Y_PADDING,
				LINK_SPRITE_DIM, LINK_SPRITE_DIM,
				gs.link.old_pos.x,
				gs.link.old_pos.y
			);

			gs.link.old_pos.x = gs.link.pos.x;
			gs.link.old_pos.y = gs.link.pos.y;

			draw_sprite_from_atlas(
				link_sheet__p, link_sheet__w,
				gs.link.anim.orientation*LINK_ORIENATION_OFFSET,
				gs.link.anim.orientation_state*LINK_ORIENATION_OFFSET,
				LINK_SPRITE_DIM, LINK_SPRITE_DIM,
				gs.link.pos.x,
				gs.link.pos.y,
				1
			);
			draw_link = 0;
		}

	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
