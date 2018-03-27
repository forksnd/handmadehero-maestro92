#include <windows.h>

// static bool running = false;

#define internal static 
#define local_persistent static 
#define global_variable static 

global_variable bool running;
global_variable BITMAPINFO bitmapInfo;
global_variable void* bitmapMemory;
global_variable HBITMAP bitmapHandle;
global_variable HDC bitmapDeviceContext;

internal void win32ResizeDibSection(int width, int height)
{
	if (bitmapHandle)
	{
		DeleteObject(bitmapHandle);
	}
	
	if(!bitmapDeviceContext)
	{
		bitmapDeviceContext = CreateCompatibleDC(0);
	}

	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	/*
	bitmapInfo.bmiHeader.biSizeImage = 0;
	bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biClrUsed = 0;
	bitmapInfo.bmiHeader.biClrImportant = 0;
	*/
	
	HBITMAP bitmapHandle = CreateDIBSection(
				bitmapDeviceContext, 
				&bitmapInfo, 
				DIB_RGB_COLORS,
				&bitmapMemory, 
				0, 0);
}

internal void win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height)
{
	StretchDIBits(deviceContext, x, y, width, height,
		x, y, width, height,
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

			win32UpdateWindow(deviceContext, x, y, width, height);

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
		HWND windowHandle = CreateWindowEx(0, 
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
		if (windowHandle)
		{
			running = true;
			while(running)
			{
				MSG message;
				BOOL messageResult = GetMessage(&message, 0, 0, 0);
				if (messageResult > 0)
				{
					// translate message turns keyboard message to proper keyboard message
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
				else
				{
					break;
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


