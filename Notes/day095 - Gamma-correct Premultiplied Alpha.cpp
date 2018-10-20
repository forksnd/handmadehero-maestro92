Handmade Hero Day 095 - Gamma-correct Premultiplied Alpha




Summary:

attempted to explain how to put gamma correction and Premultiplied alpha in the rendering pipeline.

Honestly Casey does a poor job in this episode. I didnt quite understand some of his points.

I would recommend look up other sites that explains how this works:
http://renderwonk.com/blog/index.php/archive/adventures-with-gamma-correct-rendering/


did a lot of clean up


Keyword:
Rendering, Gamma-correction, Premultiplied alpha 








2:55
if you doing gamma corrected rgb, there is this question of when do you do Premultiplied alpha 



15:48
discusses how do want to do Premultiplied alpha with gamma correction


Solution 1:
we convert rgb to linear space, then multiply it with alpha

input image from artist 

read image file into Loaded_bitmaps:

				load in <sRGB, a>		-------> 	convert to linear rgb 		and	 	we store it as 	
													multiply color channel  			Premultiplied alpha <linear rgb, a> 
													with alpha 


When Rendering To Buffer:


				 ___________						
				|			|	Premultiplied alpha <linear rgb, a> 			
				|	Image 	|					 
				|___________|


				 _______________________					
				|						|				
				|						|	Premultiplied alpha						
				|						|	pixel1 = <sRGB, a>		
				|						|			
				|		frameBuffer		|			
				|						|				
				|						|				
				|						|			 
				|_______________________|


				pixel0 as it is 
				pixel1 ---> linear space 
				blend





Casey says:				

to go back from buffer format to "input format", we would have to divide it by the alpha, or multiply it by the inverse alpha

and in cases where alpha is 0, you face the problem of 1/0, and we do not want that or even to handle that cuz that leads to 
branching logic 


which i did not really understand what he meant...... in what scenarios will we want to work in the "input format"?






Solution 2:

input image from artist 

read image file into Loaded_bitmaps:

				load in < sRGB, a >		-------> 	convert to linear rgb 		------> 	convert back to sRGB space 
													Premultiply alpha 						store back as <sRGB, a>
																

When Rendering To Buffer:

				load loaded_bitmaps 	-------->	covert to linear rgb
				in <sRGB, a>


				 ___________						
				|			|	Premultiplied alpha					
				|	Image 	|	pixel0 = <sRGB, a>					 
				|___________|


				 _______________________					
				|						|				
				|						|	Premultiplied alpha						
				|						|	pixel1 = <sRGB, a>		
				|						|			
				|		frameBuffer		|			
				|						|				
				|						|				
				|						|			 
				|_______________________|


			pixel0 ---> linear space 
			pixel1 ---> linear space 
			blend



that way the buffer will have Premultiplied alpha rgb values  						

to go back from buffer format to input format, we would have to divide it by the alpha, or multiply it by the inverse alpha

and in cases where alpha is 0, you face the problem of 1/0, and we do not want that or even to handle that cuz that leads to 
branching logic 

so this solution is no good 











19:43
so right now our code looks like: we store our bitmaps in Premultiplied alpha that is Gamma corrected.


				internal loaded_bitmap
				DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName)
				{
				    loaded_bitmap Result = {};
				    
				    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);    
				    if(ReadResult.ContentsSize != 0)
				    {
				    	...
				    	...

				        
				        uint32 *SourceDest = Pixels;
				        for(int32 Y = 0; Y < Header->Height; ++Y)
				        {
				            for(int32 X = 0; X < Header->Width; ++X)
				            {
				                uint32 C = *SourceDest;

				                v4 Texel = {(real32)((C & RedMask) >> RedShiftDown),
				                            (real32)((C & GreenMask) >> GreenShiftDown),
				                            (real32)((C & BlueMask) >> BlueShiftDown),
				                            (real32)((C & AlphaMask) >> AlphaShiftDown)};

				                Texel = SRGB255ToLinear1(Texel);

				                Texel.rgb *= Texel.a;

				                Texel = Linear1ToSRGB255(Texel);
				                
				                *SourceDest++ = (((uint32)(Texel.a + 0.5f) << 24) |
				                                 ((uint32)(Texel.r + 0.5f) << 16) |
				                                 ((uint32)(Texel.g + 0.5f) << 8) |
				                                 ((uint32)(Texel.b + 0.5f) << 0));
				            }
				        }
				    }

				    Result.Pitch = -Result.Width*BITMAP_BYTES_PER_PIXEL;
				    Result.Memory = (uint8 *)Result.Memory - Result.Pitch*(Result.Height - 1);
				    
				    return(Result);
				}



20:24
added the rgb field to v4 
				
initially, we had 

				union v4
				{
					...
					...

					struct
					{
						real32 r, g, b, a;
					};
					struct
					{
						v3 rgb;
						real32 a;
					};
					real32 E[4];
				};


but the a in 
				"real32 r, g, b, a;" 
and 
				"v3 rgb;
				 real32 a;"

 may collide, so he changed it to 


				union v4
				{
				    struct
				    {
				        real32 x, y, z, w;
				    };
				    struct
				    {
				        union
				        {
				            v3 rgb;
				            struct
				            {
				                real32 r, g, b;
				            };
				        };
				        
				        real32 a;        
				    };
				    real32 E[4];
				};



 
39:30
got rid of the header in render elements



the idea is that previously we were doing 


				 ___________________________			
				|							|
				|			Header			|	
				|	-	-	-	-	-	-	|
				|							|	
				|			data			|
				|							|	
				|							|
				|							|
				|							|
				|							|
				|___________________________|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|___________________________|
				
now we have. This way people who are working on the data, never have to worry about the header

				 ___________________________			
				|							|
				|			Header			|	
				|___________________________|
				|							|
				|			data			|
				|							|	
				|							|
				|							|
				|							|
				|							|
				|___________________________|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|___________________________|
				

we changed the PushRenderElement code 
notice that we did the 

				Size += sizeof(render_group_entry_header);

size initialy is just the size of the data portion, we have to make room for the header				

				#define PushRenderElement(Group, type) (type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type)
				inline void *
				PushRenderElement_(render_group *Group, uint32 Size, render_group_entry_type Type)
				{
				    void *Result = 0;

				    Size += sizeof(render_group_entry_header);
				    
				    if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
				    {
				        render_group_entry_header *Header = (render_group_entry_header *)(Group->PushBufferBase + Group->PushBufferSize);
				        Header->Type = Type;
				        Result = (uint8 *)Header + sizeof(*Header);
				        Group->PushBufferSize += Size;
				    }
				    else
				    {
				        InvalidCodePath;
				    }

				    return(Result);
				}





46:44
in visual c++, there are ways to optimize things with optimization directives
introduced the #pragma optimize





1:01:53
glsl has nothing to do with rendering at all

glsl is shading computations. Rendering is not in glsl, only the shading computations are in glsl



1:06:24
how would something like a dynamic array work in the current memory system?

if you end up wanting something that is totaly dynamic, like a dynamic allocator, then you have to write 
a dynamic allocator. that is what things like malloc or new are doing other the hood.

they are taking large chunks of memory and portioning them up. Then if they produce holes, they later try to 
fill those holes with other allocations that are similarly sized.

so if you need a dynamic allocator, you can write one, and they are not particularly hard.

Casey also argues: why do you need one?

dynamic array is suceptible to memory fragmentation, and less predictable





1:10:44
all modern operating system discard 100% of your memory when you close. so you dont have to free any of it



1:13:31

re-arranged some code 
previously we had 

				real32 InvRSA = (1.0f - Texel.a);
                v4 Blended = {InvRSA*Dest.r + Texel.r,
                              InvRSA*Dest.g + Texel.g,
                              InvRSA*Dest.b + Texel.b,
                              Texel.a + Dest.a - Texel.a * Dest.a);

now we have 
				real32 InvRSA = (1.0f - Texel.a);
                v4 Blended = {InvRSA*Dest.r + Texel.r,
                              InvRSA*Dest.g + Texel.g,
                              InvRSA*Dest.b + Texel.b,
                              InvRSA*Dest.a + Texel.a);
which becomes 

				real32 InvRSA = (1.0f - Texel.a);
                v4 Blended = InvRSA*Dest + Texel;
or 
				v4 Blended = (1.0f - Texel.a)*Dest + Texel;
