// Super Grayland: Copyright 2021 Vincent Robinson under the MIT license.
// See `license.txt` for more information.
// Before delving into the code, please read `readme_source.txt` to understand the basic design.

#include "screen.h"

// Temporary sprites, will be externalized to a file later.
SCR_SpriteBuffer TempTileSprites[] = {
	{
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
	},
	{
		{0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF},
		{0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF},
		{0x00, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x00}
	},
	{
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x24, 0x24, 0xE7, 0x00, 0x00, 0xE7, 0x24, 0x24},
		{0xDB, 0xDB, 0x18, 0xFF, 0xFF, 0x18, 0xDB, 0xDB}
	},
	{
		{0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x7E, 0xBD, 0xDB, 0xE7, 0xE7, 0xDB, 0xBD, 0x7E}
	},
};

void SCR_init(struct SCR_Screen *screen)
{
	FontSetSys(F_4x6);

	if (!GrayOn())
		COM_throwErr(COM_Error_OTHER, "Grayscale could not be initialized.");

	screen->GrayBuffer = HeapAllocPtr(GRAYDBUFFER_SIZE);
	if (screen->GrayBuffer == NULL)
		COM_throwErr(COM_Error_MEMORY, "grayscale buffer");
	GrayDBufInit(screen->GrayBuffer);

	SCR_clearAll();

	screen->TileBuffer = HeapAllocPtr(SCR_TB_BUFFER_SIZE);
	if (screen->TileBuffer == NULL)
		COM_throwErr(COM_Error_MEMORY, "tile buffer");

	/* if (FOpen("sgl\\tiles", &screen->TileBankFile, FM_READ, "sgls") != FS_OK)
		COM_throwErr(COM_Error_FILE, "sgl\\tiles");
	screen->TileBank = HeapDeref(screen->TileBankFile.dataH);
	if (FOpen("sgl\\objects", &screen->ObjBankFile, FM_READ, "sgls") != FS_OK)
		COM_throwErr(COM_Error_FILE, "sgl\\objects");
	screen->ObjBank = HeapDeref(screen->ObjBankFile.dataH) */

	screen->TileBank = TempTileSprites;

	return;
}

void SCR_deInit(struct SCR_Screen *screen)
{
	GrayOff(); // This can be called even when grayscale is already off.

	if (screen->GrayBuffer != NULL)
		HeapFreePtr(screen->GrayBuffer);
	if (screen->TileBuffer != NULL)
		HeapFreePtr(screen->TileBuffer);

	// FClose(&screen->TileBankFile);
	// FClose(&screen->ObjBankFile);

	COM_zero(screen);
}

void SCR_clear(void)
{
	GrayDBufSetHiddenAMSPlane(LIGHT_PLANE);
	ClrScr();
	GrayDBufSetHiddenAMSPlane(DARK_PLANE);
	ClrScr();
}

void SCR_clearAll(void)
{
	SCR_clear();
	SCR_swap();
	SCR_clear();
}

void SCR_drawRect(SCR_Pixel x, SCR_Pixel y, SCR_Pixel w, SCR_Pixel h, enum SCR_Color color)
{
	SCR_RECT rect = {{x, y, x + w, y + h}};

	GrayDBufSetHiddenAMSPlane(DARK_PLANE);
	if (color == SCR_Color_DARK || color == SCR_Color_BLACK)
		ScrRectFill(&rect, &rect, A_NORMAL);
	else
		ScrRectFill(&rect, &rect, A_REVERSE);

	GrayDBufSetHiddenAMSPlane(LIGHT_PLANE);
	if (color == SCR_Color_LIGHT || color == SCR_Color_BLACK)
		ScrRectFill(&rect, &rect, A_NORMAL);
	else
		ScrRectFill(&rect, &rect, A_REVERSE);
}

void SCR_drawTile(struct SCR_Screen *screen, SCR_Pixel x, SCR_Pixel y,
		const struct MAP_TileDef *tile)
{
	u8 *dark  = GrayDBufGetHiddenPlane(DARK_PLANE);
	u8 *light = GrayDBufGetHiddenPlane(LIGHT_PLANE);

	SCR_SpriteBuffer *bank = screen->TileBank;

	ClipSprite8(x, y, SCR_SPRITE_SIZE, bank[tile->Back][SCR_SPRITE_MASK], dark, SPRT_AND);
	ClipSprite8(x, y, SCR_SPRITE_SIZE, bank[tile->Back][SCR_SPRITE_DARK], dark, SPRT_OR);
	ClipSprite8(x, y, SCR_SPRITE_SIZE, bank[tile->Back][SCR_SPRITE_MASK], light, SPRT_AND);
	ClipSprite8(x, y, SCR_SPRITE_SIZE, bank[tile->Back][SCR_SPRITE_LIGHT], light, SPRT_OR);

	ClipSprite8(x, y, SCR_SPRITE_SIZE, bank[tile->Front][SCR_SPRITE_MASK], dark, SPRT_AND);
	ClipSprite8(x, y, SCR_SPRITE_SIZE, bank[tile->Front][SCR_SPRITE_DARK], dark, SPRT_OR);
	ClipSprite8(x, y, SCR_SPRITE_SIZE, bank[tile->Front][SCR_SPRITE_MASK], light, SPRT_AND);
	ClipSprite8(x, y, SCR_SPRITE_SIZE, bank[tile->Front][SCR_SPRITE_LIGHT], light, SPRT_OR);
}

// TODO: This is going to need some heavy optimizing, and cleaner, more understandable code
// (maybe some variable-esque macros if we can't use real variables?).
void SCR_drawTileBuffer(struct SCR_Screen *screen, SCR_Pixel shift_left, SCR_Pixel shift_up)
{
	u8 *dark  = GrayDBufGetHiddenPlane(DARK_PLANE);
	u8 *light = GrayDBufGetHiddenPlane(LIGHT_PLANE);

	u8 *it = screen->TileBuffer + shift_up * SCR_TB_PLANE_WIDTH;
	u16 is = SCR_SCREEN_BUFFER_WIDTH * SCR_HUD_HEIGHT;
	for (; is < SCR_SCREEN_BUFFER_SIZE; it += SCR_TB_PLANE_WIDTH, is += 30) {
		for (u16 i = 0; i < SCR_TB_PLANE_WIDTH; i++) {
			// Leave as-is. GCC optimizes it better all together like this instead of in
			// separate variables.
			*(dark + is + i) = (*(it + i) << shift_left) |
					(*(it + i + 1) >> (SCR_SPRITE_SIZE - shift_left));
			*(light + is + i) = (*(it + i + SCR_TB_PLANE_SIZE) << shift_left) |
					(*(it + i + SCR_TB_PLANE_SIZE + 1) >> (SCR_SPRITE_SIZE - shift_left));
			// Delete?
			/* u16 scr_offset = is + i;
			u8 *stb_dark = it + i;
			u8 *stb_light = stb_dark + SCR_TB_PLANE_SIZE;
			u16 backshift = SCR_SPRITE_SIZE - shift_left;
			*(dark + scr_offset) = (*stb_dark << shift_left) | (*(stb_dark + 1) >> backshift);
			*(light + scr_offset) = (*stb_light << shift_left) | (*(stb_light + 1) >> backshift); */
		}
		// memcpy(dark + is, it, SCR_TB_PLANE_WIDTH);
		// memcpy(light + is, it + SCR_TB_PLANE_SIZE, SCR_TB_PLANE_WIDTH);
	}
}

#ifdef DEBUG
void SCR_dispDebug(const char *format, ...)
{
	SCR_clear();
	SCR_swap();

	printf("\n");

	va_list arglist;
	va_start(arglist, format);
	vprintf(format, arglist);
	va_end(arglist);

	DrawStr(1, 1, "Hey presto! A debugging message for you:", A_NORMAL);

	GKeyFlush();
	ngetchx();
}
#endif

void SCR_dispError(const char *error, const char *info)
{
	ClrScr();
	DrawStr(1, 1, "A problem occurred. Sorry! :(", A_NORMAL);

	u16 place = 11;
	if (error != NULL) {
		DrawStr(1, place, error, A_NORMAL);
		place += 7;
	}

	if (info != NULL) {
		DrawStr(1, place, info, A_NORMAL);
		place += 7;
	}

	DrawStr(1, place + 4, "(It's probably Gravil's fault.)", A_NORMAL);
	DrawStr(1, place + 11, "Press the any key to exit.", A_NORMAL);

	GKeyFlush();
	ngetchx();
}

void SCR_TB_shift(struct SCR_Screen *screen, enum SCR_TB_Dir dir, u16 amount)
{
	u8 *buf = screen->TileBuffer;
	switch (dir) {
	case SCR_TB_Dir_LEFT:
		for (u8 *i = buf; i < buf + SCR_TB_BUFFER_SIZE; i += SCR_TB_PLANE_WIDTH)
			memmove(i, i + amount, SCR_TB_PLANE_WIDTH - amount);
		break;
	case SCR_TB_Dir_RIGHT:
		for (u8 *i = buf; i < buf + SCR_TB_BUFFER_SIZE; i += SCR_TB_PLANE_WIDTH)
			memmove(i + amount, i, SCR_TB_PLANE_WIDTH - amount);
		break;
	case SCR_TB_Dir_UP:
		{
			u16 offset = SCR_TB_SPRITES_WIDTH * amount;
			memmove(buf, buf + offset, SCR_TB_BUFFER_SIZE - offset);
		}
		break;
	case SCR_TB_Dir_DOWN:
		{
			u16 offset = SCR_TB_SPRITES_WIDTH * amount;
			memmove(buf + offset, buf, SCR_TB_BUFFER_SIZE - offset);
		}
		break;
	default:
		break;
	}
}

void SCR_TB_drawTile(struct SCR_Screen *screen, const struct MAP_TileDef *tile, u16 offset)
{
	u8 *dark = screen->TileBuffer + offset;
	u8 *light = dark + SCR_TB_PLANE_SIZE;

	SCR_SpriteBuffer *bank = screen->TileBank;

	// SCR_Sprite back  = bank[tile->back];
	// SCR_Sprite front = bank[tile->front];

	// Air is intentionally drawn because tiles shifted out in SCR_TB_shift are not erased, so
	// air will erase them.
	// TODO: Coalesce `dark + offset`? Make local variables to sprites to draw?
	for (u16 row = 0, offset = 0; row < SCR_SPRITE_SIZE; row++, offset += SCR_TB_PLANE_WIDTH) {
		*(dark + offset) = bank[tile->Back][SCR_SPRITE_DARK][row];
		*(light + offset) = bank[tile->Back][SCR_SPRITE_LIGHT][row];

		*(dark + offset) &= bank[tile->Front][SCR_SPRITE_MASK][row];
		*(dark + offset) |= bank[tile->Front][SCR_SPRITE_DARK][row];
		*(light + offset) &= bank[tile->Front][SCR_SPRITE_MASK][row];
		*(light + offset) |= bank[tile->Front][SCR_SPRITE_LIGHT][row];
	}
}

void SCR_TB_drawTileColumn(struct SCR_Screen *screen, struct MAP_Map *map, MAP_Pos tile_x,
		MAP_Pos tile_y, u16 x)
{
	s16 end = tile_y + SCR_TB_SPRITES_HEIGHT;
	for (; tile_y < end; tile_y++, x += SCR_TB_SPRITES_WIDTH)
		SCR_TB_drawTile(screen, MAP_getTile(map, tile_x, tile_y), x);
}

void SCR_TB_drawTileRow(struct SCR_Screen *screen, struct MAP_Map *map, MAP_Pos tile_x,
		MAP_Pos tile_y, u16 y)
{
	s16 end = tile_x + SCR_TB_PLANE_WIDTH;
	for (; tile_x < end; tile_x++, y++)
		SCR_TB_drawTile(screen, MAP_getTile(map, tile_x, tile_y), y);
}

void SCR_TB_drawAllTiles(struct SCR_Screen *screen, struct MAP_Map *map, MAP_Pos tile_x,
		MAP_Pos tile_y)
{
	for (u16 y = 0; tile_y < SCR_TB_SPRITES_HEIGHT; tile_y += 1, y += SCR_TB_SPRITES_WIDTH)
		SCR_TB_drawTileRow(screen, map, tile_x, tile_y, y);
}

// TODO: Implement this
void SCR_TB_updateAnimatedTiles(struct SCR_Screen *screen, struct MAP_Map *map,
		MAP_Pos tile_x, MAP_Pos tile_y)
{
	(void)screen;
	(void)map;
	(void)tile_x;
	(void)tile_y;
}
