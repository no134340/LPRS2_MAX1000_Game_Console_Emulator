
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>
#include "title_screen_idx4.h"
#include "screens_idx4.h"
#include "screen_tile_index.h"


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

#define FIRST_HUD_PADDING 88 //16+64+8
#define FIRST_HUD_SIZE 8

#define LINK_ORIENATION_OFFSET 24 // every link sprite in the sheet is 24px apart (ja sam tako nacrtala u gimp-u da bude lakše da odsecamo linkića iz sheet-a, oni bez mača)
#define SPRITE_DIM 16 // sprites are 16x16px (oni mali bez mača. oni s mačem su 32px)
						   // nismo toliko uznapredovali u razvoju igrice da pišem i konstante za taj poslednji red gde su linkići s mačem
						   // ukratko 16px link + 8px prazno + 32px link + 8px prazno + 16px link + 8px prazno + 32px link
					  // i tile-ove imaju dimenzije 16x16

#define ANIM_DELAY 10

#define TILES_H 16 // broj tile-ova koji staje u svaki red

#define TILES_V 10 // broj tile-ova koji staje u svaku vrstu

#define TILE_SIZE 16

#define OVERWORLD_MAPS_H 16

#define OVERWORLD_MAPS_V 8

#define Y_DIAMOND 6
#define Y_KEY 22
#define Y_BOMB 30

#define LINK_ATTACK_Y 72
#define VOID_BETWEEN_LINKS 8

#define MAX_ENEMIES 3
#define ENEMY_SPEED 3
#define ENEMY_STEP 2

#define IMUNITY_TIME 50

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
	point_t old_pos;
	int lives;
} link_t;

typedef struct {
	point_t pos;
	point_t old_pos;
	link_anim_t anim;
	int dead;
} enemy_t;



// pokazivači na svaki od delova mapa
// ovo su sada matrice sa indeksima tile-ova!
// 2D INDEKSI SCREEN-OVA:
// A1 - (0, 0), B1 - (0, 1) ITD.
// 1D INDEKS ZA SCREEN SA 2D INDEKSOM (i, j):
// j*16 + i
// 16 zato sto ima 16 screenova u jednom redu, redova ima 8
// (ima 16*8 = 128 screenova ukupno)
// + jedan title screen
// ovo koristimo kada prosleđujemo funkciji za crtkanje tile-ova šta da crta
uint8_t* screens[] = 
{
	A1__p, B1__p, C1__p, D1__p, E1__p, F1__p, G1__p, H1__p, I1__p, J1__p, K1__p, L1__p, M1__p, N1__p, O1__p, P1__p,
	A2__p, B2__p, C2__p, D2__p, E2__p, F2__p, G2__p, H2__p, I2__p, J2__p, K2__p, L2__p, M2__p, N2__p, O2__p, P2__p,
	A3__p, B3__p, C3__p, D3__p, E3__p, F3__p, G3__p, H3__p, I3__p, J3__p, K3__p, L3__p, M3__p, N3__p, O3__p, P3__p,
	A4__p, B4__p, C4__p, D4__p, E4__p, F4__p, G4__p, H4__p, I4__p, J4__p, K4__p, L4__p, M4__p, N4__p, O4__p, P4__p,
	A5__p, B5__p, C5__p, D5__p, E5__p, F5__p, G5__p, H5__p, I5__p, J5__p, K5__p, L5__p, M5__p, N5__p, O5__p, P5__p,
	A6__p, B6__p, C6__p, D6__p, E6__p, F6__p, G6__p, H6__p, I6__p, J6__p, K6__p, L6__p, M6__p, N6__p, O6__p, P6__p,
	A7__p, B7__p, C7__p, D7__p, E7__p, F7__p, G7__p, H7__p, I7__p, J7__p, K7__p, L7__p, M7__p, N7__p, O7__p, P7__p,
	A8__p, B8__p, C8__p, D8__p, E8__p, F8__p, G8__p, H8__p, I8__p, J8__p, K8__p, L8__p, M8__p, N8__p, O8__p, P8__p,
};

// gde ima koliko enemy-ja
uint8_t enemies_array[] = 
{
	1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 0, 1, 1, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 0, 0, 0, 1, 2, 0, 1, 1, 1, 1, 0, 0, 0, 0,
};

// pokazivači na palete
// Intro screen i redovni screenovi sa mape i link imaju zajedno
// 17 boja. Tužno. Zato smo razdvojili posebnu paletu za intro screen
// i mapu i linka. Zašto ovo postoji kad nije potrebno, ne znam. 
uint32_t* screen_palettes[] = 
{
	// izgleda da je moja izmena u img_to_src.py skripti pokupila ime poslednje slike pa je po
	// njoj nadenula ime paleti. it's not a bug it's a feature
	palette_title_screen, palette_tiles
};


uint8_t walkable_tiles[26] = {0, 2, 6, 12, 14, 18, 22, 24, 28, 30, 34, 3*18 + 4, 3*18 + 10, 3*18 + 16, 4*18 + 4, 4*18 + 10, 4*18 + 16, 5*18 + 4, 5*18 + 10, 5*18+16, 6*18 + 4, 6*18 + 10, 6*18+16, 7*18 + 5, 7*18 + 11, 7*18+17};


// kojim redom ide koji karakter u tile sheet-u za slova
typedef enum {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, COMMA, EXCL, APO, AND, PERIOD, QUOTE, QMARK, DASH} font_indices;

typedef struct {
	// trenutno prikazani deo mape / ekran
	int current_screen;
	// linkić
	link_t link;
	int game_over;
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
		int cnt = 0;
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
				cnt++;
				if(cnt > w) {
					break;
				}
				uint32_t px = (pixel>>(4*i))&0b1111;
				// ako jeste, ne crtaj ga!! (samo ako je sprajt. ako je pozadina,
				// onda je to deo pozadine i treba crtati taj crni piksel)
				if(px == 0 && sprite == 1) {
					continue;
				}
				unpack_idx4_p32[dst_idx + i] = px;
			}
		}
	}
}


static void draw_tiles(
	uint8_t* src_p,
	uint16_t src_w, // broj tile-ova horizontalno
	uint16_t src_h, // broj tile-ova vertikalno
	uint16_t dst_x,
	uint16_t dst_y
) {
		
	
	for(uint16_t y = 0; y < 10; y++){
		for(uint16_t x = 0; x < 16; x++){
			uint8_t ind = src_p[y*SPRITE_DIM + x];
			/*
				U python skripti za pravljenje indeksa tile-ova
				stavila sam da stavi kao indeks 144 ukoliko tile
				nije pronađen u tile-sheetu..... Ovo bi trebalo
				rešiti nekako kasnije (najbolje ručno dodati tile-ove u sheet,
				python skripta će bez problema odraditi ponovo posao sa 
				dodatim tile-ovima). Za sada uzme onaj skroz crni tile.
			*/
			if(ind == 150) {
				ind = 22;
			}
			uint16_t ind_vert = ind / src_w;
			uint16_t ind_horiz = ind % src_w;
			draw_sprite_from_atlas(
				tiles__p, tiles__w, ind_horiz*SPRITE_DIM, ind_vert*SPRITE_DIM, SPRITE_DIM, SPRITE_DIM, x*SPRITE_DIM, (y*SPRITE_DIM+dst_y), 0
			);
		}
	}
}



static void update_background (
	uint8_t* sprite_atlas,
	uint16_t atlas_w,
	uint16_t src_x,
	uint16_t src_y,
	uint16_t w,
	uint16_t h,
	uint16_t dst_x,
	uint16_t dst_y
) {
	// indeksi u matrici sa tile-ovima moraju biti deljivi sa veličinom tile-a (da bi smo uzeli ceo tile)
	/*
		Ako ovo nije slušaj, naš link će preklopiti makar 9 tile-a u najgorem slučaju,
		pa moramo iscrtati svih 9
	*/

	uint16_t rem_x = src_x % SPRITE_DIM;
	uint16_t rem_y = src_y % SPRITE_DIM;
	
	uint16_t tile_ind_x = (src_x - rem_x) / SPRITE_DIM;
	uint16_t tile_ind_y = (src_y - rem_y) / SPRITE_DIM;

	// upper left
	uint8_t tile_ind;
	uint16_t ind_vert;
	uint16_t ind_horiz;

	for (int i = -2; i < 3; i++) {
		for(int j = -2; j < 3; j++) {
			if(dst_y - rem_y + j*SPRITE_DIM < title_screen__h - 9 && dst_x < title_screen__w - i*SPRITE_DIM &&
			dst_y + j*SPRITE_DIM > Y_PADDING && dst_x + i*SPRITE_DIM > 0)  {
				tile_ind_x += i;
				tile_ind_y += j;
				tile_ind = sprite_atlas[tile_ind_y*atlas_w + tile_ind_x];
				ind_vert = tile_ind / tile_num_x;
				ind_horiz = tile_ind % tile_num_x;
				draw_sprite_from_atlas(tiles__p, tiles__w, ind_horiz*SPRITE_DIM, ind_vert*SPRITE_DIM, w, h, dst_x + i*SPRITE_DIM - rem_x, dst_y + j*SPRITE_DIM - rem_y, 0);
				tile_ind_x -= i;
				tile_ind_y -= j;
			}
		}
	}
}

void draw_HUD_number(uint16_t x,uint16_t location, uint16_t height) {
	draw_sprite_from_atlas(fonts_white__p, fonts_white__w, 2 * x * FIRST_HUD_SIZE, 0, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, 0 + FIRST_HUD_PADDING + 2*FIRST_HUD_SIZE-3 +location*FIRST_HUD_SIZE, height, 1);
}

void number_generator(uint16_t x,uint16_t height) {
	if( x / 10 != 0) {
		draw_HUD_number(x / 10, 0, height);
		draw_HUD_number(x % 10, 1, height);
	}
	else {
		draw_HUD_number(x, 0, height);
	}
}



void init_HUD() {
	draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, 0, 0, FIRST_HUD_SIZE, FIRST_HUD_SIZE, 0 + FIRST_HUD_PADDING, 10, 1);
	draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, 0, FIRST_HUD_SIZE, FIRST_HUD_SIZE, FIRST_HUD_SIZE, 0 + FIRST_HUD_PADDING, 10+FIRST_HUD_SIZE*2, 1);
	draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, 0, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE, FIRST_HUD_SIZE, 0 + FIRST_HUD_PADDING, 10+FIRST_HUD_SIZE*3, 1);

	draw_sprite_from_atlas(fonts_white__p, fonts_white__w, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*4, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, 0 + FIRST_HUD_PADDING + FIRST_HUD_SIZE-3, Y_DIAMOND, 1);
	draw_sprite_from_atlas(fonts_white__p, fonts_white__w, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*4, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, 0 + FIRST_HUD_PADDING + FIRST_HUD_SIZE-3, Y_KEY, 1);
	draw_sprite_from_atlas(fonts_white__p, fonts_white__w, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*4, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, 0 + FIRST_HUD_PADDING + FIRST_HUD_SIZE-3, Y_BOMB, 1);

	number_generator(0,Y_DIAMOND);
	number_generator(0,Y_KEY);
	number_generator(0,Y_BOMB);


	draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, FIRST_HUD_SIZE, 0, FIRST_HUD_SIZE*2+8, FIRST_HUD_SIZE*4-1, FIRST_HUD_PADDING + FIRST_HUD_SIZE*4, FIRST_HUD_SIZE*2, 1);//b
	//ovo neka ostane, trebace za kasnije kada se uvede intro kada udje u pecinu i pokupi mac.
	draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, FIRST_HUD_SIZE*4+4, 0, FIRST_HUD_SIZE*2+8, FIRST_HUD_SIZE*4-1, FIRST_HUD_PADDING + FIRST_HUD_SIZE*7, FIRST_HUD_SIZE*2, 1);


	draw_sprite_from_atlas(fonts_red__p, fonts_red__w, FIRST_HUD_SIZE*2*(DASH % 16), FIRST_HUD_SIZE*2*(DASH >> 4), FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*11, 15, 1);
	draw_sprite_from_atlas(fonts_red__p, fonts_red__w, FIRST_HUD_SIZE*2*(L % 16), FIRST_HUD_SIZE*2*(L >> 4), FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*12, 15, 1);
	draw_sprite_from_atlas(fonts_red__p, fonts_red__w, FIRST_HUD_SIZE*2*(I % 16), FIRST_HUD_SIZE*2 * (I >> 4), FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*13, 15, 1);
	draw_sprite_from_atlas(fonts_red__p, fonts_red__w, FIRST_HUD_SIZE*2*(F % 16), FIRST_HUD_SIZE*2*(F >> 4), FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*14, 15, 1);
	draw_sprite_from_atlas(fonts_red__p, fonts_red__w, FIRST_HUD_SIZE*2*(E % 16), FIRST_HUD_SIZE*2*(E >> 4), FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2,  FIRST_HUD_PADDING + FIRST_HUD_SIZE*15, 15, 1);
	draw_sprite_from_atlas(fonts_red__p, fonts_red__w, FIRST_HUD_SIZE*2*(DASH %16), FIRST_HUD_SIZE*2*(DASH >> 4), FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*16, 15, 1);

	for(int i = 0; i < 3; i++) {
		draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, FIRST_HUD_SIZE*1, FIRST_HUD_SIZE*4 - 2, FIRST_HUD_SIZE, FIRST_HUD_SIZE + 2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*11 + 4 + i*10, 16 + 2*FIRST_HUD_SIZE - 4, 1);
	}

	for(int r1 = 8; r1 < 40 + 8; r1++){
		for(int c = 16; c < 16 + 64; c++){
			unpack_idx4_p32[r1*SCREEN_IDX4_W + c] = 5;
		}
	}
	
}
void draw_HUD_sword() {
	draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, 0,FIRST_HUD_SIZE*4-1, FIRST_HUD_SIZE, FIRST_HUD_SIZE*2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*5, FIRST_HUD_SIZE*3+1, 1);
}


void update_minimap(uint32_t old_screen, uint32_t current_screen) {

	int sc_x = old_screen % OVERWORLD_MAPS_H;
	int sc_y = old_screen / OVERWORLD_MAPS_H;
	for(int r1 = 8 + sc_y*5; r1 < 8 + (sc_y + 1)*5; r1++){
		for(int c = 16 + sc_x*4; c < 16 + (sc_x + 1) * 4; c++){
			unpack_idx4_p32[r1*SCREEN_IDX4_W + c] = 5;
		}
	}

	sc_x = current_screen % OVERWORLD_MAPS_H;
	sc_y = current_screen / OVERWORLD_MAPS_H;
	for(int r1 = 8 + sc_y*5; r1 < 8 + (sc_y + 1)*5; r1++){
		for(int c = 16 + sc_x*4; c < 16 + (sc_x + 1) * 4; c++){
			unpack_idx4_p32[r1*SCREEN_IDX4_W + c] = 7;
		}
	}
}


// ovo je za crtanje title intro screen-a
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

int check_collision(uint8_t curr_x, uint8_t curr_y) {
	for(int i = 0; i < 26; i++) {
		if(curr_x == walkable_tiles[i] && curr_y == walkable_tiles[i]) {
			return 0;
		}
	}
	return 1;
}
void cave_update_background(int x, int y, int delete) {
	draw_sprite_from_atlas(dungeon__p, dungeon__w, x, y - Y_PADDING, SPRITE_DIM, delete, x, y, 0);
}

void cave_animation(game_state_t gs) {
	int tune_anim = 100000;
	font_indices text[] = {I, T, APO, S, DASH, DASH, D, A, N, G, E, R, O, U, S, DASH, T, O, DASH, G, O, AND, A, L, O, N, E, EXCL, DASH, H, E, R, E, DASH, T, A, K, E, DASH, T, H, I, S, EXCL};
	draw_sprite_from_atlas(dungeon__p, dungeon__w, 0, 0, dungeon__w, dungeon__h-8, 0, Y_PADDING, 0);
	int localX = 120;
	int localY = Y_PADDING + 135;
	gs.link.pos.x = localX;
	gs.link.pos.y = localY;
	int loshmeeX = localX;
	int loshmeeY = Y_PADDING+65;
	int loshmee_orientation = 0;
	int animation_state = 0;
	int lineY = Y_PADDING+30;
	int lineX = 40;
	int k = 0;
	int go = 1;
	draw_sprite_from_atlas(
				link_sheet__p, link_sheet__w,
				gs.link.anim.orientation*LINK_ORIENATION_OFFSET,
				gs.link.anim.orientation_state*LINK_ORIENATION_OFFSET,
				SPRITE_DIM, SPRITE_DIM,
				gs.link.pos.x,
				gs.link.pos.y,
				1
			);
	draw_sprite_from_atlas(
				loshmee__p, loshmee__w,
				SPRITE_DIM*loshmee_orientation,
				0,
				SPRITE_DIM, SPRITE_DIM*2,
				loshmeeX,
				loshmeeY,
				1
			);
	draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, 0,FIRST_HUD_SIZE*4-1, FIRST_HUD_SIZE, FIRST_HUD_SIZE*2, localX+4, loshmeeY+40, 1);
	int count = 0;
	while(go) {
		count++;
		for(int i = 0; i<tune_anim;i++){

		}
		if(count == 500) {
			WAIT_UNITL_0(gpu_p32[2]);
			WAIT_UNITL_1(gpu_p32[2]);
			count = 0;
			if((gs.link.pos.y >= Y_PADDING + 130) && !animation_state) {
				cave_update_background(gs.link.pos.x, gs.link.pos.y, 16);
				gs.link.pos.y -= 5;
				gs.link.anim.orientation_state ^= 1;
				draw_sprite_from_atlas(
				link_sheet__p, link_sheet__w,
				gs.link.anim.orientation*LINK_ORIENATION_OFFSET,
				gs.link.anim.orientation_state*LINK_ORIENATION_OFFSET,
				SPRITE_DIM, SPRITE_DIM,
				gs.link.pos.x,
				gs.link.pos.y,
				1
			);
			}
			else if(!animation_state) {
				animation_state = 1;
			}
			if((animation_state == 1) && (k < 44)) {
				loshmee_orientation ^= 1;
				cave_update_background(loshmeeX, loshmeeY, 32);
				draw_sprite_from_atlas(
					loshmee__p, loshmee__w,
					SPRITE_DIM*loshmee_orientation,
					0,
					SPRITE_DIM, SPRITE_DIM*2,
					loshmeeX,
					loshmeeY,
					1
				);
				if((text[k] != DASH) && (text[k] != AND)) {
					draw_sprite_from_atlas(fonts_white__p, fonts_white__w, 
						FIRST_HUD_SIZE*2*(text[k] % 16), FIRST_HUD_SIZE*2*(text[k] >> 4), 
						FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, lineX, 
						lineY, 1);
				}
				if(text[k] != AND) {
					lineX += 8;
				}
				else {
					lineX = 40;
					lineY += 16;
				}
				k++;
				if(k == 44){
					animation_state=2;
				}
			}
			if((animation_state == 2) && (gs.link.pos.y >= loshmeeY+40)) {
				cave_update_background(gs.link.pos.x, gs.link.pos.y, 16);
				gs.link.pos.y -= 5;
				gs.link.anim.orientation_state ^= 1;
				draw_sprite_from_atlas(
					link_sheet__p, link_sheet__w,
					gs.link.anim.orientation*LINK_ORIENATION_OFFSET,
					gs.link.anim.orientation_state*LINK_ORIENATION_OFFSET,
					SPRITE_DIM, SPRITE_DIM,
					gs.link.pos.x,
					gs.link.pos.y,
					1
				);
			}
			else if(animation_state == 2) {
				draw_HUD_sword();
				animation_state = 3;
				gs.link.anim.orientation = DOWN;
			}
			if((animation_state == 3) && (gs.link.pos.y <= localY+5)) {
				cave_update_background(gs.link.pos.x, gs.link.pos.y-1, 15);
				gs.link.anim.orientation_state ^= 1;
				gs.link.pos.y += 5;
				draw_sprite_from_atlas(
					link_sheet__p, link_sheet__w,
					gs.link.anim.orientation*LINK_ORIENATION_OFFSET,
					gs.link.anim.orientation_state*LINK_ORIENATION_OFFSET,
					SPRITE_DIM, SPRITE_DIM,
					gs.link.pos.x,
					gs.link.pos.y-1,
					1
				);
			}
			else if(animation_state == 3) {
				animation_state = 4;
			}
			if(animation_state == 4) {
				go = 0;
			}
			
		}
	}
}


unsigned short lfsr = 0xACE1u;
unsigned rand_generator()
{
	unsigned bit;
	bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
	return lfsr =  (lfsr >> 1) | (bit << 15);
}

void update_lives(int lives) {
	uint8_t whole;
	uint8_t rest;
	uint8_t empty;
	if(lives > 0){
		whole = lives / 2;
		rest = lives % 2;
		empty = (6 - 2*rest - whole) / 2;
	}
	else {
		whole = 0;
		rest = 0;
		empty = 3;
	}
	
	for(int i = 0; i < whole; i++) {
		draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, FIRST_HUD_SIZE*1, FIRST_HUD_SIZE*4 - 2, FIRST_HUD_SIZE, FIRST_HUD_SIZE + 2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*11 + 4 + i*10, 16 + 2*FIRST_HUD_SIZE - 4, 1);
	}
	for(int i = whole; i < whole + rest; i++) {
		draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*4 - 2, FIRST_HUD_SIZE, FIRST_HUD_SIZE + 2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*11 + 4 + i*10, 16 + 2*FIRST_HUD_SIZE - 4, 1);
	}
	for(int i = whole + rest; i < 3; i++) {
		draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, FIRST_HUD_SIZE*3, FIRST_HUD_SIZE*4 - 2, FIRST_HUD_SIZE, FIRST_HUD_SIZE + 2, FIRST_HUD_PADDING + FIRST_HUD_SIZE*11 + 4 + i*10, 16 + 2*FIRST_HUD_SIZE - 4, 1);
	}
}

void check_interaction(int draw_sword, enemy_t* enemies, game_state_t* gs, int* imunity, uint8_t* draw_enemies) {
	if(draw_sword) {
		for(int i = 0; i < enemies_array[gs->current_screen]; i++) {
			if(enemies[i].dead)
				continue;
			int kill = 0;
			switch (gs->link.anim.orientation)
			{
			case RIGHT:
				if((gs->link.pos.x + SPRITE_DIM > enemies[i].pos.x && gs->link.pos.x + SPRITE_DIM < enemies[i].pos.x + SPRITE_DIM) && (gs->link.pos.y > enemies[i].pos.y && gs->link.pos.y < enemies[i].pos.y + SPRITE_DIM))
					kill = 1;
				break;
			case DOWN:
				if((gs->link.pos.x > enemies[i].pos.x && gs->link.pos.x < enemies[i].pos.x + SPRITE_DIM) && (gs->link.pos.y + SPRITE_DIM > enemies[i].pos.y && gs->link.pos.y + SPRITE_DIM < enemies[i].pos.y + SPRITE_DIM))
					kill = 1;
				break;
			case LEFT:
			if((gs->link.pos.x - SPRITE_DIM < enemies[i].pos.x + SPRITE_DIM) && (gs->link.pos.y + SPRITE_DIM > enemies[i].pos.y && gs->link.pos.y + SPRITE_DIM < enemies[i].pos.y + SPRITE_DIM))
					kill = 1;
				break;
			case UP:
				if((gs->link.pos.x > enemies[i].pos.x && gs->link.pos.x < enemies[i].pos.x + SPRITE_DIM) && (gs->link.pos.y - SPRITE_DIM < enemies[i].pos.y + SPRITE_DIM))
					kill = 1;
				break;
			default:
				break;
			}
			if(kill) {
				enemies[i].dead = kill;
				draw_enemies[i] = 0;
			}
		}
	}
	else if (*imunity == 0){
	// !draw_sword
		for(int i = 0; i < enemies_array[gs->current_screen]; i++) {
			if(enemies[i].dead)
				continue;
			if((gs->link.pos.x > enemies[i].pos.x && gs->link.pos.x < enemies[i].pos.x + SPRITE_DIM) && (gs->link.pos.y > enemies[i].pos.y && gs->link.pos.y < enemies[i].pos.y + SPRITE_DIM)) {
				gs->link.lives -= 1;
				if(gs->link.lives < 0) {
					gs->link.lives = 0;
					gs->game_over = 1;
					return;
				}
				update_lives(gs->link.lives);
				*imunity = IMUNITY_TIME;
			}
		}
	}
}

void game_over() {

	WAIT_UNITL_0(gpu_p32[2]);
	WAIT_UNITL_1(gpu_p32[2]);
	for(uint16_t r1 = Y_PADDING; r1 < SCREEN_IDX4_H; r1++){
			for(uint16_t c8 = 0; c8 < SCREEN_IDX4_W; c8++){
				unpack_idx4_p32[r1*SCREEN_IDX4_W + c8] = 0x00000000;
			}
	}

	printf("this woroks");
	fflush(stdout);
	font_indices text[] = {G, A, M, E, DASH, O, V, E, R, EXCL};
	uint8_t lineX = 32;
	uint8_t lineY = Y_PADDING + (title_screen__h - 9  - Y_PADDING) / 2;
	for(int i = 0; i < 10; i++) {
		lineX += 2*FIRST_HUD_SIZE;
		draw_sprite_from_atlas(fonts_white__p, fonts_white__w, 
						FIRST_HUD_SIZE*2*(text[i] % 16), FIRST_HUD_SIZE*2*(text[i] >> 4), 
						FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, lineX, 
						lineY, 1);
	}

	while(1);
}

///////////////////////////////////////////////////////////////////////////////
// Game code.


int main(void) {
	
	// Setup.
	gpu_p32[0] = 2; // IDX4 mode
	gpu_p32[1] = 0; // Unpacked mode

	// Copy palette.
	for(uint8_t i = 0; i < 16; i++){
		palette_p32[i] = palette_title_screen[i];
	}

	// Game state.
	game_state_t gs;
	enemy_t enemies[MAX_ENEMIES];//max broj protivnika moguc na bilo kojoj mapi
	// redovni screenovi su od 0-127
	// -1 naznaka da se crta title screen
	gs.current_screen = -1;
	gs.game_over = 0;
	int y_padding = 0;

	gs.link.anim.orientation = LEFT;
	gs.link.anim.orientation_state = 0;
	gs.link.anim.delay = 0;
	gs.link.pos.x = 32;
	gs.link.pos.y = Y_PADDING + 2 + 64;
	gs.link.old_pos.x = gs.link.pos.x;
	gs.link.old_pos.y = gs.link.pos.y;
	gs.link.lives = 3*2; // srce moze biti na pola, 3 srca po pola
	volatile int draw_link = 0;
	volatile int draw_bg = 1;
	int started = 0;
	int draw_HUD = 0;
	int anim_done = 0;

	volatile int draw_sword = 0;
	int refresh_sword = 0; // da li smo prethodno bili nacrtali sword a sada moramo da update pozadinu za njega?
	int linkic_x=0;
	int size_y=0;
	int size_x=0;
	// spreci linkica da se crta po title-screen-u

	int counter = 0;

	// enemy anim step counter
	int enemy_counter = 0;
	int alter_axis_counter = 0;
	int alter_axis = 0;

	uint8_t last_link_draw = 0;//koristi se kao bool, da se samo jednom iscrta linkic posle pomeraja

	uint8_t current_tileX = 0;
	uint8_t current_tileY = 0;
	uint8_t* collision_screen;
	uint8_t* collision_screen_enemy;

	uint8_t number_of_enemies = 0;

	int has_sword = 0;
	int in_cave = 0;
	int delay_cave = 0;

	uint8_t draw_enemies[MAX_ENEMIES];
	uint8_t init_enemies = 1;
	int enemy_step_x[MAX_ENEMIES];
	int enemy_step_y[MAX_ENEMIES];
	for(int i = 0; i < MAX_ENEMIES;i++) {
		draw_enemies[i] = 0;
		enemy_step_x[i] = ENEMY_STEP;
		enemy_step_y[i] = ENEMY_STEP;
		enemies[i].anim.orientation_state = 0;
		enemies[i].dead = 0;
	}

	uint32_t old_screen;

	int imunity_cnt = 0;

	while(1){
		// animation counter
		if(gs.game_over) {
			game_over();
		}

		if(imunity_cnt > 0)
		{
			imunity_cnt--;
		}

		counter++;
		if(counter == ANIM_DELAY)
		{
			gs.link.anim.orientation_state ^= 1;
			for(int i = 0; i < MAX_ENEMIES; i++) {
				enemies[i].anim.orientation_state ^= 1;
			}
			counter = 0;
		}
		
		// enemy step counter
		enemy_counter++;
		if(enemy_counter == ENEMY_SPEED) {
			enemy_counter = 0;
			alter_axis_counter++;
			if(alter_axis_counter == 3*ENEMY_SPEED) {
				alter_axis_counter = 0;
				alter_axis ^= 1;
			}
		}
			
		
		/////////////////////////////////////
		// Poll controls.

		int mov_x = 0;
		int mov_y = 0;
	
		if(joypad.start && !started) {
			// neće sa start screena prelaziti na ovaj deo mape, ovo je samo test
			gs.current_screen = 7*OVERWORLD_MAPS_H + 7;
			old_screen = gs.current_screen;
			y_padding = Y_PADDING;
			draw_link = 1;
			// prebaci se na paletu za ekrane i linka
			for(uint8_t i = 0; i < 16; i++){
				palette_p32[i] = palette_tiles[i];
			}
			draw_bg = 1;
			started = 1;
			draw_HUD = 1;
		}
		/////////////////////////////////////
		// Gameplay.
		
		// Da ne prođe kroz ivice ekrana
		// Link lepo šeta po svim osama sada.
		

		if(started) {
			collision_screen = screens[gs.current_screen];//treba mi nogice slomiti
			number_of_enemies = enemies_array[gs.current_screen];
			if(joypad.left) {
				mov_x = -1;
				draw_link = 1;
				gs.link.anim.orientation = LEFT;
				current_tileX = collision_screen[((gs.link.pos.y - Y_PADDING+2)/TILE_SIZE)*TILES_H + (gs.link.pos.x+mov_x)/TILE_SIZE];
				current_tileY = collision_screen[((gs.link.pos.y - Y_PADDING + SPRITE_DIM-2)/TILE_SIZE)*TILES_H + (gs.link.pos.x+mov_x)/TILE_SIZE];
				last_link_draw = 1;
			}
			else if(joypad.right) {//razmisljam da pomerim y na sredinu kad se menja x? sta mislite nenogaci moji
				mov_x = +1;
				draw_link = 1;
				gs.link.anim.orientation = RIGHT;
				current_tileX =collision_screen[((gs.link.pos.y - Y_PADDING+2)/TILE_SIZE)*TILES_H + (gs.link.pos.x+SPRITE_DIM+mov_x)/TILE_SIZE];
				current_tileY = collision_screen[((gs.link.pos.y - Y_PADDING + SPRITE_DIM-2)/TILE_SIZE)*TILES_H + (gs.link.pos.x+mov_x + SPRITE_DIM)/TILE_SIZE];
				last_link_draw = 1;
			}
			else if(joypad.up) {
				mov_y = -1;
				draw_link = 1;
				gs.link.anim.orientation = UP;
				current_tileX = collision_screen[((mov_y + gs.link.pos.y - Y_PADDING)/TILE_SIZE)*TILES_H + (gs.link.pos.x+2)/TILE_SIZE];
				current_tileY = collision_screen[((mov_y + gs.link.pos.y - Y_PADDING)/TILE_SIZE)*TILES_H + (gs.link.pos.x + SPRITE_DIM-2)/TILE_SIZE];
				last_link_draw = 1;
			}
			else if(joypad.down) {
				mov_y = +1;
				draw_link = 1;
				gs.link.anim.orientation = DOWN;
				current_tileX = collision_screen[((mov_y + gs.link.pos.y - Y_PADDING + SPRITE_DIM)/TILE_SIZE)*TILES_H + (gs.link.pos.x+2)/TILE_SIZE];//lakse se nabada onda, nije frkica ako nogice budu blizu tile
				current_tileY = collision_screen[((mov_y + gs.link.pos.y - Y_PADDING + SPRITE_DIM)/TILE_SIZE)*TILES_H + (gs.link.pos.x + SPRITE_DIM-2)/TILE_SIZE];
				last_link_draw = 1;
			}
			else if((joypad.b) && (has_sword)) {
				draw_sword = 1;
			}
			else {
				if(gs.link.anim.orientation == LEFT) {
					gs.link.anim.orientation_state = 0;
				}
				else {
					gs.link.anim.orientation_state = 1;
				}
				if(last_link_draw) {//samo jednom ulazi ovde dok se ne pomeri ponovo
					draw_link = 1;
					last_link_draw = 0;
				}
			}

			if(current_tileX == 22 && current_tileY == 22) {
				if(!in_cave && (delay_cave == 15)) {
					in_cave = 1;
					has_sword = 1;
				}
				delay_cave++;
			}
			else {
				delay_cave = 0;
			}

			// prelaz s mape na mapu levo-desno
			if(mov_x + gs.link.pos.x < 0) {
				old_screen = gs.current_screen;
				gs.current_screen--;
				gs.link.pos.x = title_screen__w - SPRITE_DIM;
				draw_bg = 1;
				number_of_enemies = enemies_array[gs.current_screen];
				if(number_of_enemies)
					init_enemies = 1;
				else {
					for (int i = 0; i<MAX_ENEMIES;i++) {
						draw_enemies[i] = 0;
				}
			}
			}
			else if (mov_x + gs.link.pos.x >= title_screen__w - SPRITE_DIM) {
				old_screen = gs.current_screen;
				gs.current_screen++;
				gs.link.pos.x = 0;
				draw_bg = 1;
				number_of_enemies = enemies_array[gs.current_screen];
				if(number_of_enemies)
					init_enemies = 1;
				else {
					for (int i = 0; i<MAX_ENEMIES;i++) {
						draw_enemies[i] = 0;
				}
			}
			}
			else if(!check_collision(current_tileX, current_tileY)) {
				gs.link.pos.x += mov_x;
			}

			
			// prelaz s mape na mapu gore-dole
			if(mov_y + gs.link.pos.y < Y_PADDING){
				old_screen = gs.current_screen;
				gs.current_screen -= OVERWORLD_MAPS_H;
				gs.link.pos.y = title_screen__h - SPRITE_DIM - 9 - 1;
				draw_bg = 1;
				init_enemies = 1;
			}
			else if (mov_y + gs.link.pos.y > title_screen__h - 9 - SPRITE_DIM) {
				old_screen = gs.current_screen;
				gs.current_screen += OVERWORLD_MAPS_H;
				gs.link.pos.y = Y_PADDING;
				draw_bg = 1;
				init_enemies = 1;
				if(!in_cave) {
					has_sword = 1;
				}
			}
			else if(!check_collision(current_tileX, current_tileY)) {
				gs.link.pos.y += mov_y;
			}


			collision_screen_enemy = screens[gs.current_screen];
			if(init_enemies) {
				init_enemies = 0;
				int enemy_x = 16;
				int enemy_y = Y_PADDING;
				int col1 = 0;
				int col2 = 0;
				for(int i = 0; i < number_of_enemies;i++) {
					do
					{
						enemy_y += 16;
						col1 = collision_screen[((enemy_y - Y_PADDING)/TILE_SIZE)*TILES_H + (enemy_x)/TILE_SIZE];
						col2 = collision_screen[((enemy_y - Y_PADDING)/TILE_SIZE)*TILES_H + (enemy_x)/TILE_SIZE];
						if(check_collision(col1, col2))
						{	
							enemy_x += 16;
							col1 = collision_screen[((enemy_y - Y_PADDING)/TILE_SIZE)*TILES_H + (enemy_x)/TILE_SIZE];
						}
						else 
							break;
					} while (check_collision(col1, col2));

					enemies[i].pos.x = enemy_x;
					enemies[i].pos.y = enemy_y;
					enemies[i].dead = 0;
					draw_enemies[i] = 1;			
				}
			}
			if(enemies_array[gs.current_screen]) {
				int col1 = 0;
				int col2 = 0;
				for(int i = 0; i < number_of_enemies; i++) {
					if(enemies[i].dead)
						continue;

					if(enemy_step_x[i] < 0 && !alter_axis) {
						col1 = collision_screen[(( enemies[i].pos.y - Y_PADDING + 2)/TILE_SIZE)*TILES_H + (enemies[i].pos.x + enemy_step_x[i])/TILE_SIZE];
						col2 = collision_screen[(( enemies[i].pos.y - Y_PADDING + SPRITE_DIM - 2)/TILE_SIZE)*TILES_H + (enemies[i].pos.x + enemy_step_x[i])/TILE_SIZE];
						enemies[i].anim.orientation = LEFT;
					}
					else if (enemy_step_x[i] > 0 && !alter_axis) {
						col1 = collision_screen[((enemies[i].pos.y - Y_PADDING + 2)/TILE_SIZE)*TILES_H + (enemies[i].pos.x +  SPRITE_DIM + enemy_step_x[i])/TILE_SIZE];
						col2 = collision_screen[((enemies[i].pos.y - Y_PADDING + SPRITE_DIM - 2)/TILE_SIZE)*TILES_H + (enemies[i].pos.x + SPRITE_DIM + enemy_step_x[i])/TILE_SIZE];
						enemies[i].anim.orientation = RIGHT;
					}
					else if(enemy_step_y[i] < 0 && alter_axis) {
						col1 = collision_screen[((enemies[i].pos.y + enemy_step_y[i] - Y_PADDING)/TILE_SIZE)*TILES_H + (enemies[i].pos.x + 2)/TILE_SIZE];
						col2 = collision_screen[((enemies[i].pos.y + enemy_step_y[i] - Y_PADDING)/TILE_SIZE)*TILES_H + (enemies[i].pos.x + SPRITE_DIM - 2)/TILE_SIZE];
						enemies[i].anim.orientation = UP;
					}
					else if (enemy_step_y[i] > 0 && alter_axis) {
						col1 = collision_screen[((enemies[i].pos.y + enemy_step_y[i] - Y_PADDING + SPRITE_DIM)/TILE_SIZE)*TILES_H + (enemies[i].pos.x + 2)/TILE_SIZE];
						col2 = collision_screen[((enemies[i].pos.y + enemy_step_y[i] - Y_PADDING + SPRITE_DIM)/TILE_SIZE)*TILES_H + (enemies[i].pos.x + SPRITE_DIM - 2)/TILE_SIZE];
						enemies[i].anim.orientation = DOWN;
					}

					int collision = check_collision(col1, col2);

					
					if(!collision) {
						enemies[i].pos.x += enemy_step_x[i] * (enemy_counter ? 0 : 1) * (!alter_axis);
						enemies[i].pos.y += enemy_step_y[i] * (enemy_counter ? 0 : 1) * alter_axis;
					}

					if(enemy_counter) {
						enemy_step_x[i] *= (rand_generator() % 53 ? 1 : -1);
						enemy_step_y[i] *= (rand_generator() % 53 ? 1 : -1);
					}
					
					
					if(enemy_step_y[i] + enemies[i].pos.y < Y_PADDING){
						enemies[i].pos.y = Y_PADDING;
						enemy_step_y[i] = +ENEMY_STEP;
					}
					else if (enemy_step_y[i] + enemies[i].pos.y > title_screen__h - 9 - SPRITE_DIM) {
						enemies[i].pos.y = title_screen__h - 9 - SPRITE_DIM;
						enemy_step_y[i] = -ENEMY_STEP;
					}
					if(enemy_step_x[i] + (int)enemies[i].pos.x < 0){
						enemies[i].pos.x = 0;
						enemy_step_x[i] = +ENEMY_STEP;
					}
					else if (enemy_step_x[i] + enemies[i].pos.x > title_screen__w - SPRITE_DIM) {
						enemies[i].pos.x = title_screen__w - SPRITE_DIM;
						enemy_step_x[i] = -ENEMY_STEP;
					}

					draw_enemies[i] = 1;
				}
			}
			else {
				for (int i = 0; i<MAX_ENEMIES;i++) {
					draw_enemies[i] = 0;
				}
			}

		}
		
		
		check_interaction(draw_sword, enemies, &gs, &imunity_cnt, draw_enemies);
		
		/////////////////////////////////////
		// Drawing.
		
		
		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.
		
		
		if(draw_bg == 1 && draw_HUD == 1){
			for(uint16_t r1 = 0; r1 < SCREEN_IDX4_H; r1++){
				for(uint16_t c = 0; c < SCREEN_IDX4_W; c++){
					unpack_idx4_p32[r1*SCREEN_IDX4_W + c] = 0x00000000;
				}
			}

			init_HUD();
			draw_HUD = 0;
		}
		
		if(draw_bg == 1 && !started) {
			draw_background(
			title_screen__p, title_screen__w, title_screen__h - y_padding, 0, y_padding
			);
			draw_bg = 0;
		}

		if(in_cave == 1 && !anim_done && gs.current_screen == 7*OVERWORLD_MAPS_H + 7) {
			cave_animation(gs);
			gs.link.pos.x = 65;
			gs.link.pos.y = Y_PADDING+36;//35
			in_cave = 2;
			draw_bg = 1;
			draw_link = 1;
			gs.link.anim.orientation = DOWN;
			anim_done = 1;
		}

		if(draw_bg == 1 && started){
			draw_tiles(screens[gs.current_screen], tile_num_x, tile_num_y, 0, y_padding);
			update_minimap(old_screen, gs.current_screen);
			draw_bg = 0;
		}

		if(refresh_sword == 1 && !draw_sword) {
			
			update_background(
				screens[gs.current_screen], TILES_H,
				gs.link.old_pos.x,
				gs.link.old_pos.y - Y_PADDING,
				SPRITE_DIM, SPRITE_DIM,
				gs.link.old_pos.x,
				gs.link.old_pos.y
			);

			draw_sprite_from_atlas(
				link_sheet__p, link_sheet__w,
				gs.link.anim.orientation*LINK_ORIENATION_OFFSET,
				gs.link.anim.orientation_state*LINK_ORIENATION_OFFSET,
				SPRITE_DIM, SPRITE_DIM,
				gs.link.pos.x,
				gs.link.pos.y,
				1
			);

			refresh_sword = 0;
		}

		
		
		for(int i = 0; i < number_of_enemies;i++) {
			if(!enemy_counter && draw_enemies[i])
				update_background(
					screens[gs.current_screen], TILES_H,
					enemies[i].pos.x,
					enemies[i].pos.y - Y_PADDING,
					SPRITE_DIM, SPRITE_DIM,
					enemies[i].pos.x,
					enemies[i].pos.y
				);
			
		}

			

		if(draw_link == 1 && draw_sword != 1) {
			// Apdejtuj samo pozadinu oko Linka.
			update_background(
				screens[gs.current_screen], TILES_H,
				gs.link.old_pos.x,
				gs.link.old_pos.y - Y_PADDING,
				SPRITE_DIM, SPRITE_DIM,
				gs.link.old_pos.x,
				gs.link.old_pos.y
			);

			gs.link.old_pos.x = gs.link.pos.x;
			gs.link.old_pos.y = gs.link.pos.y;

			draw_sprite_from_atlas(
				link_sheet__p, link_sheet__w,
				gs.link.anim.orientation*LINK_ORIENATION_OFFSET,
				gs.link.anim.orientation_state*LINK_ORIENATION_OFFSET,
				SPRITE_DIM, SPRITE_DIM,
				gs.link.pos.x,
				gs.link.pos.y,
				1
			);

		}

		if(!enemy_counter && draw_sword != 1 && started)
		{
			draw_sprite_from_atlas(
				link_sheet__p, link_sheet__w,
				gs.link.anim.orientation*LINK_ORIENATION_OFFSET,
				gs.link.anim.orientation_state*LINK_ORIENATION_OFFSET,
				SPRITE_DIM, SPRITE_DIM,
				gs.link.pos.x,
				gs.link.pos.y,
				1
			);
		}


		if(draw_sword == 1) {

			uint16_t pos_x = gs.link.pos.x;
			uint16_t pos_y = gs.link.pos.y;

			int delta_x = 0;
			int delta_y = 0;

			if(gs.link.anim.orientation == DOWN) {
				linkic_x = 0;
				size_y =2 *SPRITE_DIM;
				size_x = SPRITE_DIM;
				if(pos_y + size_y > title_screen__h - 9) {
					size_y -= (pos_y + size_y - (title_screen__h - 9));
				}
			}
			else if(gs.link.anim.orientation == LEFT) {
				linkic_x = SPRITE_DIM + VOID_BETWEEN_LINKS;
				size_x = 2*SPRITE_DIM;
				size_y = SPRITE_DIM;
				if(pos_x > SPRITE_DIM) {
					pos_x -= SPRITE_DIM;
				} else {
					delta_x = SPRITE_DIM - pos_x;
					pos_x = 0;
				}
				
			}
			else if(gs.link.anim.orientation == UP) {
				linkic_x = 2*VOID_BETWEEN_LINKS + 3*SPRITE_DIM;
				size_x = SPRITE_DIM;
				size_y = 2*SPRITE_DIM;
				if(pos_y > SPRITE_DIM + Y_PADDING) {
					pos_y -= SPRITE_DIM;
				} else {
					delta_y = Y_PADDING + SPRITE_DIM - pos_y;
					pos_y = Y_PADDING;
				}
			}
			else {
				linkic_x = 4*SPRITE_DIM + 3*VOID_BETWEEN_LINKS;
				size_x = 2*SPRITE_DIM;
				size_y = SPRITE_DIM;
				if(pos_x + size_x > title_screen__w) {
					size_x -= ((pos_x + size_x) - title_screen__w);
				}
			}

			update_background(
				screens[gs.current_screen], TILES_H,
				gs.link.old_pos.x,
				gs.link.old_pos.y - Y_PADDING,
				SPRITE_DIM, SPRITE_DIM,
				gs.link.old_pos.x,
				gs.link.old_pos.y
			);



			draw_sprite_from_atlas(
				link_sheet__p, link_sheet__w,
				linkic_x + delta_x,
				LINK_ATTACK_Y + delta_y,
				size_x - delta_x, size_y - delta_y,
				pos_x,
				pos_y,
				1
			);
			
			refresh_sword = 1;
		}


		
		for(int i = 0; i < number_of_enemies;i++) {
			if(draw_enemies[i] && (!enemy_counter || draw_link || refresh_sword))
				draw_sprite_from_atlas(HUD_sprites__p, HUD_sprites__w, enemies[i].anim.orientation*SPRITE_DIM,FIRST_HUD_SIZE*6  + enemies[i].anim.orientation_state*SPRITE_DIM, FIRST_HUD_SIZE*2, FIRST_HUD_SIZE*2, enemies[i].pos.x, enemies[i].pos.y, 1);
		}
		draw_link = 0;
		draw_sword = 0;

	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
