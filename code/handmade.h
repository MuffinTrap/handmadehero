/* Cross platform header file */

#ifndef HANDMADE_H
#define HANDMADE_H


#include <math.h> // sin(), make your own laters.
#include <stdint.h>
#include <string.h> // memcpy and NULL
#include <cstdio> // printf

// Type defines
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

typedef int32 bool32;

static const real32 PI32 = 3.14159265359f;

// static functions are local to the file
// so we call them internal

#define internal static 
#define global_variable static
#define local_persist static

// Macros
#define bitsPerKilo 1024LL
#define SizeKiloBytes(value) ((value) * bitsPerKilo)
#define SizeMegaBytes(value) (SizeKiloBytes(value) * bitsPerKilo) 
#define SizeGigaBytes(value) (SizeMegaBytes(value) * bitsPerKilo)
#define SizeTeraBytes(value) (SizeGigaBytes(value) * bitsPerKilo)

// HANDMADE_INTERNAL: 0 for public release, 1 for internal build 
// HANDMADE_SLOW: 0 no slow code, 1 debug code and asserts
// custom assert, tries to write to memory address zero when condition
// is not true
// __LINE__ and __FILE__ are supported by C standard.
#if HANDMADE_SLOW
	#define hm_assert(expression) \
	if (!(expression)) {\
	printf("Assert on (" #expression ") at %s line %d\n", __FILE__, __LINE__);\
	{*(int *)0 = 0;} }
#else
	#define hm_assert(expression)
#endif

#define ArrayCount(array) (sizeof(array)/sizeof(array[0]))
// Services that the platform layer provides to the game

#if HANDMADE_INTERNAL
// IMPORTANT
// These are not for doing anything in the shipped game.
// They are locking and the write does not protext against lost data!
// 
struct debug_read_file_result
{
	void* memoryPointer;
	uint32 sizeBytes;

	debug_read_file_result()
	{
		memoryPointer = NULL;
		sizeBytes = 0;
	}
};

// Debug functions whose implementation differs by platform
// Pointers to implementation are given in game_memory struct

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
DEBUG_PLATFORM_FREE_FILE_MEMORY(debugPlatformFreeFileMemory);

// void debugPlatformFreeFileMemory(void* bitmapMemory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(const char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
DEBUG_PLATFORM_READ_ENTIRE_FILE(debugPlatformReadEntireFile);
// debug_read_file_result debugPlatformReadEntireFile(const char* filename);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(const char* filename, uint32 memorySize, void* memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debugPlatformWriteEntireFile);

// bool32 debugPlatformWriteEntireFile(const char* filename, uint32 memorySize, void* memory);

#endif

internal uint32 
safeTruncateUint64(uint64 value)
{
	hm_assert(value <= UINT32_MAX)
	uint32 result = (uint32)value;
	return result;
}


// All of the memory used by the game
struct game_memory
{
	bool32 isInitialized;
	uint64 permanentStorageSize;
	void* permanentStoragePointer; // Required to be all zeros at startup
	uint64 transientStorageSize;
	void* transientStoragePointer; // Required to be all zeros at startup

	game_memory()
	{
		isInitialized = false;
		permanentStorageSize = 0;
		permanentStoragePointer = NULL;
		transientStorageSize = 0;
		transientStoragePointer = NULL;
	}
	
	#if HANDMADE_INTERNAL
	debug_platform_free_file_memory *debug_free_memory;
	debug_platform_read_entire_file *debug_read_file;
	debug_platform_write_entire_file *debug_write_file;
	#endif
	
};

struct game_state
{
	int32 toneHz;
	int32 xOffset;
	int32 yOffset;
	real32 tSine;
};
/*
	Services that the game provides to the platform layer
	Timing
	controller/keyboard input
	bitmap buffer to use
	sound buffer to use
*/

struct game_pixel_buffer
{
	// NOTE: Pixels are always 32-bits wide, Memory order BB GG RR XX
	void* texturePixels;
	int32 texturePitch;
	int32 bitmapWidth;
	int32 bitmapHeight;
	int32 bytesPerPixel;

	game_pixel_buffer()
	{
		texturePixels = NULL;
		texturePitch = 0;
		bitmapWidth = 0;
		bitmapHeight = 0;
		bytesPerPixel = 0;
	}
};

struct game_audioConfig
{
	int32 samplesPerSecond;
	int32 bytesPerSample;
	uint32 runningSampleIndex;

	// For measuring latency 
	int32 currentLatencyBytes; // How much latency we have currently
	int32 currentLatencySeconds; // How much latency seconds we have currently
	
	// For sound syncing 
	int32 safetyBytes;
	int32 expectedSoundBytesPerFrame;
	int32 expectedFrameBoundaryByte;
	uint32 gameUpdateHz;
	bool32 audioCardIsLatent;
	int32 expectedBytesUntilFlip;
	uint64 audioWallClock;
	uint64 flipWallClock;
	real32 targetSecondsPerFrame;
	
	int32 latencyBytes; // how much ahead of the playcursor 
	// we would like to be
	
	// For playing debug sinewave 
		// how long is one phase to get enough Hz in second
	// -> how many samples for one phase
	int32 toneHz; // how many phases in second
	int16 toneVolume;
	real32 tForSine;
	int32 samplesPerWavePeriod;
	int32 halfSamplesPerWavePeriod;
	int32 sineWavePeriod;
};

struct game_sound_buffer
{
	void* samples;
	uint32 samplesToWrite;

	game_sound_buffer()
	{
		samples = NULL;
		samplesToWrite = 0;
	}
	
	// For sine wave output, copied from audioConfig
	uint32 runningSampleIndex;
	real32 tForSine;
	int32 samplesPerWavePeriod;
};


// INPUT
struct game_button_state
{
	int32 halfTransitions;
	bool32 endedDown;
};

// Normalized axis values
struct game_axis_state
{
	real32 average;

	game_axis_state()
	{
		average = 0.0f;
	}
};

struct game_controller_state
{
	bool32 isConnected;
	bool32 isAnalog;
	union
	{
		game_button_state buttons[12];
		struct
		{
			game_button_state moveUp;
			game_button_state moveDown;
			game_button_state moveLeft;
			game_button_state moveRight;

			game_button_state actionUp;
			game_button_state actionDown;
			game_button_state actionLeft;
			game_button_state actionRight;

			game_button_state leftShoulder;
			game_button_state rightShoulder;

			game_button_state start;
			game_button_state back;

			// 
			game_button_state terminator;
		};

	};

	game_axis_state xAxis;
	game_axis_state yAxis;

	game_controller_state()
	{
		isConnected = false;
		isAnalog = false;
	}
};

struct game_input_state
{
	// TODO What do we want to pass?
	real32 secondsElapsed;
	game_controller_state controllers[4];
	game_controller_state keyboard;

	game_input_state()
	{
		secondsElapsed = 0;
	}
};

// Used by platform 

// Function pointers for when functions are loaded from shared library or dynamic
// library
// These need to be declared to extern "C" functions so that 
// C++ name mangling does not change the names and we can load them from the 
// shared library object.

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *memory, game_pixel_buffer* pixelBuffer, game_input_state* inputState, game_state* gameState)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
extern "C"
{
	extern GAME_UPDATE_AND_RENDER(gameUpdateAndRenderStub)
	{
		// nop
	}
	extern GAME_UPDATE_AND_RENDER(gameUpdateAndRender);
}

// This needs to be fast, about a millisecond to keep sound in sync

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *memory, game_sound_buffer* buffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

extern "C"
{
	extern GAME_GET_SOUND_SAMPLES(gameGetSoundSamplesStub)
	{
		// nop
	}

	extern GAME_GET_SOUND_SAMPLES(gameGetSoundSamples);
}
//void
//gameGetSoundSamples(game_sound_buffer* buffer);

// Internal use
void 
gameOutputSound(game_sound_buffer* buffer);

void 
writeSineWave(game_sound_buffer* buffer);

void 
renderWeirdGradient(game_pixel_buffer* buffer, int32 xOffset, int32 yOffset);

void
renderBlackScreen(game_pixel_buffer* buffer);




#endif 
