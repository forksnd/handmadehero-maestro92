Handmade Hero Day 104 - Switching to Y-is-up Render Targets

Summary:

finialized teh coordiante system in the renderer to go bottom-up 
instead of top-down

lots of debugging

nothing interesting

Keyword:
renderer


13:23
decided to finalize the coordiante system / convention for the renderer
wrote a small snippet in the handmade_render_group.h


1) Everywhere outside the renderer, Y _always_ goes upward, X to the right.



2) All bitmaps including the render target are assumed to be bottom-up
  (meaning that the first row pointer points to the bottom-most row
   when viewed on screen).

since we are doing everything bottom up, we might as well do that 


3) Unless otherwise specified, all inputs to the renderer are in world
  coordinate ("meters"); NOT pixels.  Anything that is in pixel values
  will be explicitly marked as such.



4) Z is a special coordinate because it is broken up into discrete slices,
  and the renderer actually understands these slices (potentially);

 


17:54
we go back an examine the Win32ResizeDIBSection();

we changed from 
				...
				...				
			    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
			    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
			    ...
			    ...

to
				...
				...				
			    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
			    Buffer->Info.bmiHeader.biHeight = Buffer->Height;
			    ...
			    ...

like what he said, this is just windows API convention, if you pass a negative, widows treat it as top-down
we got rid of the negative sign, so no we are rendeing bottom-up

so Casey proceeds to fix the rendering code so that it works properly for bottom-up

20:12
fliped everything, so now the first row is our bottom row

20:52
Casey mentions that we are starting to write serious code, which is code that we will want to keep

Proceeds to examine every function the handmade_render_group.cpp to see if it is treating the y coordinate correctly

I think if I were to write it, most of this can be changed through a matrix transformation change?




mostly debuggin to get the game back to parity than before



1:03:06
for a matrix

		a 	b 	c
		d 	e 	f
		g 	h 	1


a 	b 	is for rotation and scaling 
d 	e   


c 	is translation
f

g 	h 	1 	is just there for the math to work out

g 	h 	1 is usually 0 	0 	1 until you start doing projection stuff



1:05:19
when you exit the program, visual studio says there are about 8 threads termianted. Why is that?

The OS creates abunch of threads without our permission.
you can go into the threads in the visual studio debugger, 
and you can see our main thread, and all the threads that windows is running.

we didnt start any of them, but they are just a consequence of using windows services, in which
it starts a bunch of threads in your process space

