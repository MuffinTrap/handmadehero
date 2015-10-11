/* Cross platform header file */

#ifndef HANDMADE_H
#define HANDMADE_H

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

// All of the memory used by the game
struct game_memory
{
	bool32 isInitialized;
	uint64 permanentStorageSize;
	void* permanentStoragePointer; // Required to be all zeros at startup
	uint64 transientStorageSize;
	void* transientStoragePointer; // Required to be all zeros at startup
};

struct game_state
{
	int32 toneHz;
	int32 xOffset;
	int32 yOffset;
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
	void* texturePixels;
	int32 texturePitch;
	int32 bitmapWidth;
	int32 bitmapHeight;
};

struct game_audioConfig
{
	int32 samplesPerSecond;
	int32 toneHz; // how many phases in second
	int16 toneVolume;
	uint32 runningSampleIndex;
	// how long is one phase to get enough Hz in second
	// -> how many samples for one phase
	int32 samplesPerWavePeriod;
	int32 halfSamplesPerWavePeriod;
	int32 sineWavePeriod;
	int32 bytesPerSample;
	real32 tForSine;
	int32 latencyBytes; // how much ahead of the playcursor 
	// we would like to be
};

struct game_sound_buffer
{
	void* samples;
	uint32 samplesToWrite;
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
	real32 start;
	real32 min;
	real32 max;
	real32 end;
};

struct game_controller_state
{
	bool32 isAnalog;
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

	game_axis_state xAxis;
	game_axis_state yAxis;
};

struct game_input_state
{
	// TODO What do we want to pass?
	real32 secondsElapsed;
	game_controller_state controllers[4];
};

internal void 
gameUpdateAndRender(game_pixel_buffer* pixelBuffer, game_sound_buffer* soundBuffer, game_input_state* inputState, game_state* gameState);


internal void 
gameOutputSound(game_sound_buffer* buffer);

internal void 
writeSineWave(void* samples, uint32 sampleCountToWrite);

internal void 
renderWeirdGradient(game_pixel_buffer* buffer, int32 xOffset, int32 yOffset);

#endif 
