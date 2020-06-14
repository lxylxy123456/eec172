# 
# eec172 - EEC 172 Final Project
# Copyright (C) 2020  lxylxy123456
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# 

import os, sys
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = "hide"
import random, pygame, time
from pygame.locals import *
from math import floor, ceil, sqrt

ZOOM = 5
FPS = 24
WINSIZE = [128 * ZOOM, 128 * ZOOM]

BLACK   = 0x0000
BLUE    = 0x001F
GREEN   = 0x07E0
CYAN    = 0x07FF
RED     = 0xF800
MAGENTA = 0xF81F
YELLOW  = 0xFFE0
WHITE   = 0xFFFF
GRAY    = 0x7bef

CHAR_W = 5
CHAR_H = 7
TRAY_C = WHITE
CHAR_C = WHITE
BRICK_COLORS = [RED, GREEN, YELLOW, CYAN, MAGENTA]

def translate_color(color) :
	'Adafruit color -> pygame color'
	if type(color) is int :
		return [(color&0xF800) >> 8, (color&0x07E0) >> 3, (color&0x001F) << 3]
	else :
		return color

def draw_point(x, y, color) :
	global screen
	c = translate_color(color)
	pygame.draw.rect(screen, c, pygame.Rect(ZOOM * x, ZOOM * y, ZOOM, ZOOM))
	# pygame.display.update()

def draw_circle(x, y, r, color) :
	c = translate_color(color)
	pygame.draw.circle(screen, c, (ZOOM * x, ZOOM * y), ZOOM * r)

def draw_rect(x, y, w, h, color) :
	for i in range(w) :
		for j in range(h) :
			draw_point(x + i, y + j, color)

def get_i2c() :
	return pygame.mouse.get_pos()[0] * 256 / WINSIZE[0] - 128

def draw_char(x, y, ch, color) :
	c = translate_color(color)
	font = pygame.font.Font(pygame.font.get_default_font(), 36)
	text_surface = font.render(ch, True, c)
	draw_rect(x, y, CHAR_W, CHAR_H, BLACK)
	screen.blit(text_surface, dest=(ZOOM * x, ZOOM * y))

def draw_str(x, y, s, color) :
	for index, i in enumerate(s) :
		draw_char(x + (index - len(s)//2) * CHAR_W, y, i, color)

BOARD_L = 10
BOARD_R = 80
BOARD_B = 10
BOARD_T = 110
BOARD_W = BOARD_R - BOARD_L
BOARD_H = BOARD_T - BOARD_B
BRICK_W = 10
BRICK_H = 5
NBRICK_X = BOARD_W // BRICK_W
NBRICK_Y = BOARD_H // BRICK_H
BALL_R = 3
BALL_C = BLUE
SPEED = 2
TRAY_W = 30
TRAY_H = 4
BORDER = 4
BORDER_C = GRAY
HSPEED_LIM = 4
STR_XL = 45		# mean(BOARD_L, BOARD_R)
STR_XR = 100	# mean(BOARD_R, 128)
STR_Y1 = 40
STR_Y2 = 50
STR_Y3 = 60
STR_Y4 = 70
TRAY_MASS = 0.12
TRAY_FRICTION = 0.1

PATTERNS = [
	[
		[3, 1, 0, 0, 0, 1, 3], 
		[2, 3, 1, 0, 1, 3, 2], 
		[4, 2, 3, 1, 3, 2, 4], 
		[1, 4, 2, 3, 2, 4, 1], 
		[3, 1, 4, 2, 4, 1, 3], # 5
		[0, 3, 1, 4, 1, 3, 0], 
		[0, 0, 3, 1, 3, 0, 0], 
		[1, 0, 0, 3, 0, 0, 1], 
		[3, 1, 0, 0, 0, 1, 3], 
		[2, 3, 1, 0, 1, 3, 2], # 10
		[4, 2, 3, 1, 3, 2, 4], 
		[0, 4, 2, 3, 2, 4, 0], 
		[0, 0, 4, 2, 4, 0, 0], 
		[0, 0, 0, 4, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], # 15
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], # 20
	], [
		[2, 3, 2, 3, 2, 3, 2], 
		[0, 0, 0, 0, 0, 0, 0], 
		[2, 3, 2, 3, 2, 3, 2], 
		[0, 0, 0, 0, 0, 0, 0], 
		[1, 4, 1, 4, 1, 4, 1], # 5
		[0, 0, 0, 0, 0, 0, 0], 
		[1, 4, 1, 4, 1, 4, 1], 
		[0, 0, 0, 0, 0, 0, 0], 
		[2, 3, 2, 3, 2, 3, 2], 
		[0, 0, 0, 0, 0, 0, 0], # 10
		[2, 3, 2, 3, 2, 3, 2], 
		[0, 0, 0, 0, 0, 0, 0], 
		[1, 4, 1, 4, 1, 4, 1], 
		[0, 0, 0, 0, 0, 0, 0], 
		[1, 4, 1, 4, 1, 4, 1], # 15
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], # 20
	], [
		[2, 3, 2, 3, 2, 3, 2], 
		[0, 2, 0, 2, 0, 2, 0], 
		[2, 3, 2, 3, 2, 3, 2], 
		[3, 0, 3, 0, 3, 0, 3], 
		[1, 4, 1, 4, 1, 4, 1], # 5
		[0, 1, 0, 1, 0, 1, 0], 
		[1, 4, 1, 4, 1, 4, 1], 
		[4, 0, 4, 0, 4, 0, 4], 
		[2, 3, 2, 3, 2, 3, 2], 
		[0, 2, 0, 2, 0, 2, 0], # 10
		[2, 3, 2, 3, 2, 3, 2], 
		[3, 0, 3, 0, 3, 0, 3], 
		[1, 4, 1, 4, 1, 4, 1], 
		[0, 1, 0, 1, 0, 1, 0], 
		[1, 4, 1, 4, 1, 4, 1], # 15
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], # 20
	], [
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 1, 2, 1, 2, 1, 0], 
		[0, 2, 1, 2, 1, 2, 0], 
		[0, 1, 2, 1, 2, 1, 0], 
		[0, 2, 1, 2, 1, 2, 0], # 5
		[0, 1, 2, 1, 2, 1, 0], 
		[0, 2, 1, 2, 1, 2, 0], 
		[0, 1, 2, 1, 2, 1, 0], 
		[0, 2, 1, 2, 1, 2, 0], 
		[0, 1, 2, 1, 2, 1, 0], # 10
		[0, 2, 1, 2, 1, 2, 0], 
		[0, 1, 2, 1, 2, 1, 0], 
		[0, 2, 1, 2, 1, 2, 0], 
		[0, 1, 2, 1, 2, 1, 0], 
		[0, 2, 1, 2, 1, 2, 0], # 15
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], # 20
	], [
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 1, 2, 3, 1, 2, 0], 
		[0, 2, 3, 1, 2, 3, 0], 
		[0, 3, 1, 2, 3, 1, 0], 
		[0, 1, 2, 3, 1, 2, 0], # 5
		[0, 2, 3, 1, 2, 3, 0], 
		[0, 3, 1, 2, 3, 1, 0], 
		[0, 1, 2, 3, 1, 2, 0], 
		[0, 2, 3, 1, 2, 3, 0], 
		[0, 3, 1, 2, 3, 1, 0], # 10
		[0, 1, 2, 3, 1, 2, 0], 
		[0, 2, 3, 1, 2, 3, 0], 
		[0, 3, 1, 2, 3, 1, 0], 
		[0, 1, 2, 3, 1, 2, 0], 
		[0, 2, 3, 1, 2, 3, 0], # 15
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], 
		[0, 0, 0, 0, 0, 0, 0], # 20
	],
]

def draw_brick(x, y, color) :
	bx = BOARD_L + x * BRICK_W
	by = BOARD_B + y * BRICK_H
	draw_rect(bx, by, BRICK_W, BRICK_H, color)

def draw_ball(x, y) :
	draw_circle(BOARD_L + int(x), BOARD_B + int(y), BALL_R, BALL_C)

def draw_tray(x, y, color) :
	tx = BOARD_L + x - TRAY_W // 2
	ty = BOARD_B + y - TRAY_H // 2
	draw_rect(tx, ty, TRAY_W, TRAY_H, color)

# Math functions
def norm(x, y) :
	return sqrt(x**2 + y**2)

def normalize(x, y) :
	tmp = sqrt(x**2 + y**2)
	return x / tmp, y / tmp

def dot_prod(x1, y1, x2, y2) :
	return x1 * x2 + y1 * y2

def map_mult(c, x, y) :
	return c * x, c * y

def map_plus(x1, y1, x2, y2) :
	return x1 + x2, y1 + y2

def map_minus(x1, y1, x2, y2) :
	return x1 - x2, y1 - y2

def map_neg(x, y) :
	return -x, -y

def refl_diff(x, y, nx, ny) :
	# nx and ny should be normalized before
	return map_mult(-2 * dot_prod(x, y, nx, ny), nx, ny)
	# return map_plus(x, y, *map_mult(2 * dot_prod(x, y, nx, ny), nx, ny))

def closest_corner(rx, ry, l, u, r, d) :
	assert l < r
	assert u < d
	return min(max(l, rx), r) - rx, min(max(u, ry), d) - ry

def init_bricks(nx, ny):
	nbricks = 0
	bricks = []
	if random.random() < 0.8 :
		ans = random.choice(PATTERNS)
		color_map = [BLACK] + random.sample(BRICK_COLORS, len(BRICK_COLORS))
		for i in range(nx):
			bricks.append([])
			for j in range(ny):
				nbricks += bool(ans[j][i])
				bricks[-1].append(color_map[ans[j][i]])
	else :
		for i in range(nx):
			bricks.append([])
			for j in range(ny):
				if random.random() < 0.5 and j < 15 :
					nbricks += 1
					bricks[-1].append(random.choice(BRICK_COLORS))
				else :
					bricks[-1].append(BLACK)
	return nbricks, bricks

def game(score):
	'Take in previous score, return new score, negative means keep playing'
	global screen
	clock = pygame.time.Clock()
	pygame.init()
	screen = pygame.display.set_mode(WINSIZE)
	pygame.display.set_caption('EEC 172 Final Project')
	tx, ty = BOARD_W // 2, BOARD_H
	tx_old = tx
	rx, ry = tx, ty - BALL_R - TRAY_H//2
	vx, vy = map_mult(SPEED, *normalize(random.random() * 3 - 1.5, -1))
	nbricks, bricks = init_bricks(NBRICK_X, NBRICK_Y)
	draw_rect(0, 0, 128, 128, BLACK)
	draw_rect(BOARD_L - BORDER, BOARD_B, BORDER, BOARD_H + BORDER, BORDER_C)
	draw_rect(BOARD_R, BOARD_B, BORDER, BOARD_H + BORDER, BORDER_C)
	draw_rect(BOARD_L - BORDER, BOARD_B - BORDER, BOARD_W + 2 * BORDER, BORDER,
				BORDER_C)
	draw_tray(tx, ty, TRAY_C)
	for i in range(NBRICK_X) :
		for j in range(NBRICK_Y) :
			if bricks[i][j] :
				draw_brick(i, j, bricks[i][j])
	draw_str(STR_XR, STR_Y1, 'SCORE', CHAR_C)
	draw_str(STR_XR, STR_Y2, str(score), CHAR_C)
	draw_str(STR_XR, STR_Y3, 'MAX', CHAR_C)
	draw_str(STR_XR, STR_Y4, '9999', CHAR_C)
	paused = False
	while True:
		if not paused :
			xs = floor((rx - BALL_R) / BRICK_W)
			xe = floor((rx + BALL_R) / BRICK_W)
			ys = floor((ry - BALL_R) / BRICK_H)
			ye = floor((ry + BALL_R) / BRICK_H)
			refl_x = False
			refl_y = False
			refl_tray = False
			coll_list = []
			# Collision with tray
			# Requires |v| < TRAY_H / 2
			dx, dy = closest_corner(rx, ry, tx - TRAY_W//2, ty - TRAY_H//2,
											tx + TRAY_W//2, ty + TRAY_H//2)
			if dx**2 + dy**2 < BALL_R**2 :
				refl_tray = True
				refl_y = True
			# Collision with border and brick
			for i in range(xs, xe + 1) :
				for j in range(ys, ye + 1) :
					if i < 0 or i >= NBRICK_X :
						refl_x = True
						continue
					if j < 0 :
						refl_y = True
						continue
					if j >= NBRICK_Y :
						if refl_tray :
							continue
						return score
					if not bricks[i][j] :
						continue
					dx, dy = closest_corner(rx, ry, i * BRICK_W, j * BRICK_H,
											(i+1) * BRICK_W, (j+1) * BRICK_H)
					if dx**2 + dy**2 > BALL_R**2 :
						continue
					bricks[i][j] = False
					nbricks -= 1
					score += 1
					draw_brick(i, j, BLACK)
					if abs(dy) == 0 :
						refl_x = True
					elif abs(dx) == 0 :
						refl_y = True
					else :
						coll_list.append((i, j))
			if refl_x :
				vx = -vx
			if refl_y :
				vy = -vy
			if not refl_x and not refl_y and coll_list :
				# Require diameter of ball < brick
				total_diff = 0, 0
				for (i, j) in coll_list :
					dx, dy = closest_corner(rx, ry, i * BRICK_W, j * BRICK_H,
											(i+1) * BRICK_W, (j+1) * BRICK_H)
					dx, dy = normalize(dx, dy)
					total_diff = map_plus(*total_diff,
											*refl_diff(vx, vy, dx, dy))
				# print(total_diff, coll_list)
				v = map_plus(vx, vy, *total_diff)
				vx, vy = map_mult(SPEED, *normalize(*v))
			if refl_tray and not refl_x :
				# Add Tray velocity to vx
				vx, vy = map_mult(SPEED,
							*normalize(vx + (tx - tx_old - vx) * TRAY_FRICTION,
										vy))
			if abs(vy) * HSPEED_LIM < abs(vx) :
				# Prevent just moving horizontally
				sgn = lambda x: (x > 0) * 2 - 1
				vx, vy = map_mult(SPEED,
									*normalize(sgn(vx) * HSPEED_LIM, sgn(vy)))
			if refl_tray and vy > 0 :
				# Prevent sticking onto the tray
				vy = -vy
			old_rx, old_ry = rx, ry
			rx += vx
			ry += vy
			draw_str(STR_XR, STR_Y2, str(score), CHAR_C)
			for x in range(int(old_rx) - BALL_R, int(old_rx) + BALL_R) :
				for y in range(int(old_ry) - BALL_R, int(old_ry) + BALL_R) :
					i = floor(x / BRICK_W)
					j = floor(y / BRICK_H)
					if x in range(tx - TRAY_W // 2, tx + TRAY_W // 2) and \
						y in range(ty - TRAY_H // 2, ty + TRAY_H // 2) :
						color = TRAY_C
					elif x < 0 or x >= BOARD_W or y < 0 :
						color = BORDER_C
					elif y >= BOARD_H :
						color = BLACK
					elif bricks[i][j] :
						color = bricks[i][j]
					else :
						color = BLACK
					draw_point(BOARD_L + x, BOARD_B + y, color)
			if tx_old < tx :
				draw_rect(BOARD_L + tx_old - TRAY_W // 2, 
						BOARD_B + ty - TRAY_H // 2, tx - tx_old, TRAY_H, BLACK)
				draw_rect(BOARD_L + tx_old + TRAY_W // 2, 
						BOARD_B + ty - TRAY_H // 2, tx - tx_old, TRAY_H, TRAY_C)
			if tx_old > tx :
				draw_rect(BOARD_L + tx + TRAY_W // 2, 
						BOARD_B + ty - TRAY_H // 2, tx_old - tx, TRAY_H, BLACK)
				draw_rect(BOARD_L + tx - TRAY_W // 2, 
						BOARD_B + ty - TRAY_H // 2, tx_old - tx, TRAY_H, TRAY_C)
			draw_ball(rx, ry)
			pygame.display.update()
			if nbricks == 0 :
				return -score
			
			tv = get_i2c() * TRAY_MASS
			tx_old = tx
			tx = min(max(int(tx + tv), TRAY_W//2), BOARD_W - TRAY_W//2)
		
		for e in pygame.event.get():
			if e.type == KEYUP and e.key == K_LEFT:
				tx = max(tx - 9, TRAY_W//2)
			if e.type == KEYUP and e.key == K_RIGHT:
				tx = min(tx + 9, BOARD_W - TRAY_W//2)
			if e.type == KEYUP and e.key == K_2 :
				paused = not paused
			if e.type == KEYUP and e.key == K_3 :
				return score
			if e.type == MOUSEBUTTONDOWN and e.button == 1:
				pass
		clock.tick(FPS)

def project_main() :
	while True :
		score = 0
		while True:
			score = game(-score)
			if score >= 0 :
				break
		draw_str(STR_XL, STR_Y1, "Game Over", TRAY_C)
		draw_str(STR_XL, STR_Y2, "Press SW2/SW3", TRAY_C)
		draw_str(STR_XL, STR_Y3, "to restart", TRAY_C)
		pygame.display.update()
		pressed = None
		while not pressed :
			for e in pygame.event.get() :
				if e.type == QUIT or (e.type == KEYUP and e.key == K_ESCAPE):
					return
				if e.type == KEYUP and e.key == K_2 :
					pressed = 2
				if e.type == KEYUP and e.key == K_3 :
					pressed = 3
			time.sleep(0.1)

def print_patterns() :
	for i in PATTERNS :
		print(end='{')
		for jndex, j in enumerate(i) :
			if jndex >= 15 :
				assert j == [0, 0, 0, 0, 0, 0, 0]
				continue
			end = ',' if jndex != 14 else ''
			print('0x0' + ''.join(map(str, j)), end=end)
		print('},')

def main() :
	if '-p' in sys.argv[1:] :
		# Print patterns
		return print_patterns()
	return project_main()

if __name__ == '__main__' :
	main()

