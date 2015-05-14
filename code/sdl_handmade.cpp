#include <SDL.h>
#include <stdint.h>
#include <sys/mman.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
// static functions are local to the file
// so we call them internal

#define internal static 
#define global_variable static
#define local_persist static

// static variables are initialized to 0
global_variable bool running;
struct WindowBuffer
{
	SDL_Texture *texture; 
	void *bitmapMemory;
	int bitmapWidth;
	int bitmapHeight;
	int bytesPerPixel;
};

struct WindowDimensions
{
	int width;
	int height;
};

global_variable WindowBuffer *buffer;

#define USE_TEXTURELOCK true


void handleEvent(SDL_Event*);
void handleInput();
internal void handleWindowResizeEvent(SDL_Event* event);
internal void sdlResizeWindowTexture(WindowBuffer *buffer, SDL_Renderer *renderer, int width, int height);
internal void sdlUpdateWindow(WindowBuffer *buffer, SDL_Renderer *renderer);
internal void renderWeirdGradient(WindowBuffer *buffer, int xOffset, int yOffset);
WindowDimensions getWindowDimensions(uint32 windowID);
SDL_Renderer* getRenderer(uint32 windowID);

int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("SDL init failed\n");
	}

	SDL_Window *window = NULL;
	window = SDL_CreateWindow("Handmade Hero",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640,
		480,
		SDL_WINDOW_RESIZABLE);

	if (window == NULL)
	{
		printf("null window\n"); 
		return 1;
	}

	SDL_Renderer *renderer = NULL;
	int autodetect = -1;
	Uint32 flags = 0;
	renderer = SDL_CreateRenderer(window, autodetect, flags);

	if (renderer == NULL)
	{
		printf("null renderer\n");
		return 1;
	}

	running = true;
	// Update window to create the texture
			int windowWidth = 0;
			int windowHeight = 0;
			SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	
	// backbuffer always same size
	windowWidth = 800;
	windowHeight = 600;

  int bytesPerPixel = 4;
	// Create buffer
	buffer = new WindowBuffer();
	buffer->bytesPerPixel = bytesPerPixel;
					
	sdlResizeWindowTexture(buffer, renderer, windowWidth, windowHeight);


	uint32 xOffset = 0;
	uint32 yOffset = 0;
	while(running)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event) != 0)
		{
			handleEvent(&event); // sets the running to false if quitting
		}

		handleInput();
		renderWeirdGradient(buffer, xOffset, yOffset);
		sdlUpdateWindow(buffer, renderer);

		xOffset++;

	}
	SDL_Quit();
	delete buffer;
	return(0);
}

void handleEvent(SDL_Event *event)
{
	switch(event->type)
	{
		case SDL_QUIT:
		{
			printf("SDL_QUIT\n");
			running = false;
		} break;

		case SDL_WINDOWEVENT:
		{
			switch(event->window.event)
			{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				{
					printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d,%d)\n",
						event->window.data1, event->window.data2);
					//handleWindowResizeEvent(event);
				} break;

				case SDL_WINDOWEVENT_RESIZED:
				{
					printf("SDL_WINDOWEVENT_SIZE_RESIZED (%d,%d)\n",
						event->window.data1, event->window.data2);
					//handleWindowResizeEvent(event);
				} break;
				case SDL_WINDOWEVENT_EXPOSED:
				{
					sdlUpdateWindow(buffer, getRenderer(event->window.windowID));
				} break;
			}
		}
	}
}

void handleInput()
{

}

void handleWindowResizeEvent(SDL_Event* event)
{
	WindowDimensions d = getWindowDimensions(event->window.windowID);
	SDL_Renderer* r = getRenderer(event->window.windowID);
	if (r)
	{
			sdlResizeWindowTexture(buffer, r, d.width, d.height);
	}

}
void sdlResizeWindowTexture(WindowBuffer *buffer, SDL_Renderer *renderer, int width, int height)
{
	printf("sdlResizeWindowTexture w:%d, h:%d\n", width, height);
	if (renderer == NULL)
	{
		return;
	}

	// Save texture width for calculating pitch
	uint32 format = 0;
	int access = 0;
	int result = SDL_QueryTexture(buffer->texture, &format, &access,
		&buffer->bitmapWidth, 
		&buffer->bitmapHeight);

	// Check if necessary
	if (width == buffer->bitmapWidth && height == buffer->bitmapHeight)
	{
		return;
	}
	if (buffer->texture)
	{
		SDL_DestroyTexture(buffer->texture);
		buffer->texture = NULL;
	}

  #if USE_LOCKTEXTURE

	#else
	if (buffer->bitmapMemory)
	{
		// if we used malloc, here would be free()
		munmap(buffer->bitmapMemory, 
			buffer->bitmapWidth * buffer->bitmapHeight * buffer->bytesPerPixel);
		buffer->bitmapMemory = NULL;
	}
	#endif

	buffer->texture = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		width, height);

	if (buffer->texture == NULL)
	{
		printf("Error when creating the texture: %s\n", SDL_GetError());
		return;
	}

	// Save texture width for calculating pitch
	result = SDL_QueryTexture(buffer->texture, &format, &access,
		&buffer->bitmapWidth, 
		&buffer->bitmapHeight);

	if (result != 0)
	{
		printf("Error querying texture: %s\n", SDL_GetError());
		return;
	}

	// malloc reserves in bytes, one byte = 8 bits
	// 4 * 8 = 32 bits
	// bitmapMemory = malloc( width * height * bytesPerPixel); 

	#if USE_LOCKTEXTURE

	#else
	// in linux we have mmap() which is similar to windows's 
	// VirtualAlloc
	buffer->bitmapMemory = mmap( 0,  // No specific start place for memory
		width * height * buffer->bytesPerPixel, // how many bytes
		PROT_READ | PROT_WRITE, // Protection flags to read/write
		MAP_ANONYMOUS | MAP_PRIVATE, // not a file, only for us
		-1, 						// no file
		0); 				// offset into the file
	#endif

}

void renderWeirdGradient(WindowBuffer *buffer, int xOffset, int yOffset)
{

	if (buffer->texture == NULL)
	{
		return;
	}
	// Modify the pixels

	void *texturePixels = NULL;
	int result = 0;
	int texturePitch = 0;
	
#if USE_TEXTURELOCK
	result = SDL_LockTexture(buffer->texture,
		NULL, // Replace all
		&texturePixels,	
		&texturePitch);
#else
	texturePixels = buffer->bitmapMemory;
	texturePitch = buffer->bitmapWidth * buffer->bytesPerPixel;
#endif
	
	if (result == 0 )
	{
		int width = buffer->bitmapWidth;
		int height = buffer->bitmapHeight;

		if (texturePixels == NULL)
		{
			printf("texturePixels NULL\n");
			return;
		}

		uint8 *row = (uint8 *)texturePixels;

		for(int y = 0;
			y < height;
			++y)
		{
			uint32* pixel = (uint32*)row;
			for(int x = 0;
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
				uint8 blue = x+xOffset;
				uint8 green = y+yOffset;

				*pixel = (( green << 8) | blue);
				pixel++;
			}

			// advance a row of bytes
			// pitch might not be width * pixels, because of padding
			row += texturePitch;
		}
		#if USE_TEXTURELOCK	
			SDL_UnlockTexture(buffer->texture);
		#else
			
			int result = SDL_UpdateTexture(buffer->texture,
					NULL, 							// update whole texture, not a portion
					texturePixels, 				// the memory where to get the texture
					texturePitch); 	// pitch, how many bytes is one line
		#endif
	}
	else
	{
		printf("error locking the texture: %s\n", SDL_GetError());
	}
}

void sdlUpdateWindow(WindowBuffer *buffer, SDL_Renderer *renderer)
{
	// According to SDL documentation
	// we should use LockTexture for frequently changing 
	// textures
	if (buffer == NULL || buffer->texture == NULL)
	{
		printf("Tried to udpate with null or empty buffer\n");
		return;
	}

	// Copy from texture to renderer, use whole area of both
	int result;
	result = SDL_RenderCopy(renderer,
		buffer->texture,
		NULL,
		NULL);

	if (result != 0)
	{
		printf("Failed to RenderCopy: %s\n", SDL_GetError());
	}

	SDL_RenderPresent(renderer);
}

WindowDimensions getWindowDimensions(uint32 windowID)
{
	WindowDimensions dim;
	SDL_Window *window = SDL_GetWindowFromID(windowID); 
	if (window)
	{
		SDL_GetWindowSize(window, &dim.width, &dim.height);
	}

	return dim;
}

SDL_Renderer* getRenderer(uint32 windowID)
{
	
	SDL_Window *window = SDL_GetWindowFromID(windowID); 
	if (window)
	{
		SDL_Renderer *renderer = SDL_GetRenderer(window);
		if (renderer)
		{
			return renderer;
		}
	}
	return NULL;
}
