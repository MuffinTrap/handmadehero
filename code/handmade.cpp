/* Cross platform code */
#include "handmade.h"

void gameUpdateAndRender(game_offscreen_buffer* buffer, int32 xOffset, int32 yOffset)
{
	renderWeirdGradient(buffer, xOffset, yOffset);
}


void renderWeirdGradient(game_offscreen_buffer *buffer, int32 xOffset, int32 yOffset)
{

	// Modify the pixels
		void *texturePixels = buffer->texturePixels;
		int32 texturePitch = buffer->texturePitch;
		int32 width = buffer->bitmapWidth;
		int32 height = buffer->bitmapHeight;

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
