/* Cross platform code */
#include "handmade.h"

global_variable game_audioConfig audioConfig;

void gameUpdateAndRender(game_pixel_buffer* pixelBuffer, game_sound_buffer* soundBuffer, game_input_state* inputState, game_state* gameState)
{

	int32& xOffset = gameState->xOffset;
	int32& yOffset = gameState->yOffset;

	game_controller_state& input0 = inputState->controllers[0];
	if (input0.isAnalog)
	{
		// TODO(Tia) analog input movement
		xOffset += (int)4.0f*(input0.xAxis.end);
		yOffset += (int)4.0f*(input0.yAxis.end);

	}
	else
	{
		// Digital input

	}

	gameOutputSound(soundBuffer);
	renderWeirdGradient(pixelBuffer, xOffset, yOffset);
}


void gameOutputSound(game_sound_buffer* buffer)
{
	if (buffer->samplesToWrite > 0)
	{
		writeSineWave(buffer->samples, buffer->samplesToWrite);
	}
}

void writeSineWave(void* samples, uint32 samplesToWrite)
{
	uint32 sampleCount = samplesToWrite;
	
	int16* sampleOut = (int16*)samples;
	real32 sampleValue = 0.0f;
	int32 runningSampleIndex = audioConfig.runningSampleIndex;
	
	for (uint32 sampleIndex = 0;
			sampleIndex < sampleCount;
			sampleIndex++)
			{
				// t should be between [0, 2PI]
				// clamp runningSampleIndex between 0 and samplesPerWavePeriod
				// int clampedRunningSampleIndex = runningSampleIndex % audioConfig.samplesPerWavePeriod;
				// clamp t between [0.0f, 1.0f]
				
				//real32 t = (real32)runningSampleIndex / (real32)audioConfig.samplesPerWavePeriod;
				// sin works with multiples of 2PI, the number does not matter otherwise.
				// t *= 2.0f * PI32;
				// sinf() returns[ -1.0f, 1.0f]
				real32 sineValue = sinf(audioConfig.tForSine);
				
				sampleValue = (int16)(sineValue * audioConfig.toneVolume);
				*sampleOut = sampleValue;
				sampleOut++;
				*sampleOut = sampleValue;
				sampleOut++;
				
				runningSampleIndex++;
				// Advance t for sine by one sample
				audioConfig.tForSine += (audioConfig.sineWavePeriod) / (real32)(audioConfig.samplesPerWavePeriod);
			}
		
	audioConfig.runningSampleIndex = runningSampleIndex;

}

void renderWeirdGradient(game_pixel_buffer *pixelBuffer, int32 xOffset, int32 yOffset)
{

	// Modify the pixels
	void *texturePixels = pixelBuffer->texturePixels;
	int32 texturePitch = pixelBuffer->texturePitch;
	int32 width = pixelBuffer->bitmapWidth;
	int32 height = pixelBuffer->bitmapHeight;

	if (texturePixels == NULL)
	{
		printf("texturePixels NULL\n");
		return;
	}

	/* Pixel in memory: 00 00 00 00
		sdl ARGB					AA RR GG BB
		LITTLE ENDIAN ARCHITECTURE

		so writing to second actually writes to GG
	*/
	// Write each byte separately
	// uint8 will wrap around automatically
	uint8 *row = (uint8 *)texturePixels;
	
	for(int y = 9;
		y < height;
		y++)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0;
			x < width;
			x++)
		{
			uint8 blue = (x + xOffset);
			uint8 green = (y + yOffset);

			*pixel = ((green << 8) | blue);
			pixel++;
		}

		// advance a row of bytes
		// pitch might not be width * pixels, because of padding
		row += texturePitch;
	}
}
