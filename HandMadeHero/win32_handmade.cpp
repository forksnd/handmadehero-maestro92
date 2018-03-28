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


global_variable bool running;
global_variable BITMAPINFO bitmapInfo;
global_variable void* bitmapMemory;
global_variable int bitmapWidth;
global_variable int bitmapHeight;
int bytesPerPixel = 4;

internal void renderWeirdGradient(int xOffset, int yOffset)
{
	int pitch = bitmapWidth * bytesPerPixel;
	uint8* row = (uint8 *)bitmapMemory;
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

			
	for (int y = 0; y < bitmapHeight; ++y)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < bitmapWidth; ++x)
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
		row += pitch;
	}
	
}


internal void win32ResizeDibSection(int width, int height)
{
	if (bitmapMemory)
	{
		VirtualFree(bitmapMemory, 0, MEM_RELEASE);
	}

	bitmapWidth = width;
	bitmapHeight = height;

	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;


	int bitmapMemorySize = bytesPerPixel * width * height;
	bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);


	renderWeirdGradient(0, 0);

}

internal void win32UpdateWindow(HDC deviceContext, RECT* windowRect, int x, int y, int width, int height)
{
	int windowWidth = windowRect->right - windowRect->left;
	int windowHeight = windowRect->bottom - windowRect->top;


	StretchDIBits(deviceContext,
		0, 0, bitmapWidth, bitmapHeight,
		0, 0, windowWidth, windowHeight, 
		bitmapMemory, 
		&bitmapInfo,
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
			RECT clientRect;
			GetClientRect(window, &clientRect);

			int width = clientRect.right - clientRect.left;
			int height = clientRect.bottom - clientRect.top;

			win32ResizeDibSection(width, height);
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
			
			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;
			RECT clientRect;
			GetClientRect(window, &clientRect);

			win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);

			/*
			local_persistent DWORD operation = WHITENESS;
			PatBlt(deviceContext, x, y, width, height, operation);
			if (operation == WHITENESS)
			{
				operation = BLACKNESS;
			}
			else
			{
				operation = WHITENESS;
			}
			*/
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
	// MessageBox(0, "Martin is awesome", "Martin", MB_OK | MB_ICONINFORMATION);

	WNDCLASS windowClass = {};


	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

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

				renderWeirdGradient(xOffset, yOffset);
				
				HDC deviceContext = GetDC(window);
				RECT clientRect;
				GetClientRect(window, &clientRect);
				int windowWidth = clientRect.right - clientRect.left;
				int windowHeight = clientRect.bottom - clientRect.top;

				win32UpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth, windowHeight);
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


