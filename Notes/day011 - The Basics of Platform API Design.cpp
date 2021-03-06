Handmade Hero Day 011 - The Basics of Platform API Design

7:55
-	Saved game locations
	if there are states u want to save, where does that go? u have to interface with windows to find out where that goes

-	Getting a handle to our own executable file

-	Asset loading path

-	Threading (Launch a thread)

-	Raw Input (support for multiple keyboards) 

-	Sleep / time Begin Period

	in order on laptops, we don't melt the processor down, by using all available cycles

-	ClipCursor() (multimonitor support)

-	Fullscreen support

-	WM_SETCURSOR (control cursor vsibility)

-	QueryCancelAutoplay

-	WM_ACTIVATEAPP (for when we are not the active application)

-	Blit speed improvements (BitBlit)

-	Hardware acceleration (OpenGL or Direct3D or BOTH??)

-	GetKeyboardLayout (for French keyboards, international WASD support)

just a partial list of stuff?


As cross platform as possible


13:40
how people in general, used to make code cross platform. or atleast the most prevalent way

is that you will us #define to define branches of code

				#if WIN32
					// windows code
					int CALLBACK WinMain()
					{


					}
				#elif LINUS	
					// linux code or stuff
					int main(int argc, char** argv)
					{


					}
				#elif MACOSX
					// mac logic
					{


					}
				#endif

then when you compile, you will inject the #define value
cl -DWIN32=1 -FC -Zi .....cpp xxx.lib xxx2.lib

this way of doing cross platform is not recommended anymore, cuz
1.	it causes the problem of not very readable and maintainable
2.	it dictates that the control must be the same across all platforms.
	Example:
		this becomes problmeatic form platforms that are different in the way they expect initalization	and platform services to work

		let us say we want to load resources asynchronously, load bitmaps while the game is running, so I do not have to stop the game or bring up a loading screen

		on one platform that maybe best accomplished on a separate thread

		on other platforms, it may be best accomplished with overlapped IOs

		or even memory map files. Each of these may require a very different structure for how the game starts up


23:30
The Newer Ways
1.	We started off with creating win32_handmade.cpp, that means we expect linux_handmade.cpp sometime later.
	we will have a single shared header file (for example) say handmade.h

	this single shared file, will include the operations that the platform layer is suppoed to be able to perform on hehalf of our cross platform code 

	essentially, it will create an API in this shared header file, and all of our cross platform code, will call into it


Example:
	assume we declare 

					handmade.h

					void* PlatformLoadFile(char* fileName);

	in our shared platform header file 

	then in both our win32_handmade.cpp and linux_handmade.cpp files, we will also define the function 

					win32_handmade.cpp

					void* PlatformLoadFile(char* fileName)
					{
						// Implements the Win32 file loading

					}



					linux_handmade.cpp

					void* PlatformLoadFile(char* fileName)
					{
						// lmplements the Linux file loading

					}


31:51
win32_handmade.cpp is the only file you will compile on windows.
This "build process" makes it to compile on a specific platform as dumb simple as possible


34:03 
the two philsophical branches
1st way, probably the more modern, "prevalent" cross-platform way, which is basically the virutalize the OS out to the game.

so you pretty much always call functions in your virtual Operating System 



2nd way
The way he prefers, 
there are only really three things the game needs to provide to the OS
1.	Give me the stuff u wanna draw
2.	Give me the sound u wanna play
3. 	Here is the users input

On the back channel 
1.	send this out to the network for me
2.	I need to read or write from a file now

this greatly simplifies the platform layer

The way we do it is:
we isolate the few places of the code where the platform player wants services from the game

and 

and the few places where the gamn needs services from the platform layer



1:03:40
threading often times in games is directly tied to resource management in a lot of places


1:08:44



