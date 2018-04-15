

#include "handmade.h"
#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <Xinput.h>
#include <dsound.h>

#include "win32_handmade.h"


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




internal void win32ClearSoundBuffer(win32_sound_output* soundOutput)
{
	VOID* region1;
	DWORD region1Size;
	VOID* region2;
	DWORD region2Size;

	if (SUCCEEDED(globalSecondBuffer->Lock(0, soundOutput->secondaryBufferSize,
		&region1, &region1Size,
		&region2, &region2Size, 0)))
	{
		int8* destSample = (int8*)region1;
		for (DWORD sampleIndex = 0; sampleIndex < region1Size; sampleIndex++)
		{
			*destSample++ = 0;
		}

		destSample = (int8*)region2;
		for (DWORD sampleIndex = 0; sampleIndex < region2Size; sampleIndex++)
		{
			*destSample++ = 0;
		}

		globalSecondBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

internal void win32FillSoundBuffer(win32_sound_output* soundOutput, DWORD byteToLock, DWORD bytesToWrite,
	game_sound_output_buffer* sourceBuffer)
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

		DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
		int16* destSample = (int16*)region1;
		int16* sourceSample = sourceBuffer->samples;

		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
		{
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			++soundOutput->runningSampleIndex;
		}

		DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
		destSample = (int16*)region2;
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
		{
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			++soundOutput->runningSampleIndex;
		}

		globalSecondBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}


internal void win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state* oldState, DWORD buttonBit, game_button_state* newState)
{
	newState->endedDown = ((XInputButtonState & buttonBit) == buttonBit);
	newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;;


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
			win32_sound_output soundOutput = {};
			
			soundOutput.samplesPerSecond = 48000;
			soundOutput.bytesPerSample = sizeof(int16) * 2;
			soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
			soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
			win32InitDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
			win32ClearSoundBuffer(&soundOutput);
			globalSecondBuffer->Play(0, 0, DSBPLAY_LOOPING);
			
			running = true;

			int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			game_input input[2] = {};
			game_input* newInput = &input[0];
			game_input* oldInput = &input[1];

			LARGE_INTEGER lastCounter;
			QueryPerformanceCounter(&lastCounter);

			uint64 lastCycleCount = __rdtsc();




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
				int maxControllerCount = XUSER_MAX_COUNT;
				if (maxControllerCount > ArrayCount(newInput->controllers))
				{
					maxControllerCount = ArrayCount(newInput->controllers);
				}

				for (DWORD controllerIndex = 0; controllerIndex < maxControllerCount; ++controllerIndex)
				{
					game_controller_input* oldController = &oldInput->controllers[controllerIndex];
					game_controller_input* newController = &newInput->controllers[controllerIndex];

					XINPUT_STATE controllerState;
					if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
					{
						// this controller is plugged in
						XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

						bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

						newController->isAnalog = true;
						newController->startX = oldController->endX;
						newController->startY = oldController->endY;

						// normalizing the x value
						real32 x;
						if (pad->sThumbLX < 0)
						{
							x = (real32)pad->sThumbLX / 32768.0f;
						}
						else
						{
							x = (real32)pad->sThumbLX / 32767.0f;
						}

						newController->minX = newController->maxX = newController->endX = x;


						real32 y;
						if (pad->sThumbLY < 0)
						{
							y = (real32)pad->sThumbLY / 32768.0f;
						}
						else
						{
							y = (real32)pad->sThumbLY / 32767.0f;
						}

						newController->minY = newController->maxY = newController->endY = y;

						win32ProcessXInputDigitalButton(pad->wButtons, &oldController->down, XINPUT_GAMEPAD_A,
																		&newController->down);

						win32ProcessXInputDigitalButton(pad->wButtons, &oldController->right, XINPUT_GAMEPAD_B,
																		&newController->right);

						win32ProcessXInputDigitalButton(pad->wButtons, &oldController->left, XINPUT_GAMEPAD_X,
																		&newController->left);

						win32ProcessXInputDigitalButton(pad->wButtons, &oldController->up, XINPUT_GAMEPAD_Y,
																		&newController->up);

						win32ProcessXInputDigitalButton(pad->wButtons, &oldController->leftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
																		&newController->leftShoulder);

						win32ProcessXInputDigitalButton(pad->wButtons, &oldController->rightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
																		&newController->rightShoulder);

						/*
						bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
						bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
						bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
						bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
						bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);
						*/

					}
					else
					{
						// the controller is not available
					}
				}

				
				DWORD byteToLock = 0;
				DWORD targetCursor = 0;
				DWORD bytesToWrite = 0;
				DWORD playCursor = 0;
				DWORD writeCursor = 0;
				bool32 soundIsValid = false;
				// TODO(casey): Tighten up sound logic so that we know where we should be
				// writing to and can anticipate the time spent in the game update.
				if (SUCCEEDED(globalSecondBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
				{
					byteToLock = ((soundOutput.runningSampleIndex*soundOutput.bytesPerSample) %
						soundOutput.secondaryBufferSize);

					targetCursor =
						((playCursor +
						(soundOutput.latencySampleCount*soundOutput.bytesPerSample)) %
							soundOutput.secondaryBufferSize);
					if (byteToLock > targetCursor)
					{
						bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
						bytesToWrite += targetCursor;
					}
					else
					{
						bytesToWrite = targetCursor - byteToLock;
					}

					soundIsValid = true;
				}

			//	int16 samples[48000 * 2];
				game_sound_output_buffer SoundBuffer = {};
				SoundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
				SoundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
				SoundBuffer.samples = samples;

				game_offscreen_buffer buffer = {};
				buffer.memory = globalBackBuffer.memory;
				buffer.width = globalBackBuffer.width;
				buffer.height = globalBackBuffer.height;
				buffer.pitch = globalBackBuffer.pitch;
				gameUpdateAndRender(newInput, &buffer, &SoundBuffer);

				// NOTE(casey): DirectSound output test
				if (soundIsValid)
				{
					//"sound is valid" << endl;
					win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &SoundBuffer);
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

				game_input* temp = newInput;
				newInput = oldInput;
				oldInput = temp;
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


