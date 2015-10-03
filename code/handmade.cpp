/* Cross platform code */
#include "handmade.h"

global_variable game_audioConfig audioConfig;

void gameUpdateAndRender(game_pixel_buffer* pixelBuffer, int32 xOffset, int32 yOffset
, game_sound_buffer* soundBuffer)
{

	// TODO(tia): Now just asks continuous samples
	// Allow sample offsets here for more robust platform options
	gameOutputSound(soundBuffer);
	//renderWeirdGradient(pixelBuffer, xOffset, yOffset);
}

void gameUpdateAndRender_dualBuffer(game_pixel_buffer* pixelBuffer, int32 xOffset, int32 yOffset
, dualBuffer* soundBuffer)
{

	// TODO(tia): Now just asks continuous samples
	// Allow sample offsets here for more robust platform options
	gameOutputSound_dualBuffer(soundBuffer);
	renderWeirdGradient(pixelBuffer, xOffset, yOffset);
}
void gameOutputSound(game_sound_buffer* buffer)
{
	if (buffer->samplesToWrite > 0)
	{
		writeSineWave(buffer->samples, buffer->samplesToWrite);
	}
}

void gameOutputSound_dualBuffer(dualBuffer* buffer)
{
	if (buffer->region1Samples > 0)
	{
		writeSineWave(buffer->region1Start, buffer->region1Samples);

	}
	if (buffer->region2Samples > 0)
	{
		writeSineWave(buffer->region2Start, buffer->region2Samples);

	}
}
void writeSineWave(void* samples, uint32 samplesToWrite)
{
	printf("writeSineWave writing %d bytes\n", samplesToWrite * audioConfig.bytesPerSample);
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

		uint8 *row = (uint8 *)texturePixels;

		uint8 blueValue = 0;
		uint8 changePoint = 100;
		int8 change = 1;
		int32 newBlue = 0;
		local_persist int32 startValue = 0;
		startValue += xOffset;
		uint8 blueStartValue = 0;

		// 0 -> changePoint * 2
		if (startValue < changePoint)
		{
			change = 1;
			blueStartValue = startValue;
		}
		else if (startValue >= changePoint)
		{
			uint8 diff = startValue - changePoint;
			blueStartValue = changePoint - diff;
			change = -1;
		}
		if (startValue > changePoint * 2)
		{
			startValue = 0;
			blueStartValue = startValue;
			change = 1;
		}
		
		// startValue is 0 - changePoint

		for(int32 y = 0;
			y < height;
			++y)
		{
			blueValue = blueStartValue;

			uint32* pixel = (uint32*)row;
			for(int32 x = 0;
				x < width;
				++x)
			{
				/* Pixel in memory: 00 00 00 00
					sdl ARGB					AA RR GG BB
					LITTLE ENDIAN ARCHITECTURE

					so writing to second actually writes to GG
				*/
				// Write each byte separately
				// uint8 will wrap around automatically
				uint8 blue = blueValue;//+xOffset;
				uint8 green = 0; //y;//+yOffset;

				*pixel = (( green << 8) | blue);
				pixel++;

				newBlue = blueValue + change;
				if (change > 0 && newBlue > changePoint)
				{
					blueValue = changePoint;
					change = -1;
				}
				else if(change < 0 && newBlue <= 0)
				{
					blueValue = 0;
					change = 1;
				}
				else
				{
					blueValue = newBlue;
				}
			}

			// advance a row of bytes
			// pitch might not be width * pixels, because of padding
			row += texturePitch;
		}
}
