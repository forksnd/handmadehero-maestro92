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


internal void GameOutputSound(game_sound_output_buffer* soundBuffer, int toneHz)
{
	local_persistent real32 tSine;
	int16 toneVolume = 3000;
	int wavePeriod = soundBuffer->samplesPerSecond / toneHz;


	int16* sampleOut = soundBuffer->samples;

	for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; sampleIndex++)
	{
		real32 sineValue = sinf(tSine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		tSine += 2.0f * Pi32 *1.0f / (real32)wavePeriod;
	}
}



internal void gameUpdateAndRender(game_offscreen_buffer* buffer, int blueOffset, int greenOffset,
	game_sound_output_buffer* soundBuffer, int toneHz)
{
	GameOutputSound(soundBuffer, toneHz);
	renderWeirdGradient(buffer, blueOffset, greenOffset);
}

