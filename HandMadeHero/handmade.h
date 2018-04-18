#ifndef HANDMADE_H_
#define HANDMADE_H_

#include "globals.h"

//	if false, we want to halt the program
//	if not true, we write to zero

/*
	HANDMADE_INTERNAL:
		0	-	build for public release	(external build)
		1	-	build for developer only	(internal build)

	HANDMADE_SLOW:
		0	-	no slow code allowed!		(with asserts off)
		1	-	slow code welcome			(with asserts on)
*/
  

#if HANDMADE_SLOW
#define Assert(expression)	\
	if(!(expression))	{*(int*)0=0;}
// this is done cuz this is the fastest way to do it platform independently
#else
#define Assert(expression)
#endif

#define Kilobytes(value)	((value) * 1024)
#define Megabytes(value)	(Kilobytes(value) * 1024)
#define Gigabytes(value)	(Megabytes(value) * 1024)
#define Terabytes(value)	(Gigabytes(value) * 1024)

#define ArrayCount(Array)	(sizeof(Array) / sizeof((Array)[0]))
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

struct game_button_state
{
	int halfTransitionCount;
	bool endedDown;
};

struct game_controller_input
{
	bool32 isAnalog;

	real32 startX;
	real32 startY;

	real32 minX;
	real32 minY;

	real32 maxX;
	real32 maxY;

	real32 endX;
	real32 endY;

	union
	{
		game_button_state buttons[6];
		struct
		{
			game_button_state up;
			game_button_state down;
			game_button_state left;
			game_button_state right;
			game_button_state leftShoulder;
			game_button_state rightShoulder;
		};
	};
};

struct game_input
{
	// insert clock value here
	game_controller_input controllers[4];
};


// using a uint64 if we every want to be giant
struct game_memory
{
	bool32 isInitialized;
	uint64 permanentStorageSize;
	void* permenantStorage;	// required to be cleared to zero at startup

	uint64 transientStorageSize;
	void* transientStorage;	// required to be cleared to zero at startup
};



internal void gameUpdateAndRender(game_memory* memory, game_input* input, game_offscreen_buffer* buffer, 
									game_sound_output_buffer* soundBuffer);


struct game_state
{
	int toneHz;
	int greenOffset;
	int blueOffset;
};



#endif