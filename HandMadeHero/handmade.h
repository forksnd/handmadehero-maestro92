#ifndef HANDMADE_H_
#define HANDMADE_H_
/*
 services that the platform layer provides to the game

*/

/*
	services that the game provides to the platform layer

*/

// FOUR things		timing
//					controller/keyboard input,
//					bitmap buffer to use
//					sound buffer to use

#include "globals.h"


struct game_offscreen_buffer
{
	void* memory;
	int width;
	int height;
	int pitch;
};

struct game_sound_output_buffer
{
	int samplesPerSecond;
	int sampleCount;	
	int16* samples;
};

internal void gameUpdateAndRender(game_offscreen_buffer* buffer, int blueOffset, int greenOffset,
									game_sound_output_buffer* soundBuffer, int toneHz);



#endif