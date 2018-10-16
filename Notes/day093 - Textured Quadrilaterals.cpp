Handmade Hero Day 093 - Textured Quadrilaterals

Summary:
implemented texture mapping on quads

showed the visual artificat of rendering magnified textured quads 
if we do not use subpixel rendering

implemented the bilinear filtering part of subpixel rendering 
(we are still not yet subpixel accurate. To be fully subpixel accurate, we have to do multiple things)

Keyword:
subpixel rendering, bilinear filtering



6:40
mentions that in this episode, he will just do a brute force, not so smart/elegant way.

Will revisit the fancier/modern way in later episodes










9:30
introduces the concept of (u,v) coordinates when addressing texture coordinates.

u is in the x direction, ranged [0, 1]
v is in the y direction, ranged [0, 1]



for bitmaps we have 
		x -> [0, width - 1]
		y -> [0, height - 1]



14:06
offers a graphical explanation. project pixel center to u axis and v axis, 
the color the pixel with texture(u,v)

math is 

	vec = vector(p, origin);

	uc = dot(vec, uaxis) * uaxis
	vc = dot(vec, vaxis) * vaxis

then pixel.color = texture(uc, vc);







27:27
doing the rounding.
quoting my notes from day 027

				if you just cast a float to an int, the default thing that C will do is that it will truncate the value.
				so it will just get rid of the fractional part

				for example if you are 0.75, rounding it will give you 0, but what you really want is 1. 
				so we offset it with a 0.5.

so we added 0.5 here.




30:25
added alpha blending to it



34:46
Casey rendered a quad that is texutred with the tree, and the rendered tree looks very pixelated...


36:55
the magnified tree is rendered, and you can see the aliasting at work


38:35
Casey shows how we are seeing wiggly, wavy lines motion as the maginified quad moves from left to right

what is causing this is becuz you have a situation where the number of pixels and the number of texels 
is not dividing out evenly. 






lets say you have some pixels on the screen
				 ___________________________			
	pixels		|_|_|_|_|_|_|_|_|_|_|_|_|_|_|


and we have our 
				 ___________________			
	texels		|_|_|_|_|_|_|_|_|_|_|


when we are magnifiying our texture, each texel maps to multiple pixels


				 ___________________________			
	pixels		|A|A|A|B|B|B|_|_|_|_|_|_|_|_|
 
				 ___________________			
	texels		|A|B|_|_|_|_|_|_|_|_|



butthe problem is that when the two does not divide out perfectly evently (which the two almost never will devide out evently);


41:11 
Casey mentions that this visual artifact is soooooooooooo common in indie games.
and it bothers him in some deep level


The solution is to use sub-pixel rendering!!!!!!!!




42:01
starting to discuss proper solution for this 

initially, we have something like this 


				internal void
				DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
				                    loaded_bitmap *Texture)
				{
				    real32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
				    real32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);
				    
				    ...
				    ...

				    for(int Y = YMin; Y <= YMax; ++Y)
				    {
				        uint32 *Pixel = (uint32 *)Row;
				        for(int X = XMin; X <= XMax; ++X)
				        {

				            v2 PixelP = V2i(X, Y);
				            v2 d = PixelP - Origin;
				      
				      		if(.....4 edge tests....)
				      		{
				      			real32 U = InvXAxisLengthSq*Inner(d, YAxis);
				      			real32 V = InvYAxisLengthSq*Inner(d, YAxis);

				      			...
				      			...

				      			real32 rawTexelX = U*(real32)(Texture->Width - 1);
				      			real32 rawTexelY = V*(real32)(Texture->Height - 1);

				      			int32 X = (int32) ( rawTexelX + 0.5f);
				      			int32 Y = (int32) ( rawTexelY + 0.5f);

				      			...
				      			...
				      		}
				        }
				    }
				}



first thing to note that is that we are passed a v2 Origin, which is float

and as we loop through our pixels, we are doing 

	            v2 PixelP = V2i(X, Y);
	            v2 d = PixelP - Origin;

this is important becuz we retain the fractional information in the "v2 d = PixelP - Origin;" subtraction


but then with the following, which we round the nearest X, Y. And then we use the X, Y to look up the texel we want

      			real32 xRatio = U*(real32)(Texture->Width - 1);
      			real32 yRatio = V*(real32)(Texture->Height - 1);

      			int32 X = (int32) ( xRatio + 0.5f);
      			int32 Y = (int32) ( yRatio + 0.5f);





44:08
the key is to do a better job of finding X and Y, essentially where we are in the texture map. That way 
we can produce results that are subpixel accurate

assume we have our texels. and also we have our pixel. Assume the pixel wants the texel value at O.
Right now what we are doing is that we just directly taking the color of texel A

what we really want is a bilinear blended value between the four texels. 

				 _______________________________			
				|		|		|		|		|	
				|	A	|	B	| 		|		|
				|	  O	|		| 		|		|
				|_______|_______|_______|_______|
				|		|		|		|		|	
				|	C	|	D	| 		|		|
				|		|		| 		|		|
				|_______|_______|_______|_______|
				|		|		|		|		|	
				|		|		| 		|		|
				|		|		| 		|		|
				|_______|_______|_______|_______|
				|		|		|		|		|	
				|		|		| 		|		|
				|		|		| 		|		|
				|_______|_______|_______|_______|


so the idea is just to lerp( lerp(A, B), lerp(C, D) );




47:24

so now we just change the code to do the bilinear blending 
note that since we are bilinear filtering, we get rid of 0.5 offset in 

      			real32 rawTexelX = U*(real32)(Texture->Width - 1);
      			real32 rawTexelY = V*(real32)(Texture->Height - 1);


Code below: 
				internal void
				DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
				                    loaded_bitmap *Texture)
				{
				    real32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
				    real32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);
				    
				    ...
				    ...

				    for(int Y = YMin; Y <= YMax; ++Y)
				    {
				        uint32 *Pixel = (uint32 *)Row;
				        for(int X = XMin; X <= XMax; ++X)
				        {

				            v2 PixelP = V2i(X, Y);
				            v2 d = PixelP - Origin;
				      
				      		if(.....4 edge tests....)
				      		{
				      			real32 U = InvXAxisLengthSq*Inner(d, YAxis);
				      			real32 V = InvYAxisLengthSq*Inner(d, YAxis);

				      			...
				      			...

				      			real32 rawTexelX = U*(real32)(Texture->Width - 1);
				      			real32 rawTexelY = V*(real32)(Texture->Height - 1);

				      			int32 X = (int32)tX;
				      			int32 Y = (int32)tY;

				      			real32 fX = tX - (real32)X;
				      			real32 fY = tY - (real32)Y;

				      			...
				      			...


				                uint8 *TexelPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
				                uint32 TexelPtrA = *(uint32 *)(TexelPtr);
				                uint32 TexelPtrB = *(uint32 *)(TexelPtr + sizeof(uint32));
				                uint32 TexelPtrC = *(uint32 *)(TexelPtr + Texture->Pitch);
				                uint32 TexelPtrD = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));


				                v4 Texel = Lerp(Lerp(TexelA, fX, TexelB),
				                                fY,
				                                Lerp(TexelC, fX, TexelD));


				      		}
				        }
				    }
				}



49:10
as explained, it is possible that we will fetch out of bounds of the texture when we try to fetch a texel
For example, in the example below, B and D may be out of bounds. 



				 _______________________________			
				|		|		|		|		|	
				|		|		| 		|	A	|	B
				|	  	|		| 		|	  O |
				|_______|_______|_______|_______|
				|		|		|		|		|	
				|		|		| 		|	C	|	D
				|		|		| 		|		|
				|_______|_______|_______|_______|
				|		|		|		|		|	
				|		|		| 		|		|
				|		|		| 		|		|
				|_______|_______|_______|_______|
				|		|		|		|		|	
				|		|		| 		|		|
				|		|		| 		|		|
				|_______|_______|_______|_______|


So just for now, he is going to do. We are just going to pretend our texture is smaller than it actually is.

      			real32 rawTexelX = U*(real32)(Texture->Width - 2);
      			real32 rawTexelY = V*(real32)(Texture->Height - 2);




57:40
overall a lot better, but you still see the stuttery motion when the tree tries to come to a stop






1:16:40
is this code we are writing gonna be much different from the GPU code?

Not really 

currenty the code looks like this 

				internal void
				DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
				                    loaded_bitmap *Texture)
				{
				    ...
				    ...

				    for(int Y = YMin; Y <= YMax; ++Y)
				    {
				        for(int X = XMin; X <= XMax; ++X)
				        {			      
				      		if(....pixel test....)
				      		{

				      			... rendering code ...

				      		}
				        }
				    }
				}


the part of 

			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        for(int X = XMin; X <= XMax; ++X)
			        {			      
			      		if(....pixel test....)

that is built in in the GPU

the 			... rendering code ...

that is just shader code 







1:17:41
there is a performance benefit of using quad as primitive in hardware as well. 

if you are doing a sprite engine, quad is the shit

Quad does not work as well in 3D though
