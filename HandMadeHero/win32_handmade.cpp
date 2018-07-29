

#include "handmade.h"
#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <Xinput.h>
#include <dsound.h>

#include "win32_handmade.h"


global_variable bool GlobalRunning;
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




internal debug_read_file_result DEBUGplatformReadEntireFile(char* filename)
{
	debug_read_file_result result = {};
	
	HANDLE fileHandle = CreateFileA(filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		0, 0);
	
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize))
		{
			// TODO(casey): defiens for maximum values UInt32Max
			uint32 fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);
			result.contents = VirtualAlloc(0, fileSize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (result.contents)
			{
				DWORD bytesRead; 
				if (ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) && 
					(fileSize32 == bytesRead))
				{
					// NOTE(casey): file read successfully
					result.contentSize = fileSize32;
				}
				else
				{
					DEBUGplatformFreeFileMemory(result.contents);
					result.contents = NULL;
				}
			}
			else
			{
				// TODO(casey): Logging
			}
		}

		CloseHandle(fileHandle);
	}

	return (result);
}

internal void DEBUGplatformFreeFileMemory(void* memory)
{
	if (memory)
	{
		VirtualFree(memory, 0, MEM_RELEASE);
	}
}

internal bool32 DEBUGplatformWriteEntireFile(char* filename, uint32 memorySize, void* memory)
{
	bool32 result = false;
	
	HANDLE fileHandle = CreateFileA(filename,
		GENERIC_WRITE,
		0,
		0,
		CREATE_ALWAYS,
		0, 0);

	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD bytesWritten;
		if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
		{
			// NOTE(casey): file read successfully
			result = (bytesWritten == memorySize);
		}
		else
		{
		
		}
		
		CloseHandle(fileHandle);
	}
	else
	{
		// TODO(casey): Logging
	}

	return (result);
}

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
		GlobalRunning = false;
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
		GlobalRunning = false;
		break;
	}

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		Assert(!"Keybaord input came in through a non-displatch event");

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


internal void win32ProcessKeyboardMessage(game_button_state* NewState, bool32 IsDown)
{
	Assert(NewState->endedDown != IsDown);
	NewState->endedDown = IsDown;
	++NewState->halfTransitionCount;
}



internal void win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state* oldState, DWORD buttonBit, game_button_state* newState)
{
	newState->endedDown = ((XInputButtonState & buttonBit) == buttonBit);
	newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;;
}

internal real32 Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
	real32 Result = 0;
	if (Value < -DeadZoneThreshold)
	{
		Result = (real32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
	}
	else if (Value > DeadZoneThreshold)
	{
		Result = (real32)((Value + DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
	}
	return Result;
}

internal void
Win32ProcessPendingMessages(game_controller_input* KeyboardController)
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
			case WM_QUIT:
			{
				GlobalRunning = false;
			}
				break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32 vkCode = (uint32)message.wParam;
				bool wasDown = ((message.lParam & (1 << 30)) != 0);
				bool isDown = ((message.lParam & (1 << 31)) == 0);

				if (wasDown != isDown)
				{
					if (vkCode == 'W')
					{
						win32ProcessKeyboardMessage(&KeyboardController->MoveUp, isDown);
					}
					else if (vkCode == 'A')
					{
						win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, isDown);
					}
					else if (vkCode == 'S')
					{
						win32ProcessKeyboardMessage(&KeyboardController->MoveDown, isDown);
					}
					else if (vkCode == 'D')
					{
						win32ProcessKeyboardMessage(&KeyboardController->MoveRight, isDown);
					}
					else if (vkCode == 'Q')
					{
						win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, isDown);
					}
					else if (vkCode == 'E')
					{
						win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, isDown);
					}
					else if (vkCode == VK_UP)
					{
						win32ProcessKeyboardMessage(&KeyboardController->ActionUp, isDown);
					}
					else if (vkCode == VK_LEFT)
					{
						win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, isDown);
					}
					else if (vkCode == VK_DOWN)
					{
						win32ProcessKeyboardMessage(&KeyboardController->ActionDown, isDown);
					}
					else if (vkCode == VK_RIGHT)
					{
						win32ProcessKeyboardMessage(&KeyboardController->ActionRight, isDown);
					}
					else if (vkCode == VK_ESCAPE)
					{
						win32ProcessKeyboardMessage(&KeyboardController->Start, isDown);
					}
					else if (vkCode == VK_SPACE)
					{
						win32ProcessKeyboardMessage(&KeyboardController->Back, isDown);
					}
				}

				bool32 altKeyWasDown = (message.lParam & (1 << 29));
				if ((vkCode == VK_F4) && altKeyWasDown)
				{
					GlobalRunning = false;
				}
			}
			break;

			default:
			{
				// translate message turns keyboard message to proper keyboard message
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
		}
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

			GlobalRunning = true;

			int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			// we will set our base addresses
#if HANDMADE_INTERNAL
			LPVOID baseAddress = (LPVOID)Terabytes((uint64)2);
#else
			LPVOID baseAddress = 0;
#endif

			game_memory gameMemory = {};
			gameMemory.permanentStorageSize = Megabytes(64);
			gameMemory.transientStorageSize = Gigabytes((uint64)4);

			uint64 totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
			gameMemory.permenantStorage = VirtualAlloc(baseAddress, totalSize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			// for this to work, make sure you compile in 64 bit mode
			gameMemory.transientStorage = ((uint8*)gameMemory.permenantStorage + gameMemory.permanentStorageSize);


			if (samples && gameMemory.permenantStorage && gameMemory.transientStorage)
			{

				game_input input[2] = {};
				game_input* newInput = &input[0];
				game_input* oldInput = &input[1];

				LARGE_INTEGER lastCounter;
				QueryPerformanceCounter(&lastCounter);

				uint64 lastCycleCount = __rdtsc();

				while (GlobalRunning)
				{
					game_controller_input* oldKeyboardController = GetController(oldInput, 0);
					game_controller_input* newKeyboardController = GetController(newInput, 0);
					game_controller_input ZeroController = {};
					*newKeyboardController = ZeroController;
					newKeyboardController->IsConnected = true;

					for (int ButtonIndex = 0; ButtonIndex < ArrayCount(newKeyboardController->buttons); ++ButtonIndex)
					{
						newKeyboardController->buttons[ButtonIndex].endedDown = oldKeyboardController->buttons[ButtonIndex].endedDown;
					}

					Win32ProcessPendingMessages(newKeyboardController);

					// should we poll this more frequenlty
					DWORD maxControllerCount = XUSER_MAX_COUNT;
					if (maxControllerCount > (ArrayCount(newInput->controllers) - 1))
					{
						maxControllerCount = (ArrayCount(newInput->controllers)-1);
					}

					for (DWORD controllerIndex = 0; controllerIndex < maxControllerCount; ++controllerIndex)
					{
						DWORD OurControllerIndex = controllerIndex + 1;
						game_controller_input* oldController = GetController(oldInput, OurControllerIndex);
						game_controller_input* newController = GetController(newInput, OurControllerIndex);

						XINPUT_STATE controllerState;
						if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
						{
							newController->IsConnected = true;

							// this controller is plugged in
							XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

							newController->StickAverageX = Win32ProcessXInputStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							newController->StickAverageY = Win32ProcessXInputStickValue(pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

							if ((newController->StickAverageX != 0.0f) ||
								(newController->StickAverageY != 0.0f))
							{
								newController->IsAnalog = true;
							}


							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
							{
								newController->StickAverageY = 1.0f;
								newController->IsAnalog = false;
							}

							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
							{
								newController->StickAverageY = -1.0f;
								newController->IsAnalog = false;
							}
							
							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
							{
								newController->StickAverageX = -1.0f;
								newController->IsAnalog = false;
							}
							
							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
							{
								newController->StickAverageX = 1.0f;
								newController->IsAnalog = false;
							}



							real32 Threshold = 0.5f;							
							win32ProcessXInputDigitalButton(
								(newController->StickAverageX < -Threshold) ? 1 : 0, 
								&oldController->MoveLeft, 1,
								&newController->MoveLeft);

							win32ProcessXInputDigitalButton(
								(newController->StickAverageX > Threshold) ? 1 : 0,
								&oldController->MoveRight, 1,
								&newController->MoveRight);

							win32ProcessXInputDigitalButton(
								(newController->StickAverageY < -Threshold) ? 1 : 0,
								&oldController->MoveDown, 1,
								&newController->MoveDown);

							win32ProcessXInputDigitalButton(
								(newController->StickAverageY > Threshold) ? 1 : 0,
								&oldController->MoveUp, 1,
								&newController->MoveUp);

							win32ProcessXInputDigitalButton(pad->wButtons, &oldController->ActionDown, XINPUT_GAMEPAD_A,
								&newController->ActionDown);

							win32ProcessXInputDigitalButton(pad->wButtons, &oldController->ActionRight, XINPUT_GAMEPAD_B,
								&newController->ActionRight);

							win32ProcessXInputDigitalButton(pad->wButtons, &oldController->ActionLeft, XINPUT_GAMEPAD_X,
								&newController->ActionLeft);

							win32ProcessXInputDigitalButton(pad->wButtons, &oldController->ActionUp, XINPUT_GAMEPAD_Y,
								&newController->ActionUp);

							win32ProcessXInputDigitalButton(pad->wButtons, &oldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
								&newController->LeftShoulder);

							win32ProcessXInputDigitalButton(pad->wButtons, &oldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
								&newController->RightShoulder);

							win32ProcessXInputDigitalButton(pad->wButtons, &oldController->Start, XINPUT_GAMEPAD_START,
								&newController->Start);
			
							win32ProcessXInputDigitalButton(pad->wButtons, &oldController->Back, XINPUT_GAMEPAD_BACK,
								&newController->Back);

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
							newController->IsConnected = false;

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

					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
					SoundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
					SoundBuffer.samples = samples;



					game_offscreen_buffer buffer = {};
					buffer.memory = globalBackBuffer.memory;
					buffer.width = globalBackBuffer.width;
					buffer.height = globalBackBuffer.height;
					buffer.pitch = globalBackBuffer.pitch;
					gameUpdateAndRender(&gameMemory, newInput, &buffer, &SoundBuffer);

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


