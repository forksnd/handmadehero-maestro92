Handmade Hero Day 100 - Reflection Vectors

Summary:

discussed how the light reflection model works 
discussed the eye vector for 2d Orthographic views

wrote the proper SampleEnvironmentMap(); with the reflection vector 

lots of debugging after that

wrote the MakeNormalMapForPyramids(); as another example

demonstrated that currenlty the normals are incorrect when the object is rotated

Keyword:
rendering, Environment Map, reflection vector



5:15
graphical explanation of our light reflection model

spent a bunch of time explaning how to calculate the reflect vector when an incoming vector 
is hitting a surface
essentially you have an eye vector, which bounces into the environment_map. Pretty much just like 
cube mapping

							  ^	
							  |
					env_map   |	    eye
					   	 ^	  |    
                          \   |   /
						   \  |  /
 							\ | /
				_____________\|/_____________________

12:21
added this light reflection model to the rendering code 
-	we first needed to figure out an eye vector. Since we are a 2D game, this is pretty tricky
2D games dont really have a rigorous definition for it. 2D games dont typically end up being actually
perspective. They are in Orthographic. so the vector to the eye is pointing straight out of the Screen.

so in this case, if your camera moves, you wont see reflection on the object change. But if the object themselves 
change, the reflection changes


				
16:01
intially he defined the EyeVector to be 

				v3 EyeVector = {0, 0, 1};

this means our that only z axis have values, and we can do some premature "optimizations"

the equation is just like what we did in collision detection

			reflection vector =	[-e + 2 dot(e, unit normal) * N]

							N

							  ^	
							  |
					    r	  |	   	eye
					   	 ^	  |    /
                          \   |   /
						   \  |  / e
 							\ | /
				_____________\|/_____________________
							 /	
							/
						   / e
						  /
						 v

recall that since we are doing dot(e, unit_normal); and our eye vector is {0, 0, 1} then we know its just gonna be the z 
coordiante of that thing 

19:07
so for this reason, we simplified our reflection vector calculation to be 

                v3 BounceDirection = 2.0f*Normal.z*Normal.xyz;
                BounceDirection.z -= 1.0f;

matching the equation 
				reflection vector =	[-e + 2 dot(e, unit normal) * N];


-	also note that we are making the change of using the BounceDirection instead of Normal

                real32 tEnvMap = BounceDirection.y;				


-	notice in the if(tEnvMap < -0.5f) case, we reversed the BounceDirection 

				BounceDirection.y = -BounceDirection.y;

	this is just the calling convention of BounceDirection + environment_map that Casey made in this case.
	he wants BounceDirection.y to always be positive. So even if you sampling from the ground plane, 
	it is as if you are sampling the environment_map on the sky. This is just entirely his calling convention here.


-	notice now we pass the BounceDirection to the SampleEnvironmentMap(); function

	            v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, BounceDirection, Normal.w, FarMap);


-	now the code looks like below 

                if(NormalMap)
                {
                    bilinear_sample NormalSample = BilinearSample(NormalMap, X, Y);

                    ...
                    ...


                    // TODO(casey): Rotate normals based on X/Y axis!
                    
                    // NOTE(casey): The eye vector is always assumed to be [0, 0, 1]
                    // This is just the simplified version of the reflection -e + 2e^T N N
                    v3 BounceDirection = 2.0f*Normal.z*Normal.xyz;
                    BounceDirection.z -= 1.0f;

                    environment_map *FarMap = 0;
                    real32 tEnvMap = BounceDirection.y;
                    real32 tFarMap = 0.0f;
                    if(tEnvMap < -0.5f)
                    {
                        FarMap = Bottom;
                        tFarMap = -1.0f - 2.0f*tEnvMap;
                        BounceDirection.y = -BounceDirection.y;
                    }
                    else if(tEnvMap > 0.5f)
                    {
                        FarMap = Top;
                        tFarMap = 2.0f*(tEnvMap - 0.5f);
                    }

                    v3 LightColor = {0, 0, 0}; // TODO(casey): How do we sample from the middle map???
                    if(FarMap)
                    {
                        v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, BounceDirection, Normal.w, FarMap);
                        LightColor = Lerp(LightColor, tFarMap, FarMapColor);
                    }

                    // TODO(casey): ? Actually do a lighting model computation here
                
                    Texel.rgb = Texel.rgb + Texel.a*LightColor;
                }





25:18
as mentioned, the way we want to use our environment_map is 


				top           dx
				________________________________
							|	  ^					
							|	 /					
 							|   /					 
					      z	|  /  d
							| /
				____________|/___________________ 
				
but the thing is, we do not know the distance z between our object and the sky (top plane);

we have the direction of d (our reflection vector);, and if we know z, we get get our d. 

the factor is essentially z / d.y  ( we are assuming y is along the z axis here)

only d.y is aligned with z 



33:30

now we revisit the SampleEnvironmentMap(); again
				
-	we added the assert 

			    Assert(SampleDirection.y > 0.0f);
	
	this is becuz we already made the SampleDirection.y positive outside of this function


-	the core math is in these few lines. Should be pretty straight forward

			    real32 C = (UVsPerMeter*DistanceFromMapInZ) / SampleDirection.y;
			    // TODO(casey): Make sure we know what direction Z should go in Y
			    v2 Offset = C * V2(SampleDirection.x, SampleDirection.z);
			    v2 UV = ScreenSpaceUV + Offset;



				DistanceFromMapInZ / SampleDirection.y
	is how many steps the SampleDirection has to take to reach the top.

	with that ratio, we calculate the x,z offset.


-	note that we also clamp our UV, cuz we could be sampling outside of our texture map

			    UV.x = Clamp01(UV.x);
			    UV.y = Clamp01(UV.y);


-	below we have the full code 

				handmade_render_group.cpp

				inline v3
				SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, real32 Roughness, environment_map *Map)
				{
				    uint32 LODIndex = (uint32)(Roughness*(real32)(ArrayCount(Map->LOD) - 1) + 0.5f);
				    Assert(LODIndex < ArrayCount(Map->LOD));

				    loaded_bitmap *LOD = &Map->LOD[LODIndex];

				    Assert(SampleDirection.y > 0.0f);
				    real32 DistanceFromMapInZ = 1.0f;
				    real32 UVsPerMeter = 0.01f;
				    real32 C = (UVsPerMeter*DistanceFromMapInZ) / SampleDirection.y;
				    // TODO(casey): Make sure we know what direction Z should go in Y
				    v2 Offset = C * V2(SampleDirection.x, SampleDirection.z);
				    v2 UV = ScreenSpaceUV + Offset;

				    UV.x = Clamp01(UV.x);
				    UV.y = Clamp01(UV.y);
				    
				    // TODO(casey): Formalize texture boundaries!!!
				    real32 tX = ((UV.x*(real32)(LOD->Width - 2)));
				    real32 tY = ((UV.y*(real32)(LOD->Height - 2)));
				    
				    int32 X = (int32)tX;
				    int32 Y = (int32)tY;

				    real32 fX = tX - (real32)X;
				    real32 fY = tY - (real32)Y;

				    Assert((X >= 0) && (X < LOD->Width));
				    Assert((Y >= 0) && (Y < LOD->Height));

				    bilinear_sample Sample = BilinearSample(LOD, X, Y);
				    v3 Result = SRGBBilinearBlend(Sample, fX, fY).xyz;

				    return(Result);
				}


39:47
we see the results of the first draft on screen



56:08
ppl in Q/A Suggested writing a pyramid Normal map, that is perhaps easier than the Sphere

gives a graphical explanation of the code. It just splitting the rectangle into four parts 

-	the part here are just determining which of the four parts are you on 

	            if(X < Y)
	            {
	                if(InvX < Y)
	                {
	                    Normal.x = -Seven;
	                }
	                else
	                {
	                    Normal.y = Seven;
	                }
	            }
	            else
	            {
	                if(InvX < Y)
	                {
	                    Normal.y = -Seven;
	                }
	                else
	                {
	                    Normal.x = Seven;
	                }
	            }

-	do note that this againta  front facing sprite, so everyone has the z component pointing outwards

	            real32 Seven = 0.707106781188f;
	            v3 Normal = {0, 0, Seven};

-	full code below 

				handmade.cpp

				internal void
				MakePyramidNormalMap(loaded_bitmap *Bitmap, real32 Roughness)
				{
				    real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
				    real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);
				    
				    uint8 *Row = (uint8 *)Bitmap->Memory;
				    for(int32 Y = 0; Y < Bitmap->Height; ++Y)
				    {
				        uint32 *Pixel = (uint32 *)Row;
				        for(int32 X = 0; X < Bitmap->Width; ++X)
				        {
				            v2 BitmapUV = {InvWidth*(real32)X, InvHeight*(real32)Y};

				            int32 InvX = (Bitmap->Width - 1) - X;
				            real32 Seven = 0.707106781188f;
				            v3 Normal = {0, 0, Seven};
				            if(X < Y)
				            {
				                if(InvX < Y)
				                {
				                    Normal.x = -Seven;
				                }
				                else
				                {
				                    Normal.y = Seven;
				                }
				            }
				            else
				            {
				                if(InvX < Y)
				                {
				                    Normal.y = -Seven;
				                }
				                else
				                {
				                    Normal.x = Seven;
				                }
				            }

				            v4 Color = {255.0f*(0.5f*(Normal.x + 1.0f)),
				                        255.0f*(0.5f*(Normal.y + 1.0f)),
				                        255.0f*(0.5f*(Normal.z + 1.0f)),
				                        255.0f*Roughness};

				            *Pixel++ = (((uint32)(Color.a + 0.5f) << 24) |
				                        ((uint32)(Color.r + 0.5f) << 16) |
				                        ((uint32)(Color.g + 0.5f) << 8) |
				                        ((uint32)(Color.b + 0.5f) << 0));
				        }

				        Row += Bitmap->Pitch;
				    }
				}



1:11:03
shows that the normals are currently incorrect when object is rotated.
so we need to rotate the normals as well.