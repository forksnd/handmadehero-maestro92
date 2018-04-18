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


internal void GameShutdown(game_state* gameState)
{

}


internal void gameUpdateAndRender(game_memory* memory, game_input* input, game_offscreen_buffer* buffer,
	game_sound_output_buffer* soundBuffer)
{
	// we have a requirement here. which is that our game state has to fit in permanentStorageSize
	Assert(sizeof(game_state) <= memory->permanentStorageSize);

	game_state* gameState = (game_state*)memory->permenantStorage;
	if (!memory->isInitialized)
	{
		gameState->toneHz = 256;
		gameState->greenOffset = 0;
		gameState->blueOffset = 0;

		// TODO: this may be more appropriate to do in the platform layer
		memory->isInitialized = true;
	}


	game_controller_input* input0 = &input->controllers[0];
	


	if (input0->down.endedDown)
	{
		gameState->greenOffset += 1;
	}



	GameOutputSound(soundBuffer, gameState->toneHz);
	renderWeirdGradient(buffer, gameState->blueOffset, gameState->greenOffset);
}
