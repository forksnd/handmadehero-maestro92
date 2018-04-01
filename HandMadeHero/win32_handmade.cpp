#include <windows.h>
#include <stdint.h>
// static bool running = false;

#define internal static 
#define local_persistent static 
#define global_variable static 

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer
{
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};

global_variable bool running;
global_variable win32_offscreen_buffer globalBackBuffer;

struct win32_window_dimension
{
	int width;
	int height;
};

win32_window_dimension win32GetWindowDimension(HWND window)
{
	win32_window_dimension result;

	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;

	return result;
}

internal void renderWeirdGradient(win32_offscreen_buffer buffer, int xOffset, int yOffset)
{
	uint8* row = (uint8 *)buffer.memory;
				
	for (int y = 0; y < buffer.height; ++y)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < buffer.width; ++x)
		{
			uint8 b = (x + xOffset);
			uint8 g = (y + yOffset);

			/*
				Memory:		BB GG RR xx
				Register:	xx RR GG BB

				Pixel (32-bits)
			*/

			*pixel++ = ((g<<8) | b);
		}
		row += buffer.pitch;
	}

	/*
	for (int y = 0; y < bitmapHeight; ++y)
	{
	uint8* pixel = (uint8*)row;
	for (int x = 0; x < bitmapWidth; ++x)
	{
	//	pixel in memory: 00, 00, 00, 00
	*pixel = (uint8)(x + xOffset);
	++pixel;

	*pixel = (uint8)(y + yOffset);
	++pixel;

	*pixel = 0;
	++pixel;

	*pixel = 0;
	++pixel;
	}
	row += pitch;
	}
	*/
	
}


internal void win32ResizeDibSection(win32_offscreen_buffer* buffer, int width, int height)
{
	if (buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	buffer->width = width;
	buffer->height = height;
	buffer->bytesPerPixel = 4;

	// When the bitHeight field is negative, this is the clue to Windows to treat this bitmap
	// as top-down, not bottom-up, meaning that the first three bytes of the image are the other for the top left pixel
	// in the bitmap, no the bottom left
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;


	int bitmapMemorySize = buffer->bytesPerPixel * buffer->width * buffer->height;
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	buffer->pitch = width * buffer->bytesPerPixel;

}

internal void win32DisplayBufferInWindow(HDC deviceContext, int windowWidth, int windowHeight, 
										win32_offscreen_buffer buffer,
										int x, int y, int width, int height)
{
	// TODO: Aspect ratio correction
	StretchDIBits(deviceContext, 
		0, 0, windowWidth, windowHeight, 
		0, 0, buffer.width, buffer.height,
		buffer.memory, 
		&buffer.info,
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

			win32DisplayBufferInWindow(deviceContext, dim.width, dim.height, globalBackBuffer, x, y, width, height);
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


int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR     cmdLine,
	int       showCode
)
{
	WNDCLASS windowClass = {};

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
											WS_OVERLAPPEDWINDOW|WS_VISIBLE,
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
			int xOffset = 0;
			int yOffset = 0;

			running = true;
			while(running)
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

				renderWeirdGradient(globalBackBuffer, xOffset, yOffset);
				
				HDC deviceContext = GetDC(window);
				
				win32_window_dimension dim = win32GetWindowDimension(window);
				
				win32DisplayBufferInWindow(deviceContext, dim.width, dim.height, globalBackBuffer, 0, 0, dim.width, dim.height);
				ReleaseDC(window, deviceContext);
				
				++xOffset;
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


