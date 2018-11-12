Handmade Hero Day 001 - Setting Up the Windows Build

https://www.youtube.com/watch?v=Ee3EtYb8d1o

1.	14:52
whenever you program on windows, you need something called msdn

https://msdn.microsoft.com/en-us/library/windows/desktop/ms633559(v=vs.85).aspx


2.	17:46
#include <windows.h>

3.	19:09
nCmdShow

4.	21:52
the whole point of a make file is that you only compile the code that changed, so you only do the minimum amount of work to rebuild the project

5.	23:35
the microsoft compiler is called "cl"

6.	pdb file
debug info file




7.	43:42
"pushd" and "popd" in base


53:10
So we wrote this WinMain(); function 

				int CALLBACK WinMain(...)
				{
					return (0);
				}

The question is who called us, and how do they do it?

the answer is, our current way of compiling, Its not actually the windows OS that calls into us,
the person who actually calls into us is the C runtime library (CRT);

The CRT is something that ships with all C compilers, as mandated by the C specification

this is stuff that is not part of windows, not part of the OS.
its just part of the standard library that everyone has to support when they ship a C compiler
becuz code is written expecting these will be there



A note on what a Runtime Library is.
https://stackoverflow.com/questions/2766233/what-is-the-c-runtime-library




9.	56:25
Show External Code
https://blogs.msdn.microsoft.com/zainnab/2010/10/24/show-external-code/

kernel32.dll
ntdll.dll

are in the operating system, if we jump in there, we are just gonna see assembly code cuz we dont have access to the source code

in the WinMainCRTStartup(); you can see shit tons of code trying to set up the CRT



10.	1:02:53
import library
import library is used for linking



11.	1:06:32
finding messageBox needsd User32.lib



12.	1:08:05
MessageBoxW(Unicode); and MessageBoxA(ANSI)
in all of windows, there's a #define
		
		#define MessageBox MessageBoxW or MessageBoxA

depending on if you are compiling UTF8 mode or ANSI mode


The Q/A is in another video