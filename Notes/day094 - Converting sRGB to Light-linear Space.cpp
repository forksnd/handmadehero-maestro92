Handmade Hero Day 094 - Converting sRGB to Light-linear Space

Summary:

introduced the concept of gamma-corrected rgb values.

showed that most monitors follows a x_out = x ^ 2.2 gamma curve 

decided to use x_out = x ^ 2 to approximate it 

showed us a few links 
https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting
http://filmicworlds.com/blog/linear-space-lighting-i-e-gamma/

Had the decision to keep the framebuffer in linear space or rgb space.
decided to keep it in rgb space.


Keyword:
Rendering, gamma-corrected rgb 





2:48
gamma corrected rendering pipelines have been in the works for 4 years ish
certainly in uncharted 2, the last of us,

gamma corrected rendering ensures that colors are blended "correctly"





4:54
shows a graph of 


GrayLevel on the x axis, that is from 0 ~ 255, essentially the pixel value
Brightness on the y axis 

5:13
the brightness is in units of cd/m^2. That is the brightness that the monitor is showing
as you can see, it is not linear

this means that for every pixel value you are sending to your monitor, 

in the low range, it is getting less and less bright

in the high range, it gets a lot brighter as you go

Apparently this is what monitors do. Standard behaviour from monitors



Gamma correction controls the overall brightness of an image





acoording to the link below,
the gamma symbol represents a numerical parameter that describes the nonlinear relationship
between pixel value and luminance (birghtness);. 
https://poynton.ca/notes/colour_and_gamma/GammaFAQ.html





7:46 
casey says that it might be becuz
your eyes are more sensitive to contrast in darker areas, than it is in bright areas

this is just an aspect of human vision







8:58
the ultimate question is: why do we care?

Maths!


10:56
even in our bilinear filtering, we are not really blending our colors properly


for example, if you blend RGB (0,0,0) and RGB (1,1,1), where the two represents the darkest and brightest color 
that the monitor can output, we actualloy get dark gray on our gamma graph (See graph at 11:37);

it is no where near of the 50% brightness spot


essentially all the "Lerp" we are doing is incorrect becuz we are doing in RGB space




13:27
the artists are actaully making the image in Gamma space
assuming the artsits are using photoshop on the default settings, it is called "sRGB"
which sort of like the gamma curve with a slight tweak

so if you think about it, if we just directly blit our images from the artist without doing any math on it
it actually looks correct. But if you do any lerp or filtering, it is no longer correct.



15:05
so the key is 
whenever we want to do math, we convert image from sRGB space into gamma-linear space, do our math
then convert them back to sRGB space

	artists 					Marts 						Monitor
	sRGB  -------------------->	Linear ------------------->	sRGB
	 ___________				 ___________				 ___________
	|			|				|			| 				|			| 
	|	Image 	|	--------->	|			| 	---------> 	|	output	| 
	|___________|				|___________| 				|___________|

this is what Triple A games do, what a competent engine do 


15:54
what we need to do is that we need to find a way to straighten out the curve

sRGB actually has a wikipedia link
https://en.wikipedia.org/wiki/SRGB

from that page you can see that the formula is fairly complicated
so we want to do it exactly, we would need a lookup table



So Casey refered us to another link 

http://filmicworlds.com/blog/linear-space-lighting-i-e-gamma/

apparently these are the slides from the uncharted presentation from one of the GDC


from the wikipedia the general formula is 

				Vout = A * (Vin ^ Gamma)



at 18:58
you can see that in the powerpoint slide, the formula is approximately


				F(x) = pow(x, 2.2);

A is 1
Gamma is 2.2
x is sRGB value 
F(x); is the brigthness that will be displayed on the monitor


apparently this value differs depending on the monitor? and monitors are said to have around a 2.2
gamma value


19:15 shows all the transformation going back and forth




	artists 							Linear Gamma Space 							Monitor
	sRGB  																			sRGB
	 
				F(x) = pow(sRGB, 2.2);					sRGB = pow(F(x), 0.45);	

				-------------------->					------------------->
	 ___________						 ___________								 ___________
	|			|						|			| 								|			| 
	|	Image 	|						|			| 				 				|	output	| 
	|___________|						|___________| 								|___________|

				<--------------------					<--------------------

				sRGB = pow(F(x), 0.45); 				F(x) = pow(sRGB, 2.2); 



20:43
the orignal formula is 

	F(x) = pow(sRGB, 2.2);	

we will approximate it using 

	F(x) = pow(sRGB, 2.0);


proceeds to show the graphs of the two in google and you can see they are not too far off
especially in the range of [0,1]



25:03
the framebuffer can either be in sRGB space or gamma linear space. we have to decide which one do we want

in our case, becuz we are a software renderer, we are not doing a lot of 3D constructive stuff. we are taking bitmaps 
that are mostly finished art, maybe doing a little re-lighting on them, we probably wont get much out of a linear framebuffer




if you have a linear framebuffer, we do can possibly save multiple transformation of the Linear to sRGB 

so we have our sRGB images, we convert to liearn then we blit it to the screen
	 ___________						
	|			|						
	|	Image 	|						 
	|___________|


	 _______________________					
	|						|				
	|						|				
	|						|	
	|						|			
	|		frameBuffer		|			
	|						|				
	|						|				
	|						|			 
	|_______________________|

then only at the very end, when we go from this phrameBuffer to monitor, we do a global linear to sRGB transformation.
if we do not have a linear framebuffer, you will have to do the transformation from linear to sRGB when you blit each 
individual image to the frameBuffer

But Casey says we are mostly going to care about bandwidth. In order to store a linear buffer, we would need at least 
16 bits per channel instead of 8 (per RGB channel that is); This is becuz 8 bits channel simply isnt enough to store linear color without 
banding

Note:
banding means "The presence or formation of visible sripes of contrasting color"

the reason for that is becuz (I didnt quite understand this part);
if you were to straghten the curve out, you wont have enough resolution, so you will see banding.

however, we do not want to do that becuz that will double the bandwidth requiresments for our reads and writes to our framebuffer 
which we do not want to do.

There are ways to get around that. If for some reason that we really need a linear frame buffer, it is entirely possible to design
our rasterizer around that. but for now Casey thinks we dont need a linear one 



28:32
Casey finds the slide that he wanted
https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting




29:42
essentially we want to mimic what the monitor is doing to make uniform brigthness out of these values 

so what we want to do is instead of 2.2, we do it with gamma of 2


	SRGB 							Linear Gamma Space 								Monitor
	  																				sRGB
	 
				F(x) = pow(sRGB, 2.0);					sRGB = sqrt(F(x));	

				-------------------->					------------------->
	 ___________						 ___________								 ___________
	|			|						|			| 								|			| 
	|	Image 	|						|			| 				 				|	output	| 
	|___________|						|___________| 								|___________|






30:41
here is what the code looks like 

as we get the four texels while doing subpixel rendering, we call the function SRGB255ToLinear1();
on both the destination and the source color.

after lerping, we call Linear1ToSRGB255();

then we blend

				internal void
				DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
				                    loaded_bitmap *Texture)
				{
					...
					...

				    for(int Y = YMin; Y <= YMax; ++Y)
				    {
				        uint32 *Pixel = (uint32 *)Row;
				        for(int X = XMin; X <= XMax; ++X)
				        {
				                
				                // TODO(casey): Formalize texture boundaries!!!
								...
								...
				                
				                uint8 *TexelPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
				                uint32 TexelPtrA = *(uint32 *)(TexelPtr);
				                uint32 TexelPtrB = *(uint32 *)(TexelPtr + sizeof(uint32));
				                uint32 TexelPtrC = *(uint32 *)(TexelPtr + Texture->Pitch);
				                uint32 TexelPtrD = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));

				                // TODO(casey): Color.a!!
				                v4 TexelA = {(real32)((TexelPtrA >> 16) & 0xFF),
				                             (real32)((TexelPtrA >> 8) & 0xFF),
				                             (real32)((TexelPtrA >> 0) & 0xFF),
				                             (real32)((TexelPtrA >> 24) & 0xFF)};
				                v4 TexelB = {(real32)((TexelPtrB >> 16) & 0xFF),
				                             (real32)((TexelPtrB >> 8) & 0xFF),
				                             (real32)((TexelPtrB >> 0) & 0xFF),
				                             (real32)((TexelPtrB >> 24) & 0xFF)};
				                v4 TexelC = {(real32)((TexelPtrC >> 16) & 0xFF),
				                             (real32)((TexelPtrC >> 8) & 0xFF),
				                             (real32)((TexelPtrC >> 0) & 0xFF),
				                             (real32)((TexelPtrC >> 24) & 0xFF)};
				                v4 TexelD = {(real32)((TexelPtrD >> 16) & 0xFF),
				                             (real32)((TexelPtrD >> 8) & 0xFF),
				                             (real32)((TexelPtrD >> 0) & 0xFF),
				                             (real32)((TexelPtrD >> 24) & 0xFF)};

				                // NOTE(casey): Go from sRGB to "linear" brightness space
				                TexelA = SRGB255ToLinear1(TexelA);
				                TexelB = SRGB255ToLinear1(TexelB);
				                TexelC = SRGB255ToLinear1(TexelC);
				                TexelD = SRGB255ToLinear1(TexelD);
				                
		
				                v4 Texel = Lerp(Lerp(TexelA, fX, TexelB),
				                                fY,
				                                Lerp(TexelC, fX, TexelD));
		

				                real32 RSA = Texel.a; 
				                
				                v4 Dest = {(real32)((*Pixel >> 16) & 0xFF),
				                           (real32)((*Pixel >> 8) & 0xFF),
				                           (real32)((*Pixel >> 0) & 0xFF),
				                           (real32)((*Pixel >> 24) & 0xFF)};
				                
				                // NOTE(casey): Go from sRGB to "linear" brightness space
				                Dest = SRGB255ToLinear1(Dest);
				                
				                real32 RDA = Dest.a;
				            
				                real32 InvRSA = (1.0f-RSA);
				                
				                v4 Blended = {InvRSA*Dest.r + Texel.r,
				                              InvRSA*Dest.g + Texel.g,
				                              InvRSA*Dest.b + Texel.b,
				                              (RSA + RDA - RSA*RDA)};

				                // NOTE(casey): Go from "linear" brightness space to sRGB
				                v4 Blended255 = Linear1ToSRGB255(Blended);

				                *Pixel = (((uint32)(Blended255.a + 0.5f) << 24) |
				                          ((uint32)(Blended255.r + 0.5f) << 16) |
				                          ((uint32)(Blended255.g + 0.5f) << 8) |
				                          ((uint32)(Blended255.b + 0.5f) << 0));
				            }
				#else
				            *Pixel = Color32;
				#endif
				            
				            ++Pixel;
				        }
				        
				        Row += Buffer->Pitch;
				    }
				}







Note the function names 

				SRGB255ToLinear1();
				Linear1ToSRGB255();

notice that we have the words SRGB255 and Linear1, that is to indicate we are going from [0 ~ 255] to [0 ~ 1] ranges
				    


32:56
Casey mentioend "gotta loved that windows ordering"

                v4 Dest = {(real32)((*Pixel >> 16) & 0xFF),
                           (real32)((*Pixel >> 8) & 0xFF),
                           (real32)((*Pixel >> 0) & 0xFF),
                           (real32)((*Pixel >> 24) & 0xFF)};





32:26
writes the two functions SRGB255ToLinear1(); and Linear1ToSRGB255();
we do not square the alpha value. The alpha value is strictly a value that tells us how much do we want to blend 


				handmade_render_group.cpp

				inline v4
				SRGB255ToLinear1(v4 C)
				{
				    v4 Result;

				    real32 Inv255 = 1.0f / 255.0f;
				    
				    Result.r = Square(Inv255*C.r);
				    Result.g = Square(Inv255*C.g);
				    Result.b = Square(Inv255*C.b);
				    Result.a = Inv255*C.a;

				    return(Result);
				}

				inline v4
				Linear1ToSRGB255(v4 C)
				{
				    v4 Result;

				    real32 One255 = 255.0f;

				    Result.r = One255*SquareRoot(C.r);
				    Result.g = One255*SquareRoot(C.g);
				    Result.b = One255*SquareRoot(C.b);
				    Result.a = 255.0f*C.a;

				    return(Result);
				}




51:15
initially we have our pixel value to be. Recall that Texel is premultiplied alpha
so the we have the formula = (1-source alpha) * dest_r + premultipled_alpha_source_r

                TexelA = SRGB255ToLinear1(TexelA);
                TexelB = SRGB255ToLinear1(TexelB);
                TexelC = SRGB255ToLinear1(TexelC);
                TexelD = SRGB255ToLinear1(TexelD);

                v4 Texel = Lerp(Lerp(TexelA, fX, TexelB),
                                fY,
                                Lerp(TexelC, fX, TexelD));

                real32 RSA = Texel.a; 			           
				                
                v4 Dest = {(real32)((*Pixel >> 16) & 0xFF),
                           (real32)((*Pixel >> 8) & 0xFF),
                           (real32)((*Pixel >> 0) & 0xFF),
                           (real32)((*Pixel >> 24) & 0xFF)};


                Dest = SRGB255ToLinear1(Dest);

                real32 RDA = Dest.a;
            
                real32 InvRSA = (1.0f-RSA);


                v4 Blended = {InvRSA*Dest.r + Texel.r,
                              InvRSA*Dest.g + Texel.g,
                              InvRSA*Dest.b + Texel.b,
                              (RSA + RDA - RSA*RDA)};



Casey mentions that he wants to start utilizing the Color argument that is passed into the function


so assuming we have our texel r, g, b. And if we want to tint this, we can add coefficients
				
				cr * r, cg * g, cb * b

we essentially control how much red, green and blue we read out of our texture map

also we want the color that is passed in to be premultiplied with alpha as well, so we got 

two major changes 

                real32 RSA = Texel.a * Color.a;		
and 
                InvRSA*Dest.r + (Color.a * Color.r) * Texel.r


so the full code is:

				Color.a * Color.r


                TexelA = SRGB255ToLinear1(TexelA);
                TexelB = SRGB255ToLinear1(TexelB);
                TexelC = SRGB255ToLinear1(TexelC);
                TexelD = SRGB255ToLinear1(TexelD);

                v4 Texel = Lerp(Lerp(TexelA, fX, TexelB),
                                fY,
                                Lerp(TexelC, fX, TexelD));

                real32 RSA = Texel.a * Color.a;		           
				                
                v4 Dest = {(real32)((*Pixel >> 16) & 0xFF),
                           (real32)((*Pixel >> 8) & 0xFF),
                           (real32)((*Pixel >> 0) & 0xFF),
                           (real32)((*Pixel >> 24) & 0xFF)};


                Dest = SRGB255ToLinear1(Dest);

                real32 RDA = Dest.a;
            
                real32 InvRSA = (1.0f-RSA);


                v4 Blended = {InvRSA*Dest.r + Color.a * Color.r * Texel.r,
                              InvRSA*Dest.g + Color.g * Color.g * Texel.g,
                              InvRSA*Dest.b + Color.b * Color.b * Texel.b,
                              (RSA + RDA - RSA*RDA)};





1:14:54
do all monitors use the same 2.2 curve?

Nope. 2.2 is an approximation of what all monitors are trying to do. 



