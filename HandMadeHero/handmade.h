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


inline uint32
SafeTruncateUInt64(uint64 value)
{
	Assert(value <= 0xFFFFFFFF);
	uint32 result = (uint32)value;
	return (result);
}

/*
 services that the platform layer provides to the game
 */

#define HANDMADE_INTERNAL 1


#if HANDMADE_INTERNAL
/*	IMPORTANT(casey)

	These are NOT for doing anything in the shipping game 
	- they are blocking and the write doesn't protect against lost data!

*/


struct debug_read_file_result
{
	uint32 contentSize;
	void* contents;
};

debug_read_file_result DEBUGplatformReadEntireFile(char* filename);
void DEBUGplatformFreeFileMemory(void* bitmapMemory);
bool32 DEBUGplatformWriteEntireFile(char* filename, uint32 memorySize, void* memory);
#endif


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
	bool32 IsConnected;
	bool32 IsAnalog;
	real32 StickAverageX;
	real32 StickAverageY;

	union
	{
		game_button_state buttons[12];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Back;
			game_button_state Start;

			game_button_state Terminator;
		};
	};
};

struct game_input
{
	// insert clock value here
	game_controller_input controllers[5];
};

inline game_controller_input* GetController(game_input* Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input * Result = &Input->controllers[ControllerIndex];
	return Result;
}


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