/* Copyright (C) 2017  Curtis McEnroe <june@causal.agency>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _XOPEN_SOURCE_EXTENDED

#include <curses.h>
#include <err.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <sysexits.h>
#include <wchar.h>

#include "torus.h"

static void curse(void) {
	setlocale(LC_CTYPE, "");

	initscr();
	start_color();

	assume_default_colors(0, 0);
	if (COLORS >= 16) {
		for (short pair = 1; pair < 0x80; ++pair) {
			init_pair(pair, pair & 0x0F, (pair & 0xF0) >> 4);
		}
	} else {
		for (short pair = 1; pair < 0100; ++pair) {
			init_pair(pair, pair & 007, (pair & 070) >> 3);
		}
	}

	color_set(ColorWhite, NULL);
	mvhline(CellRows, 0, 0, CellCols);
	mvhline(CellRows * 2 + 1, 0, 0, CellCols);
	mvvline(0, CellCols, 0, CellRows * 2 + 1);
	mvaddch(CellRows, CellCols, ACS_RTEE);
	mvaddch(CellRows * 2 + 1, CellCols, ACS_LRCORNER);
	color_set(0, NULL);

	cbreak();
	noecho();
	keypad(stdscr, true);
	set_escdelay(100);
}

static attr_t colorAttr(uint8_t color) {
	if (COLORS >= 16) return A_NORMAL;
	return (color & ColorBright) ? A_BOLD : A_NORMAL;
}
static short colorPair(uint8_t color) {
	if (COLORS >= 16) return color;
	return (color & 0x70) >> 1 | (color & 0x07);
}

static void drawTile(int offsetY, const struct Tile *tile) {
	for (uint8_t cellY = 0; cellY < CellRows; ++cellY) {
		for (uint8_t cellX = 0; cellX < CellCols; ++cellX) {
			uint8_t color = tile->colors[cellY][cellX];
			uint8_t cell = tile->cells[cellY][cellX];

			cchar_t cch;
			wchar_t wch[] = { CP437[cell], L'\0' };
			setcchar(&cch, wch, colorAttr(color), colorPair(color), NULL);
			mvadd_wch(offsetY + cellY, cellX, &cch);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 4) return EX_USAGE;

	FILE *fileA = fopen(argv[1], "r");
	if (!fileA) err(EX_NOINPUT, "%s", argv[1]);

	FILE *fileB = fopen(argv[2], "r");
	if (!fileB) err(EX_NOINPUT, "%s", argv[2]);

	FILE *fileC = fopen(argv[3], "w");
	if (!fileC) err(EX_CANTCREAT, "%s", argv[3]);

	curse();

	struct Tile tileA, tileB;
	for (;;) {
		size_t countA = fread(&tileA, sizeof(tileA), 1, fileA);
		if (ferror(fileA)) err(EX_IOERR, "%s", argv[1]);

		size_t countB = fread(&tileB, sizeof(tileB), 1, fileB);
		if (ferror(fileB)) err(EX_IOERR, "%s", argv[2]);

		if (!countA && !countB) break;
		if (!countA || !countB) errx(EX_DATAERR, "different size inputs");

		struct Meta metaA = tileMeta(&tileA);
		struct Meta metaB = tileMeta(&tileB);

		const struct Tile *tileC = (metaA.accessTime > metaB.accessTime)
			? &tileA
			: &tileB;

		if (metaA.modifyTime != metaB.modifyTime) {
			drawTile(0, &tileA);
			drawTile(CellRows + 1, &tileB);
			move(CellRows * 2 + 2, 0);
			refresh();

			int c;
			do { c = getch(); } while (c != 'a' && c != 'b');
			tileC = (c == 'a') ? &tileA : &tileB;
		}

		fwrite(tileC, sizeof(*tileC), 1, fileC);
		if (ferror(fileC)) err(EX_IOERR, "%s", argv[3]);
	}

	endwin();
	return EX_OK;
}
