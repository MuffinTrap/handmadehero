#ifndef SDL_HANDMADE_H
#define SDL_HANDMADE_H

#include <SDL.h>

struct WindowBuffer
{
	SDL_Texture *texture; 
	void *bitmapMemory;
	int32 bitmapWidth;
	int32 bitmapHeight;
	int32 bytesPerPixel;
};

struct WindowDimensions
{
	int32 width;
	int32 height;
};

struct sdl_audio_debug_marker
{
	int outputPlayCursor;
	int outputWriteCursor;
	int outputLocation;
	int outputByteCount;
	
	int flipWriteCursor;
	int flipPlayCursor;
	
	int flipCursor;

	sdl_audio_debug_marker()
	{
		outputPlayCursor = 0;
		outputWriteCursor = 0;
		outputLocation = 0;
		outputByteCount = 0;
		flipWriteCursor = 0;
		flipPlayCursor = 0;
		flipCursor = 0;
	}
};


#endif