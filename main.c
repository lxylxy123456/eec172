//  
//  eec172 - EEC 172 Final Project
//  Copyright (C) 2020  lxylxy123456
//  
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  License, or (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//  
//  You should have received a copy of the GNU Affero General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//  

// https://en.wikibooks.org/wiki/X_Window_Programming/Xlib

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include <X11/Xlib.h>

Display *display;
Window window;
XEvent event;
int screen;

enum {
	ZOOM = 5,
	WIN_X = 10,
	WIN_Y = 10,
	WIN_WIDTH = 128 * ZOOM,
	WIN_HEIGHT = 128 * ZOOM,
	WIN_BORDER = 1,

	K_2 = 11,
	K_3 = 12,
	K_ESC = 9
};

#define BLACK   0x0000
#define CHAR_W 6
#define CHAR_H 8

int msleep(long msec)
{
	// https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep
	struct timespec ts;
	int res;
	if (msec < 0)
		return -1;
	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	do {
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);
	return res;
	}

typedef short color_t;
typedef short oled_t;

void set_color(color_t color) {
	// Adafruit color -> Xlib color -> XSetForeground
	XSetForeground(display, DefaultGC(display, screen),
					((0xF800 & color) << 8) | 
					((0x07E0 & color) << 5) | 
					((0x001F & color) << 3));
}

void draw_point(oled_t x, oled_t y, color_t color) {
	set_color(color);
	XFillRectangle(display, window, DefaultGC(display, screen),
					x * ZOOM, y * ZOOM, ZOOM, ZOOM);
}

void draw_circle(oled_t x, oled_t y, oled_t r, color_t color) {
	set_color(color);
	XFillArc(display, window, DefaultGC(display, screen),
			(x - r) * ZOOM, (y - r) * ZOOM, (2 * r) * ZOOM, (2 * r) * ZOOM,
			0, 360 * 64);
}

void draw_rect(oled_t x, oled_t y, oled_t w, oled_t h, color_t color) {
	for (oled_t i = 0; i < w; i++)
		for (oled_t j = 0; j < h; j++)
			draw_point(x + i, y + j, color);
}

char get_i2c() {
	Window root_ret, child_ret;
	int root_x, root_y, win_x, win_y;
	unsigned int mask_ret;
	XQueryPointer(display, window, &root_ret, &child_ret,
					&root_x, &root_y, &win_x, &win_y, &mask_ret);
	// printf("get_i2c %d %d %d %d\n", root_x, root_y, win_x, win_y);
	return win_x * 256 / WIN_WIDTH - 128;
}

float get_random_float() {
	// Return random number in [0, 1)
	return (float) rand() / ((float) RAND_MAX + 1);
}

int get_random_int(int upper_bound) {
	// Return random number in [0, upper bound)
	return rand() % upper_bound;	// not perfect
}

#include "glcdfont.h"

void draw_char(oled_t x, oled_t y, char ch, color_t color) {
	color_t c;
	for (oled_t i = 0; i < 6; i++) {
		for (oled_t j = 0; j < 8; j++) {
			if (i == 5)
				c = BLACK;
			else
				c = (((font[ch * 5 + i]) >> (j)) & 1) ? color : BLACK;
			draw_point(x + i, y + j, c);
		}
	}
}

void draw_char_(oled_t x, oled_t y, char ch, color_t color) {
	XTextItem item;
	item.chars = &ch;
	item.nchars = 1;
	item.delta = 0;
	item.font = None;
	draw_rect(x, y, CHAR_W, CHAR_H, BLACK);
	set_color(color);
	XDrawText(display, window, DefaultGC(display, screen),
				x * ZOOM, y * ZOOM + CHAR_H + 3, &item, 1);
}

int get_max_score() {
	FILE* f = fopen("/tmp/eec172_max_score.txt", "r");
	msleep(1000);
	if (!f)
		return 0;
	int score;
	fscanf(f, "%d", &score);
	return score;
}

void send_score(int score) {
	FILE* f = fopen("/tmp/eec172_max_score.txt", "w");
	msleep(1000);
	fprintf(f, "%d\n", score);
}

// Theoreotically portable code starts

#define BLACK   0x0000
#define BLUE    0x001F
#define GREEN   0x07E0
#define CYAN    0x07FF
#define RED     0xF800
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GRAY    0x7bef

#define CHAR_W 6
#define CHAR_H 8
#define TRAY_C WHITE
#define CHAR_C WHITE
#define HIGH_C RED
const color_t BRICK_COLORS[] = {RED, GREEN, YELLOW, CYAN, MAGENTA};
#define NBRICK_COLORS (sizeof(BRICK_COLORS) / sizeof(BRICK_COLORS[0]))

#define BOARD_L 10
#define BOARD_R 80
#define BOARD_B 10
#define BOARD_T 110
#define BOARD_W (BOARD_R - BOARD_L)
#define BOARD_H (BOARD_T - BOARD_B)
#define BRICK_W 10
#define BRICK_H 5
#define NBRICK_X (BOARD_W / BRICK_W)
#define NBRICK_Y (BOARD_H / BRICK_H)
#define BALL_R 3
#define BALL_C BLUE
#define SPEED 2
#define TRAY_W 30
#define TRAY_H 4
#define BORDER 4
#define BORDER_C GRAY
#define HSPEED_LIM 4
#define STR_XL 45		// mean(BOARD_L, BOARD_R)
#define STR_XR 106		// mean(BOARD_R + BORDER, 128)
#define STR_Y1 40
#define STR_Y2 50
#define STR_Y3 60
#define STR_Y4 70
#define TRAY_MASS 0.12
#define TRAY_FRICTION 0.1
#include "pattern.h"

#define COLL_LIST_N 10

void draw_brick(int x, int y, color_t color) {
	oled_t bx = BOARD_L + x * BRICK_W;
	oled_t by = BOARD_B + y * BRICK_H;
	draw_rect(bx, by, BRICK_W, BRICK_H, color);
}

void draw_ball(float x, float y) {
	draw_circle(BOARD_L + (int)x, BOARD_B + (int)y, BALL_R, BALL_C);
}

void draw_tray(float x, float y, color_t color) {
	oled_t tx = BOARD_L + x - TRAY_W / 2;
	oled_t ty = BOARD_B + y - TRAY_H / 2;
	draw_rect(tx, ty, TRAY_W, TRAY_H, color);
}

void draw_str(oled_t x, oled_t y, char* s, color_t color, char* cache) {
	// draw n chars
	const int n = strlen(s);
	oled_t cx = x - n * CHAR_W / 2;
	oled_t cy = y - CHAR_H / 2;
	if (cache && strlen(cache) == n) {
		for (int i = 0; i < n; i++, cx += CHAR_W)
			if (s[i] != cache[i])
				draw_char(cx, cy, s[i], color);
	} else {
		for (int i = 0; i < n; i++, cx += CHAR_W)
			draw_char(cx, cy, s[i], color);
	}
	if (cache)
		strncpy(cache, s, n);
}

// Math functions
// lapacke.h
#define MAX(x,  y)   (((x) > (y)) ? (x) : (y))
#define MIN(x,  y)   (((x) < (y)) ? (x) : (y))

typedef struct { float x; float y; } vect_t;

vect_t normalize(vect_t xy) {
	float n = sqrtf(xy.x*xy.x + xy.y*xy.y);
	return (vect_t){xy.x / n, xy.y / n};
}

float dot_prod(vect_t xy1, vect_t xy2) {
	return xy1.x * xy2.x + xy1.y * xy2.y;
}

vect_t map_mult(float c, vect_t xy) {
	return (vect_t){c * xy.x, c * xy.y};
}

vect_t map_plus(vect_t xy1, vect_t xy2) {
	return (vect_t){xy1.x + xy2.x, xy1.y + xy2.y};
}

vect_t map_minus(vect_t xy1, vect_t xy2) {
	return (vect_t){xy1.x - xy2.x, xy1.y - xy2.y};
}

vect_t map_neg(vect_t xy) {
	return (vect_t){-xy.x, -xy.y};
}

vect_t refl_diff(vect_t xy, vect_t nxy) {
	// nx and ny should be normalized before
	return map_mult(-2 * dot_prod(xy, nxy), nxy);
}

vect_t closest_corner(vect_t rxy, float l, float u, float r, float d) {
	assert(l < r);
	assert(u < d);
	return (vect_t){MIN(MAX(l, rxy.x), r) - rxy.x,
					MIN(MAX(u, rxy.y), d) - rxy.y};
}

void get_random_perm(int n, int* ans) {
	// Rand permutation in range(1, n + 1)
	for (int i = 0; i < n; i++) {
		int cnt = get_random_int(n - i);
		for (int j = 0; j < n; j++) {
			for (int k = 0; k < i; k++) {
				if (ans[k] == j)
					goto get_random_perm_found;
			}
			if (!(cnt--)) {
				ans[i] = j;
				goto get_random_perm_decided;
			}
			get_random_perm_found:;
		}
		assert(0);
		get_random_perm_decided:;
	}
}

int init_bricks(int nx, int ny, color_t bricks[nx][ny]) {
	// Initialize bricks, return number of bricks
	int nbricks = 0;
	if (get_random_int(10) < 8) {
		int choice = get_random_int(sizeof(PATTERNS) / sizeof(PATTERNS[0]));
		int perm[NBRICK_COLORS];
		get_random_perm(NBRICK_COLORS, perm);
		for (int j = 0; j < ny; j++) {
			int line = PATTERNS[choice][j];
			for (int i = 0; i < nx; i++) {
				int tmp;
				if (j >= 15)
					bricks[i][j] = BLACK;
				else if (!(tmp = ((line >> (i * 4)) & 0xf)))
					bricks[i][j] = BLACK;
				else {
					bricks[i][j] = BRICK_COLORS[perm[tmp - 1]];
					nbricks++;
				}
			}
		}
	} else {
		for (int i = 0; i < nx; i++) {
			for (int j = 0; j < ny; j++) {
				if (j < 15 && get_random_int(10) < 5) {
					bricks[i][j] = BRICK_COLORS[get_random_int(NBRICK_COLORS)];
					nbricks++;
				} else {
					bricks[i][j] = BLACK;
				}
			}
		}
	}
	return nbricks;
}

int game(int prev_score, int max_score) {
	int score = prev_score;
	vect_t txy = {BOARD_W / 2, BOARD_H};
	float tx_old = txy.x;
	vect_t rxy = {txy.x, txy.y - BALL_R - TRAY_H / 2};
	vect_t vxy = map_mult(SPEED, normalize((vect_t){
				get_random_float() * 3 - 1.5, -1}));
	color_t bricks[NBRICK_X][NBRICK_Y];
	int nbricks = init_bricks(NBRICK_X, NBRICK_Y, bricks);
	draw_rect(0, 0, 128, 128, BLACK);
	draw_rect(BOARD_L - BORDER, BOARD_B, BORDER, BOARD_H + BORDER, BORDER_C);
	draw_rect(BOARD_R, BOARD_B, BORDER, BOARD_H + BORDER, BORDER_C);
	draw_rect(BOARD_L - BORDER, BOARD_B - BORDER, BOARD_W + 2 * BORDER, BORDER,
				BORDER_C);
	draw_tray(txy.x, txy.y, TRAY_C);
	for (int i = 0; i < NBRICK_X; i++)
		for (int j = 0; j < NBRICK_Y; j++)
			if (bricks[i][j])
				draw_brick(i, j, bricks[i][j]);
	char score_str[20], score_cache[20] = "";
	draw_str(STR_XR, STR_Y1, "SCORE", CHAR_C, NULL);
	sprintf(score_str, "%d", score);
	draw_str(STR_XR, STR_Y2, score_str, CHAR_C, score_cache);
	draw_str(STR_XR, STR_Y3, "MAX", CHAR_C, NULL);
	sprintf(score_str, "%d", max_score);
	draw_str(STR_XR, STR_Y4, score_str, CHAR_C, NULL);
	int paused = 0;
	int count_down = 31;
	while (1) {
		vect_t old_rxy = rxy;
		if (!paused && !count_down) {
			// Calculate new ball position and brick collision etc
			const int xs = floorf((rxy.x - BALL_R) / BRICK_W);
			const int xe = floorf((rxy.x + BALL_R) / BRICK_W);
			const int ys = floorf((rxy.y - BALL_R) / BRICK_H);
			const int ye = floorf((rxy.y + BALL_R) / BRICK_H);
			int refl_x = 0;
			int refl_y = 0;
			int refl_tray = 0;
			int coll_list_x[COLL_LIST_N], coll_list_y[COLL_LIST_N];
			int coll_list_n = 0;
			// Collision with tray
			// Requires |v| < TRAY_H / 2
			vect_t dxy = closest_corner(rxy, txy.x - TRAY_W/2, txy.y - TRAY_H/2,
											txy.x + TRAY_W/2, txy.y + TRAY_H/2);
			if (dxy.x * dxy.x + dxy.y * dxy.y < BALL_R * BALL_R) {
				refl_tray = 1;
				refl_y = 1;
			}
			// Collision with border and brick
			for (int i = xs; i <= xe; i++) {
				for (int j = ys; j <= ye; j++) {
					if (i < 0 || i >= NBRICK_X) {
						refl_x = 1;
						continue;
					}
					if (j < 0) {
						refl_y = 1;
						continue;
					}
					if (j >= NBRICK_Y) {
						if (refl_tray)
							continue;
						return score;
					}
					if (!bricks[i][j])
						continue;
					vect_t dxy = closest_corner(rxy, i * BRICK_W, j * BRICK_H,
											(i+1) * BRICK_W, (j+1) * BRICK_H);
					if (dxy.x * dxy.x + dxy.y * dxy.y > BALL_R * BALL_R)
						continue;
					bricks[i][j] = 0;
					nbricks--;
					score++;
					draw_brick(i, j, BLACK);
					if (dxy.y == 0)
						refl_x = 1;
					else if (dxy.x == 0)
						refl_y = 1;
					else {
						coll_list_x[coll_list_n] = i;
						coll_list_y[coll_list_n++] = j;
						assert(coll_list_n < COLL_LIST_N);
					}
				}
			}
			if (refl_x)
				vxy.x = -vxy.x;
			if (refl_y)
				vxy.y = -vxy.y;
			if (!refl_x && !refl_y && coll_list_n) {
				// Require diameter of ball < brick
				vect_t total_diff = {0, 0};
				for (int index = 0; index < coll_list_n; index++) {
					int i = coll_list_x[index], j = coll_list_y[index];
					dxy = closest_corner(rxy, i * BRICK_W, j * BRICK_H,
										(i+1) * BRICK_W, (j+1) * BRICK_H);
					dxy = normalize(dxy);
					total_diff = map_plus(total_diff, refl_diff(vxy, dxy));
				}
				// print(total_diff, coll_list)
				vxy = map_mult(SPEED, normalize(map_plus(vxy, total_diff)));
			}
			if (refl_tray && !refl_x) {
				// Add tray velocity to vx
				vxy.x += (txy.x - tx_old - vxy.x) * TRAY_FRICTION;
				vxy = map_mult(SPEED, normalize(vxy));
			}
			if (fabsf(vxy.y) * HSPEED_LIM < fabsf(vxy.x)) {
				// Prevent just moving horizontally
				#define SGN(x) ((x) > 0 ? 1 : -1)
				vxy = map_mult(SPEED, normalize((vect_t){
						SGN(vxy.x) * HSPEED_LIM, SGN(vxy.y)}));
			}
			if (refl_tray && vxy.y > 0) {
				// Prevent sticking onto the tray
				vxy.y = -vxy.y;
			}
			rxy = map_plus(rxy, vxy);
		}
		if (!paused && count_down) {
			// Count down
			switch(count_down) {
			case 31:
				draw_str(STR_XL, STR_Y2, "3", CHAR_C, NULL);
				break;
			case 21:
				draw_str(STR_XL, STR_Y2, "2", CHAR_C, NULL);
				break;
			case 11:
				draw_str(STR_XL, STR_Y2, "1", CHAR_C, NULL);
				break;
			case 1:
				draw_str(STR_XL, STR_Y2, "0", CHAR_C, NULL);
				break;
			// no default
			}
		}
		if (!paused) {
			// Draw update
			sprintf(score_str, "%d", score);
			draw_str(STR_XR, STR_Y2, score_str, CHAR_C, score_cache);
			int txy_x = txy.x, txy_y = txy.y, tx_old_i = tx_old;
			if (tx_old < txy.x) {
				draw_rect(BOARD_L + tx_old_i - TRAY_W / 2, 
						BOARD_B + txy.y - TRAY_H / 2, txy_x - tx_old_i, 
						TRAY_H, BLACK);
				draw_rect(BOARD_L + tx_old_i + TRAY_W / 2, 
						BOARD_B + txy.y - TRAY_H / 2, txy_x - tx_old_i, 
						TRAY_H, TRAY_C);
			}
			if (tx_old > txy.x) {
				draw_rect(BOARD_L + txy_x + TRAY_W / 2, 
						BOARD_B + txy.y - TRAY_H / 2, tx_old_i - txy_x, 
						TRAY_H, BLACK);
				draw_rect(BOARD_L + txy_x - TRAY_W / 2, 
						BOARD_B + txy.y - TRAY_H / 2, tx_old_i - txy_x, 
						TRAY_H, TRAY_C);
			}
			oled_t ux1, ux2, uy1, uy2;
			if (count_down == 1) {
				ux1 = STR_XL - BOARD_L - CHAR_W / 2;
				ux2 = STR_XL - BOARD_L + CHAR_W / 2;
				uy1 = STR_Y2 - BOARD_B - CHAR_H / 2;
				uy2 = STR_Y2 - BOARD_B + CHAR_H / 2;
			} else {
				ux1 = (oled_t) MIN(old_rxy.x, rxy.x) - BALL_R;
				ux2 = (oled_t) MAX(old_rxy.x, rxy.x) + BALL_R;
				uy1 = (oled_t) MIN(old_rxy.y, rxy.y) - BALL_R;
				uy2 = (oled_t) MAX(old_rxy.y, rxy.y) + BALL_R;			
			}
			for (oled_t x = ux1; x <= ux2; x++) {
				for (oled_t y = uy1; y <= uy2; y++) {
					int i = x / BRICK_W;
					int j = y / BRICK_H;
					color_t color;
					if (txy_x - TRAY_W / 2 <= x && txy_x + TRAY_W / 2 > x &&
						txy_y - TRAY_H / 2 <= y && txy_y + TRAY_H / 2 > y) {
						color = TRAY_C;
					} else if (x < 0 || x >= BOARD_W || y < 0) {
						color = BORDER_C;
					} else if ((x - rxy.x) * (x - rxy.x) + \
								(y - rxy.y) * (y - rxy.y) < BALL_R * BALL_R) {
						color = BALL_C;
					} else if (y >= BOARD_H) {
						color = BLACK;
					} else {
						color = bricks[i][j];
					}
					draw_point(BOARD_L + x, BOARD_B + y, color);
				}
			}
			if (nbricks == 0)
				return -score;
			float tv;
			if (count_down)
				tv = 0;
			else
				tv = get_i2c() * TRAY_MASS;
			tx_old = txy.x;
			txy.x = MIN(MAX(txy.x + tv, TRAY_W/2), BOARD_W - TRAY_W/2);
		}
		if (!paused && count_down)
			count_down--;
		while (XPending(display)) {
			XNextEvent(display, &event);
			switch (event.type) {
				case KeyPress:
					switch (((XKeyEvent*)&event)->keycode) {
						case K_2:
							paused = !paused;
							break;
						case K_3:
							return score;
						case K_ESC:
							exit(0);
						default:
							printf("%d\n", ((XKeyEvent*)&event)->keycode);
					}
					break;
				/* NO DEFAULT */
			}
		}
		XFlush(display);
		msleep(40);
		// TODO: clock.tick(FPS)
	}
	return prev_score;
}

void project_main() {
	int max_score = get_max_score();
	while (1) {
		int score = 0;
		do {
			score = game(-score, max_score);
		} while(score < 0);
		draw_str(STR_XL, STR_Y1, "Game Over", CHAR_C, NULL);
		draw_str(STR_XL, STR_Y3, "Sending", CHAR_C, NULL);
		draw_str(STR_XL, STR_Y4, "score", CHAR_C, NULL);
		if (score > max_score) {
			draw_str(STR_XL, STR_Y2, "HIGH SCORE!", HIGH_C, NULL);
			max_score = score;
		}
		XFlush(display);
		send_score(score);
		draw_str(STR_XL, STR_Y3, "Press SW2/SW3", CHAR_C, NULL);
		draw_str(STR_XL, STR_Y4, "to restart", CHAR_C, NULL);
		while (1) {
			while (!XPending(display))
				msleep(100);
			XNextEvent(display, &event);
			switch (event.type) {
				case KeyPress:
					switch (((XKeyEvent*)&event)->keycode) {
						case K_2:
						case K_3:
							goto new_game;
						case K_ESC:
							return;
						default:
							printf("%d\n", ((XKeyEvent*)&event)->keycode);
					}
					break;
				case ClientMessage:
					return;
				case Expose:
					/* draw the window */
					project_main();
				/* NO DEFAULT */
			}
		}
		new_game:;
	}
}

int main() {
	/* open connection with the server */
	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	screen = DefaultScreen(display);

	/* create window */
	window = XCreateSimpleWindow(display, RootWindow(display, screen), 
		WIN_X, WIN_Y, WIN_WIDTH, WIN_HEIGHT, WIN_BORDER, 
		BlackPixel(display, screen), WhitePixel(display, screen));
	XStoreName(display, window, "EEC 172 Final Project");

	/* process window close event */
	Atom del_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(display, window, &del_window, 1);

	/* select kind of events we are interested in */
	XSelectInput(display, window, ExposureMask | KeyPressMask);

	/* display the window */
	XMapWindow(display, window);

	/* event loop */
	while (1) {
		if (XPending(display)) {
			XNextEvent(display, &event);

			switch (event.type) {
				case KeyPress:
					assert(0);
				case ClientMessage:
					assert(0);
				case Expose:
					project_main();
					goto main_exit;
				/* NO DEFAULT */
			}
		}
	}
	main_exit:;
	/* destroy window */
	XDestroyWindow(display, window);

	/* close connection to server */
	XCloseDisplay(display);

	return 0;
}
