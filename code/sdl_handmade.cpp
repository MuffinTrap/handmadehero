// ** GLOBAL **

#include <stdint.h>
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
// static functions are local to the file
// so we call them internal

#define internal static 
#define global_variable static
#define local_persist static

static const real32 PI32 = 3.14159265359f;

#include <string.h> // memcpy and NULL
#include <cstdio> // printf
#include <assert.h>
#include <math.h> // sin(), make your own laters.

// ** Cross platform API **

// Cross platform code include
// needs the definitions above
#include "handmade.cpp"
// And code below needs the definitions in handmade.h

// ** SDL and Linux specific includes
#include <SDL.h>	
#include <sys/mman.h>

#include "immintrin.h" // Cpu cycle counter, official place
#include "x86intrin.h" // GCC place for cycle counter

// static variables are initialized to 0
global_variable bool running;


// ** RENDERING **

// According to SDL documentation
// we should use LockTexture for frequently changing 
// textures
#define USE_TEXTURELOCK true

struct WindowBuffer
{
	SDL_Texture *texture; 
	void *bitmapMemory;
	int32 bitmapWidth;
	int32 bitmapHeight;
	int32 bytesPerPixel;
};
global_variable WindowBuffer *gWindowBuffer;

struct WindowDimensions
{
	int32 width;
	int32 height;
};

internal void handleWindowResizeEvent(SDL_Event* event);
internal void sdlResizeWindowTexture(WindowBuffer *buffer, SDL_Renderer *renderer, int32 width, int32 height);
internal void sdlUpdateWindow(WindowBuffer *buffer, SDL_Renderer *renderer);
internal game_pixel_buffer preparePixelBuffer(WindowBuffer* buffer);
internal void renderPixelBuffer(WindowBuffer* buffer);
internal WindowDimensions getWindowDimensions(uint32 windowID);
internal SDL_Renderer* getRenderer(uint32 windowID);


// ** INPUT **
// 
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

	int16 lstickX;
	int16 lstickY;
};
struct controller
{
	SDL_GameController *handle;
	controllerState state;
};
static const uint32 MAX_CONTROLLERS = 4;
global_variable controller controllers[MAX_CONTROLLERS];
global_variable controller keyboard;
static const int16 CONTROLLER_AXIS_MAX_VALUE = 32767;
static const int16 CONTROLLER_AXIS_MIN_VALUE = -32768;

internal void initControllers();
internal void closeControllers();

internal void handleInput(game_input_state& oldInput, game_input_state& outNewInput);

internal void replaceOldInput(game_input_state* oldInput, game_input_state* newInput);

internal void updateGameController(game_controller_state& oldController, game_controller_state& newController, controllerState& sdlController);

internal void updateGameButton(game_button_state& oldButton, game_button_state& newButton, bool isDown);

internal void updateGameAxis(game_axis_state& oldAxis, game_axis_state& newAxis, int16 value);
// ** AUDIO **

static const uint32 SDL_AUDIO_BUFFER_SIZE_BYTES = 2048;
static const uint32 SDL_SAMPLES_PER_CALLBACK = 1024;

SDL_AudioDeviceID audioDevice; 

struct ringBufferInfo
{
	uint32 sizeBytes;
	uint32 writeCursorBytes;
	uint32 playCursorBytes;
	void* data;
	bool updateNeeded;
};

struct dualBuffer
{
	void* region1Start;
	void* region2Start;
	uint32 region1Samples;
	uint32 region2Samples;
};

void* gameInputSoundData;

global_variable ringBufferInfo ringBuffer;

internal void initAudio(int32 samplesPerSecond);
internal void audioCallback(void *userData, uint8 *buffer, int32 length);
internal void clearRingBuffer();
internal dualBuffer prepareSoundBuffer();
internal void writeSoundBuffer(game_sound_buffer& gameInputBuffer, dualBuffer& requiredBuffer);

internal void handleEvent(SDL_Event*);
internal void handleKey(SDL_Keycode, bool wasDown);

// ** Game API

internal void updateGame(WindowBuffer* windowBuffer, game_input_state& inputState);

// ** SDL CODE

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
	int32 autodetect = -1;
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
	int32 windowWidth = 0;
	int32 windowHeight = 0;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	
	// backbuffer always same size
	windowWidth = 800;
	windowHeight = 600;

	int32 bytesPerPixel = 4;
	// Create buffer
	gWindowBuffer = new WindowBuffer();
	gWindowBuffer->bytesPerPixel = bytesPerPixel;
					
	sdlResizeWindowTexture(gWindowBuffer, renderer, windowWidth, windowHeight);


	// not an offset but speed of grardient move
	uint32 xOffset = 1;
	uint32 yOffset = 0;

	initAudio(48000);
	
	// How many times the counter updates per second 
	uint64 performanceCounterFrequency = SDL_GetPerformanceFrequency();
	uint64 lastCounter = SDL_GetPerformanceCounter();
	
	uint64 lastCycleCount = _rdtsc();

	// for updating input
	game_input_state input1 = {};
	game_input_state input2 = {};
	game_input_state* pOldInput = &input1;
	game_input_state* pNewInput = &input2;
	
	while(running)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event) != 0)
		{
			handleEvent(&event); // sets the running to false if quitting
		}

		game_input_state& oldInput = *pOldInput;
		game_input_state& newInput = *pNewInput;

		handleInput(oldInput, newInput);
		updateGame(gWindowBuffer, newInput);
		sdlUpdateWindow(gWindowBuffer, renderer);
		
		uint64 endCounter = SDL_GetPerformanceCounter();
		uint64 counterElapsed = endCounter - lastCounter;
		
		// integer division returns as zero if divided is smaller
		// uint64 secondsElapsedInFrame = counterElapsed / performanceCounterFrequency;
		uint64 msPerFrame = (1000 * counterElapsed) / performanceCounterFrequency;
		// uint64 fps = (1 / (msPerFrame / 1000)); // < Arithmetic exception> Division by zero
		// uint64 fps = (1 / counterElapsed) / performanceCounterFrequency; // < Arithmetic exception
		// 1 / counterElapsed -> 0 / 10000000 -> 0
		uint64 altFps = performanceCounterFrequency / counterElapsed;
		// This works because 1 000 000 000 / 65 000 000 > 0
		
		uint64 endCycleCount = _rdtsc();
		uint64 elapsedCycleCount = endCycleCount - lastCycleCount;
		lastCycleCount = endCycleCount;
		uint64 megaelapsedCycleCount = elapsedCycleCount / (1000 * 1000);
		
		// printf("MsF: %lu FpS: %lu mCpF: %lu\n", msPerFrame, altFps, megaelapsedCycleCount );
		lastCounter = endCounter;


		replaceOldInput(pOldInput, pNewInput);
	}
	SDL_CloseAudio();
	SDL_Quit();
	delete gWindowBuffer;
	free(ringBuffer.data);
	free(gameInputSoundData);
	return(0);
}

void initControllers()
{
	// jIndex is the SDL controller index
	uint32 maxJoysticks = SDL_NumJoysticks();
	uint32 controllerIndex = 0;
	for(uint32 jIndex = 0; jIndex < maxJoysticks; jIndex++)
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
	for( uint32 cIndex = 0; cIndex < MAX_CONTROLLERS; cIndex++)
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
	int32 flags = 0; // what things are allowed to differ from required ones
		audioDevice = SDL_OpenAudioDevice( NULL, // automatic
		0,  // for playback
		&requirements,
		&obtained,
		flags);

	if (audioDevice == 0 || obtained.format != AUDIO_S16LSB)
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

		
	audioConfig.samplesPerSecond = obtained.freq;
	// 15th of a second latency
	audioConfig.latencyBytes = (audioConfig.samplesPerSecond * audioConfig.bytesPerSample) / 15;
	audioConfig.toneHz = 256; // how many phases in second
	audioConfig.toneVolume = 3000;
	audioConfig.runningSampleIndex = 0;
	// how long is one phase to get enough Hz in second
	// -> how many samples for one phase
	audioConfig.samplesPerWavePeriod = audioConfig.samplesPerSecond / audioConfig.toneHz;
	audioConfig.halfSamplesPerWavePeriod = audioConfig.samplesPerWavePeriod / 2;
	
	audioConfig.sineWavePeriod = 2.0f * PI32;
	
	
	audioConfig.bytesPerSample = sizeof(int16) * 2; // left and right for stereo
	printf("Audiodefaults bytesPerSample is %d\n", audioConfig.bytesPerSample);
	
	// Create a ring buffer with one second of audio
	ringBuffer.sizeBytes = audioConfig.samplesPerSecond * audioConfig.bytesPerSample;
	ringBuffer.data = malloc(ringBuffer.sizeBytes);
	gameInputSoundData = malloc(ringBuffer.sizeBytes);
	if (ringBuffer.data != NULL)
	{
		printf("Reserved a ring buffer of %d bytes\n", ringBuffer.sizeBytes);
	}
	ringBuffer.playCursorBytes = 0;
	ringBuffer.writeCursorBytes = 0;
	ringBuffer.updateNeeded = false;

	clearRingBuffer();
	// start the callbacks
	SDL_PauseAudioDevice(audioDevice, 0);
}

// copies from ring buffer to SDL buffer
void audioCallback(void *userData, uint8 *buffer, int32 bytes)
{
	ringBufferInfo* ringInfo = (ringBufferInfo*)userData;
	uint32 regionSize1bytes = bytes;
	uint32 regionSize2bytes = 0;
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

void clearRingBuffer()
{

	uint8* bytePointer = (uint8*)ringBuffer.data;
	for(uint32 i = 0; i < ringBuffer.sizeBytes; i++)
	{
		*bytePointer = 0;
		bytePointer++;
	}

	ringBuffer.updateNeeded = true;
}

dualBuffer prepareSoundBuffer()
{
	// Initialize to zero
	dualBuffer soundBuffer = {};
	soundBuffer.region1Start = NULL;
	soundBuffer.region1Start = NULL;
	soundBuffer.region1Samples = 0;
	soundBuffer.region2Samples = 0;

	if (ringBuffer.updateNeeded)
	{
		ringBuffer.updateNeeded = false;
		// Lock audio device so that the callback function
		// does not change the values that we need to get
		SDL_LockAudioDevice(audioDevice);
		
		// We don't actually use the writeCursorBytes to anything, we want 
		// the part of the buffer where left with runningSampleIndex
		
		int32 wantedWriteByte = (audioConfig.runningSampleIndex * audioConfig.bytesPerSample) 
			% ringBuffer.sizeBytes;
		int32 targetCursorByte = ringBuffer.playCursorBytes + (audioConfig.latencyBytes);
		targetCursorByte = targetCursorByte % ringBuffer.sizeBytes;
		int32 bytesToWrite = 0;
		
		SDL_UnlockAudioDevice(audioDevice);

		if (wantedWriteByte > targetCursorByte)
		{
			bytesToWrite = ringBuffer.sizeBytes - wantedWriteByte;
			bytesToWrite += targetCursorByte;
		}
		else // write < play
		{
			bytesToWrite = targetCursorByte - wantedWriteByte;
		}
		
		void* region1start = (uint8*)ringBuffer.data + wantedWriteByte;
		int32 region1sizeBytes = bytesToWrite;
		
		// Check if about to write over the end of buffer
		if (wantedWriteByte + region1sizeBytes > ringBuffer.sizeBytes)
		{
			region1sizeBytes = ringBuffer.sizeBytes - wantedWriteByte;
		}
		
		void* region2start = ringBuffer.data;
		int32 region2sizeBytes = bytesToWrite - region1sizeBytes;
		
		soundBuffer.region1Start = region1start;
		soundBuffer.region1Samples = region1sizeBytes / audioConfig.bytesPerSample;
		
		if (region2sizeBytes > 0)
		{
			soundBuffer.region2Start = region2start;
			soundBuffer.region2Samples = region2sizeBytes / audioConfig.bytesPerSample;
		}
		else
		{
			soundBuffer.region2Samples = 0;
		}
	}
	return soundBuffer;
}

void writeSoundBuffer(game_sound_buffer& gameInputBuffer, dualBuffer& requiredBuffer)
{
	if (requiredBuffer.region1Start == NULL)
	{
		return;
	}
	//write from gameInputSoundData to ringbuffer as directed by parameters
	
#if 0
	// Simple copy version
	uint32 bytesToCopy1 = requiredBuffer.region1Samples * audioConfig.bytesPerSample;
	uint32 bytesToCopy2 = requiredBuffer.region2Samples * audioConfig.bytesPerSample;
	
	void* destination = memcpy(requiredBuffer.region1Start, gameInputBuffer.samples, bytesToCopy1);
	memcpy(requiredBuffer.region2Start, (uint8*)(gameInputBuffer.samples) + bytesToCopy1, bytesToCopy2);
	
	
#else
	
	int16 *inputSample = (int16*)gameInputBuffer.samples;
	int16 *ringBufferSample = (int16*)requiredBuffer.region1Start;
	
	uint32 samplesToCopy = requiredBuffer.region1Samples;
	for(uint32 s = 0; s < samplesToCopy; s++)
	{

		// STEREO SOUND: each sample consist of two data points 
		(*ringBufferSample) = (*inputSample);

		ringBufferSample++;
		inputSample++;
		
		(*ringBufferSample) = (*inputSample);

		ringBufferSample++;
		inputSample++;
		
	}
	
	uint32 samplesToCopy2 = requiredBuffer.region2Samples;
	if (samplesToCopy2 > 0)
	{
		ringBufferSample = (int16*)requiredBuffer.region2Start;	
		for(uint32 s = 0; s < samplesToCopy2; s++)
		{
			(*ringBufferSample) = (*inputSample);

			ringBufferSample++;
			inputSample++;
			
			(*ringBufferSample) = (*inputSample);

			ringBufferSample++;
			inputSample++;
		}
	}
#endif
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
					sdlUpdateWindow(gWindowBuffer, getRenderer(event->window.windowID));
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

void handleInput(game_input_state& oldInput, game_input_state& outNewInput)
{

	// Get button and axis values from SDL
	SDL_GameController *pad = NULL;
	for (uint32 cIndex = 0;
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


			// set values in newInput using the values from SDL and oldInput
			game_controller_state& oldController = oldInput.controllers[cIndex];
			game_controller_state& newController = outNewInput.controllers[cIndex];
			controllerState& sdlController = controllers[cIndex].state;

			updateGameController(oldController, newController, sdlController);
		}
	}
}

void replaceOldInput(game_input_state* oldInput, game_input_state* newInput)
{
	game_input_state* temp = oldInput;
	oldInput = newInput;
	newInput = temp;
}

void updateGameController(game_controller_state& oldController, game_controller_state& newController, controllerState& sdlController)
{
	updateGameButton(oldController.up, newController.up, sdlController.Y);
	updateGameButton(oldController.down, newController.down, sdlController.A);
	updateGameButton(oldController.left, newController.left, sdlController.X);
	updateGameButton(oldController.right, newController.right, sdlController.B);
	updateGameAxis(oldController.xAxis, newController.xAxis, sdlController.lstickX);
	updateGameAxis(oldController.yAxis, newController.yAxis, sdlController.lstickY);

	// TODO: Set isAnalog by usage of sticks or D-pad?
	newController.isAnalog = true;
}

void updateGameButton(game_button_state& oldButton, game_button_state& newButton, bool isDown)
{
	newButton.endedDown = isDown;
	newButton.halfTransitions = (oldButton.endedDown != newButton.endedDown) ? 1 : 0;
}

void updateGameAxis(game_axis_state& oldAxis, game_axis_state& newAxis, int16 value)
{
	int16 rawAxis = value;
	real32 axisValue = 0.0f;
	if (rawAxis >= 0)
	{
		axisValue = (real32)rawAxis / (real32)CONTROLLER_AXIS_MAX_VALUE;
	}
	else
	{
		axisValue = -1.0f * (real32)rawAxis / (real32)CONTROLLER_AXIS_MIN_VALUE;
	}

	// TODO: Deadzone 
	
	newAxis.start = oldAxis.end;
	newAxis.end = axisValue;
	newAxis.min = newAxis.max = newAxis.end;
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
			sdlResizeWindowTexture(gWindowBuffer, r, d.width, d.height);
	}

}

void sdlResizeWindowTexture(WindowBuffer *buffer, SDL_Renderer *renderer, int32 width, int32 height)
{
	printf("sdlResizeWindowTexture w:%d, h:%d\n", width, height);
	if (renderer == NULL)
	{
		return;
	}

	// Save texture width for calculating pitch
	uint32 format = 0;
	int32 access = 0;
	int32 result = SDL_QueryTexture(buffer->texture, &format, &access,
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
	//nop
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
	// nop
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


void updateGame(WindowBuffer* windowBuffer, game_input_state& inputState)
{
	
	dualBuffer preparedBuffer = prepareSoundBuffer();
	game_sound_buffer gameSoundBuffer = {};
	
	gameSoundBuffer.samples = gameInputSoundData;
	gameSoundBuffer.samplesToWrite = preparedBuffer.region1Samples 
	+ preparedBuffer.region2Samples;
	
	if (gameSoundBuffer.samplesToWrite > 0)
	{
		memset(gameInputSoundData, 0, ringBuffer.sizeBytes);
	}

	game_pixel_buffer gamePixelBuffer = preparePixelBuffer(windowBuffer);

	// Game modifies the given buffers
	gameUpdateAndRender(&gamePixelBuffer, &gameSoundBuffer, &inputState);

	writeSoundBuffer(gameSoundBuffer, preparedBuffer);
	renderPixelBuffer(windowBuffer);
}

game_pixel_buffer preparePixelBuffer(WindowBuffer* buffer)
{
	game_pixel_buffer gameScreenBuffer = {};
	void *texturePixels = NULL;
	int32 result = 0;
	int32 texturePitch = 0;

	#if USE_TEXTURELOCK
	result = SDL_LockTexture(buffer->texture,
		NULL, // Replace all
		&texturePixels,	
		&texturePitch);
	#else
	texturePixels = buffer->bitmapMemory;
	texturePitch = buffer->bitmapWidth * buffer->bytesPerPixel;
	#endif

	if (result == 0)
	{
		gameScreenBuffer.texturePixels = texturePixels;
		gameScreenBuffer.texturePitch = texturePitch;
		gameScreenBuffer.bitmapWidth = buffer->bitmapWidth;
		gameScreenBuffer.bitmapHeight = buffer->bitmapHeight;
	}

	return gameScreenBuffer;
}

void renderPixelBuffer(WindowBuffer *buffer)
{
	#if USE_TEXTURELOCK	
		SDL_UnlockTexture(buffer->texture);
	#else
		
		int32 result = SDL_UpdateTexture(buffer->texture,
				NULL, 							// update whole texture, not a portion
				texturePixels, 				// the memory where to get the texture
				texturePitch); 	// pitch, how many bytes is one line
		if (result != 0)
		{
			printf("error updating the texture: %s\n", SDL_GetError());
		}
	#endif
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
	int32 result;
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
