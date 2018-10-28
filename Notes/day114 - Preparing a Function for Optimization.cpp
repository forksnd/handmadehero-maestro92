Handmade Hero Day 114 - Preparing a Function for Optimization

Summary:
setup to optimize the DrawRectangleSlowly(); function
replaced the edge test to simply do UV test 

flatten out all the operation in DrawRectangleSlowly();

discussed possible "Wide" strategies
either pixel0, pixel1, pixel2, pixel3 
or 
rgba 

opt to go for the 4 pixel strategy cuz it is closer to GPU 


Keyword:
optimization, SIMD






8:18
starting to look at DrawRectangleSlowly in great detail


11:47
we first modify the edge tests 

initially we have 
	            real32 Edge0 = Inner(d, -Perp(XAxis));
	            real32 Edge1 = Inner(d - XAxis, -Perp(YAxis));
	            real32 Edge2 = Inner(d - XAxis - YAxis, Perp(XAxis));
	            real32 Edge3 = Inner(d - YAxis, Perp(YAxis));
	            
	            if((Edge0 < 0) &&
	               (Edge1 < 0) &&
	               (Edge2 < 0) &&
	               (Edge3 < 0))

Casey mentioned that this is not efficient

so he got rid of the edge tests completely and just directly compute the UV coordinates 

                
		    v2 nXAxis = InvXAxisLengthSq*XAxis;
		    v2 nYAxis = InvYAxisLengthSq*YAxis;

            real32 U = Inner(d, nXAxis);
            real32 V = Inner(d, nYAxis);
            
            if((U >= 0.0f) &&
               (U <= 1.0f) &&
               (V >= 0.0f) &&
               (V <= 1.0f))
            {

Does this work with rotated rects?



19:37
flattening out the function so it is easier for Casey to look at one place instead of tracking 
a bunch of other functions

also this is setting up for SIMD Optimization

41:23
somethings look exactly the same, multiplies look like mulitples in SIMD. But packing and unpacking 
actually look quite different.



43:29
Casey says this unpacking is nasty 
     
                real32 TexelAr = (real32)((SampleA >> 16) & 0xFF);
                real32 TexelAg = (real32)((SampleA >> 8) & 0xFF);
                real32 TexelAb = (real32)((SampleA >> 0) & 0xFF);
                real32 TexelAa = (real32)((SampleA >> 24) & 0xFF);

which lead to the question:

What is our "Wide" strategy?

since we can operate 4 things at a time in SIMD, we have to think about what are the 4 things?

we could make it pixel0, pixel1, pixel2, pixlel3

			   ___________________________
			  |		 |		|      | 	  |
			  | RGBA | RGBA | RGBA | RGBA |
			  |______|______|______|______|

or 

red, green, blue, alpha
			   ___________________________
			  |		 |		|      | 	  |
			  | R    | G    | B    | A    |
			  |______|______|______|______|



in our code, rgba seems like a bad setup,
rgba treat the a differently, so they dont chain so well.


but for the pixels setup, we also have a problem

when we load 4 pixels in, we have unpack to get all the Rs out, all the Gs out, Bs and As

				128 bits 

			  32 bit 
			   ___________________________
			  |		 |		|      | 	  |
			  | RGBA | RGBA | RGBA | RGBA |
			  |______|______|______|______|

				|      |       |	  |
			   RRRR   GGGG    BBBB   AAAA

48:27
one thing that often happens is that you might convert your framebuffer format where
it stores all the Rs together, Gs togheter, Bs and As



49:19
the goal for this software renderer is educational, so we want to model after what GPUs do.
Since GPUs do not reformat the framebuffer, we wont do it here

The reason becuz GPUs has much more powerful packing and unpacking power


1:07:00
does the compiler automatically do any SSE optimization?
a bit. on some LLVM, definitely

not sure on visual studio
proceeds to check the SSE registers in the visual studio debugger
and says no 






