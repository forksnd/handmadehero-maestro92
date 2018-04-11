#include "handmade.h"


internal void renderWeirdGradient(game_offscreen_buffer* buffer, int xOffset, int yOffset)
{
	uint8* row = (uint8 *)buffer->memory;

	for (int y = 0; y < buffer->height; ++y)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < buffer->width; ++x)
		{
			uint8 b = (x + xOffset);
			uint8 g = (y + yOffset);

			/*
			Memory:		BB GG RR xx
			Register:	xx RR GG BB

			Pixel (32-bits)
			*/

			*pixel++ = ((g << 8) | b);
		}
		row += buffer->pitch;
	}

	/*
	for (int y = 0; y < bitmapHeight; ++y)
	{
		uint8* pixel = (uint8*)row;
		for (int x = 0; x < bitmapWidth; ++x)
		{
			//	pixel in memory: 00, 00, 00, 00
			*pixel = (uint8)(x + xOffset);
			++pixel;

			*pixel = (uint8)(y + yOffset);
			++pixel;

			*pixel = 0;
			++pixel;

			*pixel = 0;
			++pixel;
		}
		row += pitch;
	}
	*/

}


internal void gameUpdateAndRender(game_offscreen_buffer* buffer, int blueOffset, int greenOffset)
{
	renderWeirdGradient(buffer, blueOffset, greenOffset);
}