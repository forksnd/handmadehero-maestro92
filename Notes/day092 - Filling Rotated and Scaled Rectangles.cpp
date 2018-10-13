Handmade Hero Day 092 - Filling Rotated and Scaled Rectangles

Summary:
added supported for filling roated and scaled rectangles.
did so by testing whether a pixel is inside a rotated rectangle

the testing is done by doing 4 edges tests, 

wrote the 2D Perp(); operation 

explain how mipmapping works in the Q/A
explain how anti-aliasing works in the Q/A


Keyword:
renderer, rotated rectangle collision detection, mipmap



8:17
plans to add support to render a solid rectangle in the renderer



8:43
Collision Detection and Rendering are not that different

pixel filling has a lot to do with collision detection





22:27
discussing how to test whether a pixel is inside a rotated rectangle 

traditionally you would do

				if(	x >= xmin 	&&
					x < xmax 	&&    (recall Casey is doing exclusive max)
					y >= ymin	&&
					y < ymax ) {}

what essentially this is doing is that point <x,y> is doing 4 edge tests
making sure that <x,y> is inside all 4 rectangle edge

so even if we have a rotated rectangle, we can do the same thing 


				if(	x >= xmin 	&&
					x < xmax 	&&    
					y >= ymin	&&
					y < ymax ) {}


becomes 
				if(	edgeTest(0)	&&
					edgeTest(1) &&    
					edgeTest(2)	&&
					edgeTest(3) ) {}



24:50 
Casey doing it the the Brute force way.
we loop through all the screen pixels, test whether each single pixel is inside my rotated rectangle

				

				p1				
				-----------------			
				|				|
				|				|
				|		p		|
				|				|
				|				|
				-----------------
				p0 				p2


so if we want to know if point p is on the inside of p0 and p2, 
we just need to do the dot(vector_p_po, vector_p1_po);

if the dot product is +, then its inside, if -, then outside





35:18
so for the edge test, we are essentially testing whether a point is on a certain side of an edge 
through dot product, and we check the sign of the dot product value.

Casey mentioned that we tend to think things on the outside of a surface as positive.
things on the inside of a surface as negative 

				-----------------								
				| 				|
				|				|		
				|				|
				|		-		|
				|				|
				|				|
				-----------------
				p0 				p2

						+

so he wrote the convention of 

	            
	            if((Edge0 < 0) &&
	               (Edge1 < 0) &&
	               (Edge2 < 0) &&
	               (Edge3 < 0))






40:38
starting to write the proper edge test for this function 


note that if we have a rectangle below

				p1
				^----------------
				|				|
				| 				|
				|				|		
				|				|
				|				|
				|				|
				|				|
				----------------->
				p0 				p2

the XAxis and YAxis is 

			-----------------> is XAxis
			p0				p2


			p1
			^
			|
			|
			|
			|
			|
			p0

			is YAxis 




Note that the first edge test is 

	            real32 Edge0 = Inner(PixelP - Origin, -YAxis);


				p1
				^----------------
				|				|
				| 				|
				|				|		
				|				|
				|		p		|
				|				|
				|				|
				----------------->
				p0 		|		p2
						|
						|
						v
		
we are testing the vector p0_p with the normal of edge p0_p2
so we do the dot(PixelP - Origin, -YAxis);






42:08

we do the same for all other edges 

				internal void
				DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color)
				{    
				    uint32 Color32 = .... what color i need

				    uint8 *Row = ((uint8 *)Buffer->Memory;
				    for(int Y = YMin; Y <= YMax; ++Y)
				    {
				        uint32 *Pixel = (uint32 *)Row;
				        for(int X = XMin; X <= XMax; ++X)
				        {
				            v2 PixelP = V2i(X, Y);

				            real32 Edge0 = Inner(PixelP - Origin, -YAxis);
				            real32 Edge1 = Inner(PixelP - (Origin + XAxis), XAxis));
				            real32 Edge2 = Inner(PixelP - (Origin + XAxis + YAxis), YAxis);
				            real32 Edge3 = Inner(PixelP - (Origin + YAxis), -XAxis));
				            
				            if((Edge0 < 0) &&
				               (Edge1 < 0) &&
				               (Edge2 < 0) &&
				               (Edge3 < 0))
				            {
				                *Pixel = Color32;
				            }

				            
				            ++Pixel;
				        }
				        
				        Row += Buffer->Pitch;
				    }
				}




31:00

Casey went through multple versions, step by step do write the edges tests

Personally I do not know why he went though all these trouble to write all this.... I guess he just wants to
show all the noobs how to do this step by step


version 1
				x >= xmin &&
				x < xmax &&    
				y >= ymin &&
				y < ymax 


version 2
				x - vMin.x >= 0 &&
				x - vMax.x < 0 &&
				y - vMin.y >= 0 &&
				y - vMax.y < 0



version 3
	            real32 Edge0 = vMin.x - x;
	            real32 Edge1 = x - vMax.x;
	            real32 Edge2 = vMin.y - y;
	            real32 Edge3 = y - vMax.y;
	            
	            if((Edge0 < 0) &&
	               (Edge1 < 0) &&
	               (Edge2 < 0) &&
	               (Edge3 < 0))

version 4
	            real32 Edge0 = Inner(PixelP - vMin, v2(-1, 0));
	            real32 Edge1 = Inner(PixelP - vMax, v2(1, 0));
	            real32 Edge2 = Inner(PixelP - vMin, v2(0, -1));
	            real32 Edge3 = Inner(PixelP - vMax, v2(0, 1));
	            
	            if((Edge0 < 0) &&
	               (Edge1 < 0) &&
	               (Edge2 < 0) &&
	               (Edge3 < 0));




44:49
instead of testing all the pixel on screen, we only test the pixels bounded by the min and max
of the AABB formed by the min and max of your rotated rectangle





56:10
someone asks in the Q A: would skewing the axis still work?

No. 
we currently have 
	            real32 Edge0 = Inner(PixelP - Origin, -YAxis);
	            real32 Edge1 = Inner(PixelP - (Origin + XAxis), XAxis));
	            real32 Edge2 = Inner(PixelP - (Origin + XAxis + YAxis), YAxis);
	            real32 Edge3 = Inner(PixelP - (Origin + YAxis), -XAxis));

that is assume XAxis and YAxis are orthogonal.

to have make it work with skewed axis woudl have  

	            real32 Edge0 = Inner(PixelP - Origin, -Perp(XAxis));
	            real32 Edge1 = Inner(PixelP - (Origin + XAxis), -Perp(YAxis));
	            real32 Edge2 = Inner(PixelP - (Origin + XAxis + YAxis), Perp(XAxis));
	            real32 Edge3 = Inner(PixelP - (Origin + YAxis), Perp(YAxis));
	            

we added the Perp function in handmade_math.h

				inline v2
				Perp(v2 A)
				{
				    v2 Result = {-A.y, A.x};
				    return(Result);
				}



1:01:26
if downsizing bitmap correctly slower than upscaling?

not necessarily


when you down sample, you still doing interpolation for every 4 cells to one cell
				 _______________________________			
				|		|		|		|		|	
				|		|		| 		|		|
				|	A1	|	A2	| 		|		|
				|_______|_______|_______|_______|
				|		|		|		|		|	
				|		|		| 		|		|
				|	A3	|	A4	| 		|		|
				|_______|_______|_______|_______|
				|		|		|		|		|	
				|		|		| 		|		|
				|		|		| 		|		|
				|_______|_______|_______|_______|
				|		|		|		|		|	
				|		|		| 		|		|
				|		|		| 		|		|
				|_______|_______|_______|_______|


so A_ = Blend(A1, A2, A3, A4)    blend it howevery you want, interpolate or what not
 

				 _______________		
				|		|		|		
				|		|		|		
				|	A_	|		|   		
				|_______|_______|
				|		|		|			
				|		|		| 		
				|		|		| 		
				|_______|_______|



when you are upscaling, you are just copying values from 1 cell to every 4 cell.
so not necessarily. So donwsizing or upscaling are equally expensive if you talking about 0.5x or 2x 


typically no one downscales bitmaps more than 2x.

what they do is mipmapping.

when you have other numbers of shrinkage, they just use mipmaps 

so you precompute your texture at different powers of 2 resolutions 

assume you start with 

256 x 256 texture 
128 x 128
64 x 64
32 x 32
16 x 16
8 x 8
4 x 4
2 x 2
1 x 1

so if you to downsize to 30 x 30
you would do trilinear filtering, taking next highest and next lowest
so 32 x 32 and 16 x 16 in this case
and so you would sample 4 texels from both mipmap to get one destinatino texel 



1:05:48
will we be working on anti-aliasing?

dont want to explain hy in detail right now. But the reason is that since we are pulling from alpha bitmaps.
it turns out we will get anti-aliasing for free.

so we do not need to worry about multi-sampling

if you are drawing actual shapes whose edges are visible, thats when you have to worry about anti-aliasing

this is becuz when we render alpha-bitmaps, the texture sampling handles anti-aliasing for us

but if we are drawing a solid rectangle that didnt have any alpha, then we do not have texture sampling to 
save us.

that is why graphics card have MSAA "multi-sampling"

you never need multi-sampling to anti-aliase bitmaps blits that have alphas 

what you need is for the edges

how it works is that it takes many sample inside a pixel of the edges that passes through the pixel.
then it does some smoothing.

we do not have to do this cuz we have a contour of alpha that smooths our the edges for us.



Note that none of this is true for 3D







1:13:05
Note that the edge test we did, that will work with any convex shape 

