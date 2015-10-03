/* Cross platform header file */

#ifndef HANDMADE_H
#define HANDMADE_H

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

internal void 
gameUpdateAndRender(game_pixel_buffer* pixelBuffer, int32 xOffset, int32 yOffset
, game_sound_buffer* soundBuffer);

internal void 
gameUpdateAndRender_dualBuffer(game_pixel_buffer* pixelBuffer, int32 xOffset, int32 yOffset
, dualBuffer* soundBuffer);

internal void 
gameOutputSound(game_sound_buffer* buffer);
internal void 
gameOutputSound_dualBuffer(dualBuffer* buffer);

internal void writeSineWave(void* samples, uint32 sampleCountToWrite);

internal void 
renderWeirdGradient(game_pixel_buffer* buffer, int32 xOffset, int32 yOffset);

#endif 
