Handmade Hero Day 081 - Creating Ground with Overlapping Bitmaps
  
Summary:
loaded a bunch more bitmaps for the background
"grass.bmp", "tuft.bmp", "stone.bmp" ... etc 

using our random number table, wrote a function to return a random float from [0 ~ 1].

following up that one, we have our random number that was in [0 ~ 1], scaled and moved it to [-1 ~ 1]

render the ground by rendering a bunch of random grass, tuft and stone

briefly explained what "megatexture" is in the Q/A


Keyword: 
math, rendering, megatexture 






23:04
picking random points on the screen to draw the patch


25:54
we do not know the max value in our RandomNumberTable
so he does a linear search to find out :). Smart!


26:51
does a liear search to find the min as well


28:44
wants to get a random float number from 0 ~ 1

				(real32) RandomNumberTable[RandomNumberIndex++] / (real32)MaxRandomNumber,

this is also the C++ way.

then he scale it then moves it from [0 ~ 1] to [-1 ~ 1]

				2.0f * (real32) RandomNumberTable[RandomNumberIndex++] / (real32)MaxRandomNumber - 1, 






52:10
wrote a function that makes vectors out of integers
its a function that cuts down on the redundant typing by a bit

				handmade_math.h

				inline v2
				V2i(int32 X, int32 Y)
				{
				    v2 Result = {(real32)X, (real32)Y};

				    return(Result);
				}

				inline v2
				V2i(uint32 X, uint32 Y)
				{
				    v2 Result = {(real32)X, (real32)Y};

				    return(Result);
				}

				inline v2
				V2(real32 X, real32 Y)
				{
				    v2 Result;

				    Result.X = X;
				    Result.Y = Y;

				    return(Result);
				}




1:05:57
mentions Bezier and b-spline

a bezier curve is a 3rd order polynomial





1:25:03
briefly talked about megatexture 

says it is one giant image, and that giant image has all the images/textures used in the game.
so its a giant image stitched together by a bunch of smaller images.



1:33:04
megatexture wasnt a great scheme


1:35:09
for more on megatexture, go look at "sparse virtual textures" Video Sean Barrett

