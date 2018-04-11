


#include "handmade.h"
#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>


struct win32_offscreen_buffer
{
	// NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
};


struct win32_window_dimension
{
	int width;
	int height;
};

global_variable bool running;
global_variable win32_offscreen_buffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondBuffer;


#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_


#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void win32LoadXInput(void)
{
	// TODO(casey): Test this on windows 8
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}

	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState)
		{
			XInputGetState = XInputGetStateStub;
		}

		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState)
		{
			XInputSetState = XInputSetStateStub;
		}

	}

}


internal void win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
	// Load the library
	HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");

	if (dSoundLibrary)
	{
		// Get a DirectSound object!
		direct_sound_create* directSoundCreate = (direct_sound_create*)GetProcAddress(dSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND directSound;
		if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
		{
			WAVEFORMATEX waveFormat;
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = samplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				// "Create" a primary buffer. This primary buffer is mainly for legacy code
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
				{
					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
					{

					}
					else
					{

					}
				}
			}
			else
			{

			}

			// "create" a secondary buffer, where you will actually write to
			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;

			HRESULT error = directSound->CreateSoundBuffer(&bufferDescription, &globalSecondBuffer, 0);
			if (SUCCEEDED(error))
			{
				// Start it playing
				OutputDebugStringA("Secondary buffer created successfully.\n");
			}

		}
		else
		{

		}

	}
}

// we don't want to link with Xinput.lib, so we are gonna loading windows functions ourselves
// typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE* pState);
// typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);




internal win32_window_dimension win32GetWindowDimension(HWND window)
{
	win32_window_dimension result;

	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;

	return result;
}

internal void win32ResizeDibSection(win32_offscreen_buffer* buffer, int width, int height)
{
	if (buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	buffer->width = width;
	buffer->height = height;
	
	int bytesPerPixel = 4;

	// When the bitHeight field is negative, this is the clue to Windows to treat this bitmap
	// as top-down, not bottom-up, meaning that the first three bytes of the image are the other for the top left pixel
	// in the bitmap, no the bottom left
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;


	int bitmapMemorySize = bytesPerPixel * buffer->width * buffer->height;

	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	buffer->pitch = width * bytesPerPixel;

}

internal void win32DisplayBufferInWindow(win32_offscreen_buffer* buffer, HDC deviceContext, int windowWidth, int windowHeight)
{
	// TODO: Aspect ratio correction
	StretchDIBits(deviceContext,
		0, 0, windowWidth, windowHeight,
		0, 0, buffer->width, buffer->height,
		buffer->memory,
		&buffer->info,
		DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK win32MainWindowCallback(
	HWND   window,
	UINT   message,
	WPARAM wparam,
	LPARAM lparam
)
{
	LRESULT result = 0;

	switch (message)
	{
	case WM_SIZE:
	{
		break;
	}


	case WM_CLOSE:
	{
		// handle this with a message to the user
		running = false;
		break;
	}

	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
		break;
	}
	case WM_DESTROY:
	{
		// handle this as an error - recreate window?
		running = false;
		break;
	}

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32 vkCode = wparam;
		bool wasDown = ((lparam & (1 << 30)) != 0);
		bool isDown = ((lparam & (1 << 31)) == 0);

		if (wasDown != isDown)
		{
			if (vkCode = 'W')
			{
			}
			else if (vkCode = 'A')
			{
			}
			else if (vkCode = 'S')
			{
			}
			else if (vkCode = 'D')
			{
			}
			else if (vkCode = 'Q')
			{
			}
			else if (vkCode = 'E')
			{
			}
			else if (vkCode = VK_UP)
			{
			}
			else if (vkCode = VK_LEFT)
			{
			}
			else if (vkCode = VK_DOWN)
			{
			}
			else if (vkCode = VK_RIGHT)
			{
			}
			else if (vkCode = VK_ESCAPE)
			{
				if (isDown)
				{

				}

				if (wasDown)
				{

				}
			}
			else if (vkCode = VK_SPACE)
			{
			}
		}

		bool32 altKeyWasDown = (lparam & (1 << 29));
		if ((vkCode == VK_F4) && altKeyWasDown)
		{
			running = false;
		}


		break;
	}


	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);

		int x = paint.rcPaint.left;
		int y = paint.rcPaint.top;

		win32_window_dimension dim = win32GetWindowDimension(window);

		int width = paint.rcPaint.right - paint.rcPaint.left;
		int height = paint.rcPaint.bottom - paint.rcPaint.top;
		RECT clientRect;
		GetClientRect(window, &clientRect);

		win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dim.width, dim.height);
		EndPaint(window, &paint);

		break;
	}

	default:
	{
		result = DefWindowProc(window, message, wparam, lparam);
		break;
	}
	}
	return result;
}


struct win32_sound_output
{
	int samplesPerSecond;
	int toneHz;
	int toneVolume;
	uint32 runningSampleIndex;
	int wavePeriod;
	int bytesPerSample;
	int secondaryBufferSize;

};


internal void win32FillSoundBuffer(win32_sound_output* soundOutput, DWORD byteToLock, DWORD bytesToWrite)
{
	// int16 int16   int16 int16
	// [left  right] [left  right] 
	VOID* region1;
	DWORD region1Size;
	VOID* region2;
	DWORD region2Size;

	if (SUCCEEDED(globalSecondBuffer->Lock(byteToLock, bytesToWrite,
		&region1, &region1Size,
		&region2, &region2Size, 0)))
	{

		// TODO(Casey): assert that Region1Size/Region2Size is valid
		int16* sampleOut = (int16*)region1;
		DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
		{
			// Draw this out for people
			real32 t = 2.0f * Pi32 * (real32)soundOutput->runningSampleIndex / (real32)soundOutput->wavePeriod;
			real32 sineValue = sinf(t);
			int16 sampleValue = (int16)(sineValue * soundOutput->toneVolume);
			*sampleOut++ = sampleValue;
			*sampleOut++ = sampleValue;
			++soundOutput->runningSampleIndex;
		}

		DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
		sampleOut = (int16*)region2;
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
		{
			real32 t = 2.0f * Pi32 * (real32)soundOutput->runningSampleIndex / (real32)soundOutput->wavePeriod;
			real32 sineValue = sinf(t);
			int16 sampleValue = (int16)(sineValue * soundOutput->toneVolume);
			*sampleOut++ = sampleValue;
			*sampleOut++ = sampleValue;
			++soundOutput->runningSampleIndex;
		}

		globalSecondBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}


struct platform_window
{
	HWND handle;
};


int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR     cmdLine,
	int       showCode
)
{
	LARGE_INTEGER perfCounterFrequencyResult;
	QueryPerformanceFrequency(&perfCounterFrequencyResult);
	int64 perfCountFrequency = perfCounterFrequencyResult.QuadPart;

	win32LoadXInput();

	WNDCLASSA windowClass = {};

	win32ResizeDibSection(&globalBackBuffer, 1288, 728);

	windowClass.style = CS_HREDRAW | CS_VREDRAW;

	// function callback for events happening to this window
	// the lpfnWndProc is how we will handle all the messages that's coming from windows
	windowClass.lpfnWndProc = win32MainWindowCallback;
	windowClass.hInstance = instance;
	//	lpszMenuName;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";


	if (RegisterClass(&windowClass))
	{
		HWND window = CreateWindowEx(0,
			windowClass.lpszClassName,
			"Martin Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			instance,
			0);
		if (window)
		{
			HDC deviceContext = GetDC(window);

			// graphics test
			int xOffset = 0;
			int yOffset = 0;

			win32_sound_output soundOutput = {};

			soundOutput.samplesPerSecond = 48000;
			soundOutput.toneHz = 256;
			soundOutput.toneVolume = 3000;
			soundOutput.runningSampleIndex = 0;
			soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
			soundOutput.bytesPerSample = sizeof(int16) * 2;
			soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
			win32InitDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
			win32FillSoundBuffer(&soundOutput, 0, soundOutput.secondaryBufferSize);
			globalSecondBuffer->Play(0, 0, DSBPLAY_LOOPING);


			LARGE_INTEGER lastCounter;
			QueryPerformanceCounter(&lastCounter);

			uint64 lastCycleCount = __rdtsc();


			running = true;
			while (running)
			{

				MSG message;
				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
					{
						running = false;
					}

					// translate message turns keyboard message to proper keyboard message
					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				// should we poll this more frequenlty
				for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; ++controllerIndex)
				{
					XINPUT_STATE controllerState;
					if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
					{
						// this controller is plugged in
						XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

						bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
						bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
						bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
						bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
						bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 stickX = pad->sThumbLX;
						int16 stickY = pad->sThumbLY;

						xOffset += stickX >> 12;
						yOffset += stickY >> 12;
					}
					else
					{
						// the controller is not available
					}
				}

				game_offscreen_buffer buffer = {};
				buffer.memory = globalBackBuffer.memory;
				buffer.width = globalBackBuffer.width;
				buffer.height = globalBackBuffer.height;
				buffer.pitch = globalBackBuffer.pitch;
				gameUpdateAndRender(&buffer, xOffset, yOffset);

				//			renderWeirdGradient(&globalBackBuffer, xOffset, yOffset);

				// DirectSound output test
				DWORD playCursor;
				DWORD writeCursor;
				if (SUCCEEDED(globalSecondBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
				{
					DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
					DWORD bytesToWrite;

					if (byteToLock > playCursor)
					{
						bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
						bytesToWrite += playCursor;
					}
					else
					{
						bytesToWrite = playCursor - byteToLock;
					}

					win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
				}


				win32_window_dimension dim = win32GetWindowDimension(window);
				win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dim.width, dim.height);
			
				uint64 endCycleCount = __rdtsc();

				LARGE_INTEGER endCounter;
				QueryPerformanceCounter(&endCounter);

				uint64 cyclesElapsed = endCycleCount - lastCycleCount;
				int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
				int32 msPerFrame = (int32)((1000 * counterElapsed) / perfCountFrequency);
				int32 fps = perfCountFrequency / counterElapsed;
				int32 megaCyclesPerFrame = (int32)(cyclesElapsed / (1000 * 1000));

#if 0
				char buffer[256];
				wsprintf(buffer, "%dms/f, %dmf/s, %dmc/f\n", msPerFrame, fps, megaCyclesPerFrame);
				OutputDebugStringA(buffer);
#endif

				lastCounter = endCounter;
				lastCycleCount = endCycleCount;
			}
		}
		else
		{

		}
	}
	else
	{
		// TODO (casey): Logging
	}
	return 0;
}


