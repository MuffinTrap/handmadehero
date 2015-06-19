#include <SDL.h>	
#include <stdint.h>
#include <sys/mman.h>
#include <string.h> // memcpy
#include <assert.h>

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

#define USE_TEXTURELOCK true
struct WindowDimensions
{
	int width;
	int height;
};

global_variable WindowBuffer *buffer;

struct controllerState
{
	bool A;
	bool B;
	bool X;
	bool Y;
	bool START;
	bool BACK;
	bool UP;
	bool DOWN;
	bool LEFT;
	bool RIGHT;

	uint16 lstickX;
	uint16 lstickY;
};
struct controller
{
	SDL_GameController *handle;
	controllerState state;
};
static const uint MAX_CONTROLLERS = 4;
global_variable controller controllers[MAX_CONTROLLERS];
global_variable controller keyboard;

static const uint SDL_AUDIO_BUFFER_SIZE_BYTES = 2048;
static const uint SDL_SAMPLES_PER_CALLBACK = 1024;
struct audioConfig
{
	int samplesPerSecond;
	int toneHz; // how many phases in second
	int16 toneVolume;
	uint32 runningSampleIndex;
	// how long is one phase to get enough Hz in second
	// -> how many samples for one phase
	int squareWavePeriod;
	int halfSquareWavePeriod;
	int bytesPerSample;
	SDL_AudioDeviceID audioDevice; 
};

struct ringBufferInfo
{
	uint sizeBytes;
	uint writeCursorBytes;
	uint playCursorBytes;
	void* data;
	bool updateNeeded;
};


global_variable audioConfig audioDefaults;
global_variable ringBufferInfo ringBuffer;
global_variable bool soundIsPlaying;

internal void initAudio(int samplesPerSecond);
internal void audioCallback(void *userData, uint8 *buffer, int length);
internal void playSquareWave();
internal void writeSquareWave(void* targetBuffer, int bytesToWrite);
internal void initControllers();
internal void closeControllers();

internal void handleEvent(SDL_Event*);
internal void handleKey(SDL_Keycode, bool wasDown);

internal void handleInput();

internal void handleWindowResizeEvent(SDL_Event* event);
internal void sdlResizeWindowTexture(WindowBuffer *buffer, SDL_Renderer *renderer, int width, int height);
internal void sdlUpdateWindow(WindowBuffer *buffer, SDL_Renderer *renderer);
internal void renderWeirdGradient(WindowBuffer *buffer, int xOffset, int yOffset);
WindowDimensions getWindowDimensions(uint32 windowID);
SDL_Renderer* getRenderer(uint32 windowID);

int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO | 
		SDL_INIT_GAMECONTROLLER | 
		SDL_INIT_AUDIO) != 0)
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

	initControllers();

	

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


	// not an offset but speed
	uint32 xOffset = 1;
	uint32 yOffset = 0;
	soundIsPlaying = false;
	initAudio(48000);
	while(running)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event) != 0)
		{
			handleEvent(&event); // sets the running to false if quitting
		}

		handleInput();
		renderWeirdGradient(buffer, xOffset, yOffset);
		playSquareWave();
		sdlUpdateWindow(buffer, renderer);
	}
	SDL_CloseAudio();
	SDL_Quit();
	delete buffer;
	return(0);
}

void initControllers()
{
	// jIndex is the SDL controller index
	int maxJoysticks = SDL_NumJoysticks();
	int controllerIndex = 0;
	for(int jIndex = 0; jIndex < maxJoysticks; jIndex++)
	{
		if (controllerIndex >= MAX_CONTROLLERS)
		{
			break;
		}
		if (SDL_IsGameController(jIndex))
		{
			controllers[controllerIndex].handle = SDL_GameControllerOpen(jIndex);
			controllerIndex++;
		}
	}
}

void closeControllers()
{
	for( int cIndex = 0; cIndex < MAX_CONTROLLERS; cIndex++)
	{
		if (controllers[cIndex].handle != NULL)
		{
			SDL_GameControllerClose(controllers[cIndex].handle);
		}
	}
}

void initAudio(int32 samplesPerSecond)
{
	SDL_AudioSpec requirements = {0};
	SDL_AudioSpec obtained;
	requirements.freq = samplesPerSecond;
	requirements.format = AUDIO_S16LSB;
	requirements.channels = 2;
	requirements.samples = SDL_SAMPLES_PER_CALLBACK;
	requirements.callback = &audioCallback;
	requirements.userdata = &ringBuffer;
	// if callback is null, we must supply the data
	// with SDL_QueueAudio(device, void *data, uint32 bytes);
	int flags = 0; // what things are allowed to differ from required ones
	audioDefaults.audioDevice = SDL_OpenAudioDevice( NULL, // automatic
		0,  // for playback
		&requirements,
		&obtained,
		flags);

	if (audioDefaults.audioDevice == 0 || obtained.format != AUDIO_S16LSB)
	{
		printf("Could not get suitable audio device.\n");
		SDL_CloseAudio();
		return;
	}
	else
	{
		printf("Got audio device: freq: %d, channels: %d, buffer size %d\n", 
					 obtained.freq, obtained.channels, obtained.samples);
	}

		
	audioDefaults.samplesPerSecond = obtained.freq;
	audioDefaults.toneHz = 256; // how many phases in second
	audioDefaults.toneVolume = 3000;
	audioDefaults.runningSampleIndex = 0;
	// how long is one phase to get enough Hz in second
	// -> how many samples for one phase
	audioDefaults.squareWavePeriod = audioDefaults.samplesPerSecond / audioDefaults.toneHz;
	audioDefaults.halfSquareWavePeriod = audioDefaults.squareWavePeriod / 2;
	audioDefaults.bytesPerSample = sizeof(int16) * 2; // left right
	printf("Audiodefaults bytesPerSample is %d\n", audioDefaults.bytesPerSample);
	
	// Create a ring buffer with one second of audio
	ringBuffer.sizeBytes = audioDefaults.samplesPerSecond * audioDefaults.bytesPerSample;
	ringBuffer.data = malloc(ringBuffer.sizeBytes);
	if (ringBuffer.data != NULL)
	{
		printf("Reserved a ring buffer of %d bytes\n", ringBuffer.sizeBytes);
	}
	ringBuffer.playCursorBytes = 0;
	ringBuffer.writeCursorBytes = 0;
	ringBuffer.updateNeeded = true;

	playSquareWave(); // Fill the ring buffer
	if (!soundIsPlaying)
	{
		// start the callbacks
		SDL_PauseAudioDevice(audioDefaults.audioDevice, 0);
		soundIsPlaying = true;
	}
}

// copies from ring buffer to SDL buffer
void audioCallback(void *userData, uint8 *buffer, int bytes)
{
	
	ringBufferInfo* ringInfo = (ringBufferInfo*)userData;
	printf("Audio call back. Playcursor at %d\n", ringInfo->playCursorBytes);
	uint regionSize1bytes = bytes;
	uint regionSize2bytes = 0;
	if (ringInfo->playCursorBytes + bytes > ringInfo->sizeBytes)
	{
			regionSize1bytes = ringInfo->sizeBytes - ringInfo->playCursorBytes;
			regionSize2bytes = bytes - regionSize1bytes;
	}
	memcpy(buffer, (uint8*)(ringInfo->data) + ringInfo->playCursorBytes, regionSize1bytes); // to, from, amount
	memcpy(&buffer[regionSize1bytes], ringInfo->data, regionSize2bytes);
	
	ringInfo->playCursorBytes = (ringInfo->playCursorBytes + bytes) % ringInfo->sizeBytes;
	ringInfo->writeCursorBytes = (ringInfo->playCursorBytes + SDL_AUDIO_BUFFER_SIZE_BYTES) % ringInfo->sizeBytes;
	ringInfo->updateNeeded = true;
}


// writes square wave to ring buffer
void playSquareWave()
{

	
	if (ringBuffer.updateNeeded)
	{
		ringBuffer.updateNeeded = false;
		// Lock audio device while we are writing to the ring buffer
		SDL_LockAudioDevice(audioDefaults.audioDevice);
		
		// We don't actually use the writeCursorBytes to anything, we want 
		// the part of the buffer where left with runningSampleIndex
		
		
		int wantedWriteByte = audioDefaults.runningSampleIndex * audioDefaults.bytesPerSample 
			% ringBuffer.sizeBytes;
		int bytesToWrite = 0;
		assert(wantedWriteByte < ringBuffer.sizeBytes);

		if (ringBuffer.playCursorBytes == wantedWriteByte)
		{
			bytesToWrite = ringBuffer.sizeBytes;
		}
		else if (wantedWriteByte > ringBuffer.playCursorBytes)
		{
			bytesToWrite = ringBuffer.sizeBytes - wantedWriteByte;
			bytesToWrite += ringBuffer.playCursorBytes;
		}
		else // write < play
		{
			bytesToWrite = ringBuffer.playCursorBytes - wantedWriteByte;
		}
		void* region1start = (uint8*)ringBuffer.data + wantedWriteByte;
		int region1sizeBytes = bytesToWrite;
		if (wantedWriteByte + region1sizeBytes > ringBuffer.sizeBytes)
		{
			region1sizeBytes = ringBuffer.sizeBytes - wantedWriteByte;
		}
		void* region2start = ringBuffer.data;
		int region2sizeBytes = bytesToWrite - region1sizeBytes;
		
		SDL_UnlockAudioDevice(audioDefaults.audioDevice);
		
		writeSquareWave(region1start, region1sizeBytes);
		if (region2sizeBytes > 0)
		{
			writeSquareWave(region2start, region2sizeBytes);
		}
		
		printf("playSquareWave writing to %d region1 %d region2 %d\n", wantedWriteByte, region1sizeBytes, region2sizeBytes);
	}
}


// writes square wave to any buffer
void writeSquareWave(void* targetBuffer, int bytesToWrite)
{
	printf("writeSquareWave writing %d bytes\n", bytesToWrite);
	int sampleCount = bytesToWrite / audioDefaults.bytesPerSample;
	// bytes /  bytes / samples ->  bytes * samples / bytes -> samples
	
	int16* sampleOut = (int16*)targetBuffer;
	int16 sampleValue = 0;
	int runningSampleIndex = audioDefaults.runningSampleIndex;
	
	for( int sampleIndex = 0;
		sampleIndex < sampleCount;
		sampleIndex++)
		{
			sampleValue = ((runningSampleIndex / audioDefaults.halfSquareWavePeriod) % 2 ) ? 
				audioDefaults.toneVolume : -audioDefaults.toneVolume;
				
			*sampleOut = sampleValue;
			*sampleOut++;

			*sampleOut = sampleValue;
			*sampleOut++;
			
			runningSampleIndex++;
		}

	audioDefaults.runningSampleIndex = runningSampleIndex;
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
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			SDL_Keycode keyCode = event->key.keysym.sym;
			bool wasDown = event->key.state;
			handleKey(keyCode, wasDown);
		} break;

		// Controllers added or changed
		// SDL_CONTROLLERDEVICEADDED, SDL_CONTROLLERDEVICEREMOVED
		// SDL_CONTROLLERDEVICEREMAPPED .which is joystic device index
	}
}

void handleInput()
{
	// Get which buttons are down
	SDL_GameController *pad = NULL;
	for (int cIndex = 0;
		cIndex < MAX_CONTROLLERS;
		cIndex++)
	{
		
		pad = controllers[cIndex].handle;
		if (pad != NULL)
		{
			if (SDL_GameControllerGetAttached(pad));
			{
				controllers[cIndex].state.A = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_A) == 1);
				controllers[cIndex].state.B = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_B) == 1);
				controllers[cIndex].state.X = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_X) == 1);
				controllers[cIndex].state.Y = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_A) == 1);

				controllers[cIndex].state.START = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_START) == 1);
				controllers[cIndex].state.BACK = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_BACK) == 1);
				controllers[cIndex].state.UP = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_DPAD_UP) == 1);
				controllers[cIndex].state.DOWN = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_DPAD_DOWN) == 1);
				controllers[cIndex].state.LEFT = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_DPAD_LEFT) == 1);
				controllers[cIndex].state.RIGHT = (SDL_GameControllerGetButton(pad, 
					SDL_CONTROLLER_BUTTON_DPAD_RIGHT) == 1);

				controllers[cIndex].state.lstickX = SDL_GameControllerGetAxis(pad,
				SDL_CONTROLLER_AXIS_LEFTX);
				controllers[cIndex].state.lstickY = SDL_GameControllerGetAxis(pad,
				SDL_CONTROLLER_AXIS_LEFTY);
			}
		}
	}

	// Get directions 

}

void handleKey(SDL_Keycode keyCode, bool wasDown)
{
	bool down = !wasDown;
	switch(keyCode)
	{
		case SDLK_UP:
		{
			keyboard.state.UP = down;
		} break;
		case SDLK_DOWN:
		{
			keyboard.state.DOWN = down;
		} break;
		case SDLK_LEFT:
		{
			keyboard.state.LEFT = down;
		} break;
		case SDLK_RIGHT:
		{
			keyboard.state.RIGHT = down;
		} break;

		case SDLK_a:
		{
			keyboard.state.A = down;
		} break;
		case SDLK_b:
		{
			keyboard.state.B = down;
		} break;
		case SDLK_x:
		{
			keyboard.state.X = down;
		} break;
		case SDLK_y:
		{
			keyboard.state.Y = down;
		} break;

		case SDLK_RETURN:
		{
			keyboard.state.START = down;
		} break;
		case SDLK_ESCAPE:
		{
			keyboard.state.BACK = down;
		} break;
	}
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

		uint8 blueValue = 0;
		uint8 changePoint = 100;
		int8 change = 1;
		int newBlue = 0;
		local_persist int startValue = 0;
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

		for(int y = 0;
			y < height;
			++y)
		{
			blueValue = blueStartValue;

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
