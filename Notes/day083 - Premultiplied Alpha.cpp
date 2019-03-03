Handmade Hero Day 083 - Premultiplied Alpha

Summary:

talked about different alpha value compositing formulas
discussed the following two methods: 

ADD + CLAMP
                
                SA + DA

and 
the "Coverage" Based formula, which is 

                SA + (1 - SA) * DA 

Showed when using non-premmultiplied alpha gives you unwanted black fringes when blending.

introduced how premultiplied alpha helps us remove these black fringes. 

The alpha compositing formula for premultiplied alpha is SA + DA - SA * DA

showed algebra to derive this formula.

[I find lots of Caseys explanation on Alpha values to be not very intuitive, I much prefer 
    Sean Baretts explanation in his article]

mentioned couple of links on the internet [espeically Sean Barretts]

https://nothings.org/gamedev/blend_equations_with_destination_alpha.txt

look for Sean Barretts article in the link below
http://twvideo01.ubm-us.net/o1/vault/GD_Mag_Archives/Game.Developer.2004.11.pdf

Explains that non-premmultiplied alpha doesnt work cuz the math does not work out.


says premultiplied alpha is the industry standard.

Keyword: 
Rendering, alpha compositing, alpha values, premultiplied alphas










Anootating Sean Barretts article
http://twvideo01.ubm-us.net/o1/vault/GD_Mag_Archives/Game.Developer.2004.11.pdf

-   The initial motiviation of premultiplied alpha is efficiency.

-   The game industry uses premultiplied alpha for a different reason. It doesnt interpolate properly.
    alpha-blending does not distribute over linear interpolation if you use non-premultiplied alpha

-   alpha-blending interpolated colors is non-linear
    this is also what he meant by "mixing lerp with and alpha-blending leads to problem"

    we get problems when we are alpha-blending lerped colors

-   the formula for lerp-then-blend is not linear

-   the set up: we have three things: 

                    C1, C2, background

    Blend then Lerp:
                    C1 over background,     C2 over background
                    Lerp the two 

    Lerp then Blend:                
                    C1 lerp C2
                    blend the lerped color with background



-   so Listing 1 and Listing 2 explains why the lerp-then-blend is non linear. 

    Listing 1 has the raw math.

    Listing 2 is just expressing output in Listing 1 in the form of Ar + Bs + Cz

    as you can see C is the same in both 

    but term A_tilda and B_tilda is not linear anymore. they both have lambda squared term. Therefore the lerp then blend 
    has non-linear terms

-   apparently if you use premultiplied alpha, this problem goes away. 



-   When you talk about alpha blending, it is almost most common to use the word "over"
    You always think of alpha blending as A over B, or B over C. There is always a foreground and a background

-   it is associative: [A over B] over C == A over [B over C]


-   given a background color z, and a non-premultiplied foreground color <r, a>
    Note that we just care about one channel in this case. Also we doing foreground over background, so we do not care
    about the background alpha 

    the basic alpha-blending computation <r,a> over z is just = a * r + (1 - a) * z


-   with premultiplied-alpha, you can lerp then blend and get the same result as if you blended then lerped

-   premultiplied alpha generally just turns out to be better. For lossy imgae compression of RGB, it turns out you are actually 
    better offe compressing a premultipied RBA image, even though normally its better to decorrelate channels. 

-   The bottom line is, you should probably just use premultipied alpha all the time.


-   We use alpha blending to make edges look smooth. 
So we can potentially interpret Alpha as "Coverage" values

so if you can imagine an alpha-blended sprite with an opaque interior and transparent edge.
the interior will have alpha values of 1. The outside will have alpha values of 0. The edges
will have a varying alpha values. see the pictures below:

You can interpret the alpha as a percentage of the texel covered by the opaque part of the sprite.

the alpha thing is the concept of converage, density, or population. Like for a particular texel, what percentage of this texel
is populated by your RGB color. that is what the concept of alpha is

its kind of like, within a province or region, how many of your population is asians, or caucasians 

      _____________   _____________   _____________   _____________   _____________      
     |             | |#   #   #   #| |# # # # # # #| |### ### ### #| |#############| 
     |             | |  #   #   #  | |# # # # # # #| |# # # # # # #| |#############| 
     |             | |#   #   #   #| |# # # # # # #| |### ### ### #| |#############| 
     |             | |  #   #   #  | |# # # # # # #| |# # # # # # #| |#############| 
     |             | |#   #   #   #| |# # # # # # #| |### ### ### #| |#############| 
     |_____________| |__#___#___#__| |#_#_#_#_#_#_#| |#_#_#_#_#_#_#| |#############| 


so if we take the middle texle, which has an alpha value of 0.5, it will only have 50% coverage. 

So you can understand alpha blending operation as filing the empty coverage with the new texel color, 
which is (1 - src alpha) * dst color;
            
then we add the original population which is alpha * src color             

we add the two, and you get your alpha blending result.

(if non-premultiplied);
                src color * src alpha + (1 - src alpha) * dst color

(if premultiplied);
                src color + (1 - src alpha) * dst color



9:02
pretty much summarises what we did last time

for our ground, we render tons of ground, stones, turfs on to a buffer in initialization.
THen every tick, we just blit our buffer onto the screen

the goal is to produce the alpha value for the intermeddiate buffer.

Like what we said in episode 38, the screen doesnt need alpha. The screen just needs RGB. The alpha isnt used
by windows to do anything. Again, alpha values are used for compositing. Windows only needs to know the color channels





11:10
for rendering any given pixels in the intermediate buffer, we have the 
                source alpha = SA
                destination alpha = DA

at start time, all destination alpha should be 0


11:55
when the 1st bitmap comes in, DA should just equal SA. 

                DA = SA


12:34
when the 2nd bitmap comes in. If a destination pixel hasnt been touched, which (DA will be 0), then we just get SA

if the source alpha is totaly opaque, then my destination alpha should also be totaly opaque.

                if DA == 0
                    DA = SA

                if SA == 1
                    DA = 1

if the source alpha is 0, we want destination alpha to be whatever it was before

                if SA == 0              
                    DA = DA


15:11
So the ultimate goal is to devise a formula for alphas

if you have a half-transparent thing drawn over a half-transparent background. What should happen?

                if SA == 0.5 & DA == 0.5

should  DA == 0.5^2, or 
        DA = 0.75 or 
        DA = 0.5 + 0.5 = 1, 
        or something else?
 




17:10
This is the part where I really dont find Casey explanation to be intuitive.
Sean Barretts explanation is much better

but I will still write down what he says

so if a bitmap is partially covering a pixel, (For example, an edge passes through and only occupies 60% of the pixel)
the alpha value should be 0.6

                 _______________
                |       ########|
                |       ########|
                |       ########|
                |       ########|
                |   ############|
                |___############|

so it is kind of like asking the quesiton, how much area inside the pixel is covered



18:06
you can have one bitmap covered 30% right side of the pixel
                 _______________
                |            ###|
                |            ###|
                |            ###|       
                |            ###|
                |            ###|
                |____________###|


and another bitmap covered 30% left side of the pixel

                 _______________
                |###            |
                |###            |
                |###            |       
                |###            |
                |###            |
                |###____________|


when you put the two together

                 _______________
                |###         ###|
                |###         ###|
                |###         ###|       
                |###         ###|
                |###         ###|
                |###_________###|

the combined should be 0.6





18:26
you can imageine in another scenario below, where the two is almost covering the exact same part

                 _______________
                |            ###|
                |            ###|
                |            ###|       
                |            ###|
                |          #####|
                |__________#####|



                 _______________
                |          #####|
                |          #####|
                |            ###|       
                |            ###|
                |            ###|
                |____________###|



the combined result is probably just 0.35

                 _______________
                |          #####|
                |          #####|
                |            ###|       
                |            ###|
                |          #####|
                |__________#####|




18:59
apparently this is also why you end up problems like "multi sampling" in hardware
where we take many samples per pixel, to find out where the converage actually was 
instead of what we do over here, which is reducing it down to a singer number




19:33
for our case, we assume when two bitmaps draw ontop of a pixel, the coverage is the same
if we use the addition formula, we can easily end up in overflow situations

that is not necessarily bad, we can clamp it to 1 if we want to


ADD and CLAMP



20:47
the other formula we can try to intepret alpha to indicate converage parts that have not been covered


Example:
lets say 0.4 is already been covered. region A represnes the 0.6 area that is not covered
                 _______________
                |          #####|
                |          #####|
                |   A      #####|       
                |          #####|
                |          #####|
                |__________#####|


so when another bitmap comes that is 0.5, then this bitmap will have coverage 0.5 of the remaining 0.6

so our final value is 0.4 + 0.5 * (1-0.4) = 0.7

representing in terms of formula is:

                DA + SA * (1 - DA) 

            =   SA - SA * DA + DA

            =   SA + (1 - SA) * DA


so there is actually two ways to phrase this formula

                DA + SA * (1 - DA) or

                SA + (1 - SA) * DA




23:35
so depends on what "values" you already have, you can pick and choose whichever formula is conveinient for you
for example, we initially have the code below: and as you can see we already have (1 - SA); so it can be more conveinient
to use the SA + (1 - SA) * DA one.

                internal void
                DrawBitmap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
                           real32 RealX, real32 RealY, real32 CAlpha = 1.0f)
                {

                
                    ...
                    ...

                    real32 A = Maximum(DA, 255.0f * SA);
                    real32 R = (1.0f - SA) * DR + SA * SR;
                    real32 G = (1.0f - SA) * DG + SA * SG;
                    real32 B = (1.0f - SA) * DB + SA * SB;

                    *Dest = ...
                }



29:27
so Casey attempted the SA + (1 - SA) * DA formula 
lets see the code below

-   RSA means real source alpha. it is within range of [0 ~ 1]
    CAlpha is 1.0f in this case 


-   the code should be pretty straightforward 
    we first get SA, SR, SG, SB, as well as DA, DR, DG, DB

-   then we composite RGB + A

    for RGB, we essentially using the formula below 

                real32 R = (1.0f - SA) * DR + SA * SR;


-   finally we bit shift RGBA and put them into *Dest.

                internal void
                DrawBitmap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
                           real32 RealX, real32 RealY, real32 CAlpha = 1.0f)
                {
                  
                    ...
                    ...

                    for(int Y = MinY; Y < MaxY; ++Y)
                    {
                        uint32 *Dest = (uint32 *)DestRow;
                        uint32 *Source = (uint32 *)SourceRow;

                        for(int X = MinX; X < MaxX; ++X)
                        {
                            real32 SA = (real32)((*Source >> 24) & 0xFF);
                            real32 RSA = (SA / 255.0f) * CAlpha;            
                            real32 SR = CAlpha*(real32)((*Source >> 16) & 0xFF);
                            real32 SG = CAlpha*(real32)((*Source >> 8) & 0xFF);
                            real32 SB = CAlpha*(real32)((*Source >> 0) & 0xFF);

                            real32 DA = (real32)((*Dest >> 24) & 0xFF);
                            real32 DR = (real32)((*Dest >> 16) & 0xFF);
                            real32 DG = (real32)((*Dest >> 8) & 0xFF);
                            real32 DB = (real32)((*Dest >> 0) & 0xFF);
                            real32 RDA = (DA / 255.0f);
                            
                            real32 InvRSA = (1.0f - RSA);
                            // TODO(casey): Check this for math errors
                            real32 A = InvRSA * DA + SA
                            real32 R = InvRSA * DR + RSA * SR;
                            real32 G = InvRSA * DG + RSA * SG;
                            real32 B = InvRSA * DB + RSA * SB;

                            *Dest = (((uint32)(A + 0.5f) << 24) |
                                     ((uint32)(R + 0.5f) << 16) |
                                     ((uint32)(G + 0.5f) << 8) |
                                     ((uint32)(B + 0.5f) << 0));
                            
                            ++Dest;
                            ++Source;
                        }

                        ...
                        ...

                    }
                }



31:14
Casey starts to check if this alpha compositing formula is "working" or not 
to know if our compositing looks good, we want to make sure there isnt any noticeable "Fringe" effect

Casey points out that there are lots of darkness on the edges. So what Casey does is that he only renders the ground
with ground images that does not have darkness. Since we are curious as to where the darkness came from.




34:14
through a series of debugging, you can see that lots of the black came from the cleared buffer. In Sean Barretts article
it is said that "this occured becuz the developers used non-premmultiplied alpha and set their totally transparent texels to black"

the "totally transparent texel" is our starting ground buffer, which we set it to (0,0,0,0)



35:08
Casey attempted the ADD + CLAMP method. Code below:


                internal void
                DrawBitmap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
                           real32 RealX, real32 RealY, real32 CAlpha = 1.0f)
                {
                  
                    ...
                    ...

                    for(int Y = MinY; Y < MaxY; ++Y)
                    {
                        uint32 *Dest = (uint32 *)DestRow;
                        uint32 *Source = (uint32 *)SourceRow;

                        for(int X = MinX; X < MaxX; ++X)
                        {
                            ...
                            
                            getting SA, RSA, SR, SG, SB
                                    DA, RDA, DR, DG, DB

                            ...

                            real32 InvRSA = (1.0f - RSA);
                            // TODO(casey): Check this for math errors
                            real32 A = DA + SA;
                            if(A > 255.0f)
                            {
                                A = 255.0f;
                            }

                            real32 R = InvRSA * DR + RSA * SR;
                            real32 G = InvRSA * DG + RSA * SG;
                            real32 B = InvRSA * DB + RSA * SB;

                            *Dest = putting RGBA together 
                            
                            ...
                            ...
                        }

                        ...
                        ...

                    }
                }

and Casey found out that you still get the same problem where the cleared buffer blackness is bleeding through.



40:29
Therefore Casey mentions apparently premultiplied alpha is used to solve the darkness bleeding 


apparently premultiplied alpha came as an optimization idea
people start to notice that for most formulas, you are almost always multiple your source color with source alpha.

We almost never use SR, SG, SB themselves. Almost always, we use them with their values after multiplying alphas


                // TODO(casey): Check this for math errors
                real32 A = InvRSA * DA + SA
                real32 R = InvRSA * DR + RSA * SR;
                real32 G = InvRSA * DG + RSA * SG;
                real32 B = InvRSA * DB + RSA * SB;

so why not just store the bitmap premultiplied, so I can save some multiplications 


                // TODO(casey): Check this for math errors
                real32 A = InvRSA * DA + SA
                real32 R = InvRSA * DR + SR;
                real32 G = InvRSA * DG + SG;
                real32 B = InvRSA * DB + SB;





56:23
starting to get Premultiplied alpha working. So when we load our BMP files, we changed it to below:
Notice the key step is these three lines

                R = R*AN;
                G = G*AN;
                B = B*AN;

AN is alpha normalized. So when we load an image and convert it to the bitmap, 
we directly just multiply it with the alpha values

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

                                ... getting the proper RGBA ...
                                real32 R = ... 
                                real32 G = ...
                                real32 B = ...
                                real32 A = ...
                                real32 AN = (A / 255.0f);

                #if 1
                                R = R*AN;
                                G = G*AN;
                                B = B*AN;
                #endif
                                
                                *SourceDest++ = (((uint32)(A + 0.5f) << 24) |
                                                 ((uint32)(R + 0.5f) << 16) |
                                                 ((uint32)(G + 0.5f) << 8) |
                                                 ((uint32)(B + 0.5f) << 0));
                            }
                        }
                    }

                    Result.Pitch = -Result.Width*BITMAP_BYTES_PER_PIXEL;
                    Result.Memory = (uint8 *)Result.Memory - Result.Pitch*(Result.Height - 1);
                    
                    return(Result);
                }



1:03:34
then in our DrawBitmap function(); we change the formula



                real32 InvRSA = (1.0f-RSA);
                // TODO(casey): Check this for math errors
                real32 A = InvRSA*DA + SA;
                real32 R = InvRSA*DR + SR;
                real32 G = InvRSA*DG + SG;
                real32 B = InvRSA*DB + SB;


and at this point we already got rid of the blackness



1:12:13
Casey tries derive the formula for the alpha channel 

                < C0, A0 >      --->    Bitmap0
                < C1, A1 >      --->    Bitmap1
                < Cs,    >      --->    Screen
                < Cb, Ab >      --->    Buffer


                < Cr,    >      --->    Result Screen


The final resulting Screen color we want is effectively
                
                Cr = C1 over C0 over Cs

if you have more images, this keeps on going 
                
                Cr = Cn over Cn-1 ...... C3 over C2 over C1 over C0 over C_screen

recall the color blending formula is 

                Blend (S,D) = src color * src alpha + (1-src alpha) * dest color 





so starting from the formula below: 

                [C1 over C0] over Cs == C1 over [C0 over Cs]

he derives the alpha blending formula:

                Ab = A0 + A1 - A0 * A1

Personally I felt this episode is a bit misleading. Only until I read Sean Barretts article till I know what is going on.


look for Sean Barretts article 
http://twvideo01.ubm-us.net/o1/vault/GD_Mag_Archives/Game.Developer.2004.11.pdf

https://nothings.org/gamedev/blend_equations_with_destination_alpha.txt




1:25:09
so our code looks like 

                real32 InvRSA = (1.0f-RSA);
                // TODO(casey): Check this for math errors
                real32 A = 255.0f*(RSA + RDA - RSA*RDA);
                real32 R = InvRSA*DR + SR;
                real32 G = InvRSA*DG + SG;
                real32 B = InvRSA*DB + SB;









1:37:06
Sean Barretts says "Non Premultiplied Alpha does not distribute over Lerp (Linear inteprolation)"

what he means is that if you want stack linear interpolation on top of each other, 

for example, if you are doing nested blending.

                Blend( Blend(p0, p1), Blend(p2, p3) );

if you do not pre-multiply alpha, the blend does not distribute. Meaning you cant compose the blend in arbiturary ways 
unless the alpha is pre-multiplied






1:39:36
for a long time, the game industry did not know about premultiplied alpha. Which is why in many old games, you see black fringes
on sprites. 

once bilinear filtering textures came out,


bilinear filtering is actually a blend operation

            Blend( Blend(A, B), Blend(C, D) );

               A _______________ B
                |               |
                |               |
                |           o   |       
                |               |
                |               |
                |_______________|
               C                  D


then that gets blended to the frameBuffer. So you have this series of blending going on with texture filtering


1:42:14
so you really carea bout the math, you can not do the premultiplied alpha and re-divide everything or what.
but eventually you will work with GPU/hardware. and the hardware might have fixed notions of using premultiplied alpha

maybe you can just turnoff the texture-filtering hardware entirely, and that might work with regular alpha



