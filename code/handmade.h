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

struct game_offscreen_buffer
{
	void* texturePixels;
	int32 texturePitch;
	int32 bitmapWidth;
	int32 bitmapHeight;
};

internal void 
gameUpdateAndRender(game_offscreen_buffer* buffer, int32 xOffset, int32 yOffset);

internal void 
renderWeirdGradient(game_offscreen_buffer* buffer, int32 xOffset, int32 yOffset);

#endif 
