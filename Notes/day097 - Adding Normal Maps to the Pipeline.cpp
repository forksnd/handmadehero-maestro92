Handmade Hero Day 097 - Adding Normal Maps to the Pipeline


Summary:
Discussed his environment_map scheme for this 2D game.

added environment_map and normal_map into the DrawRectangleSlowly(); function.
nothing is actually working. He will finish everything up in the next episode.



Keyword:
rendering, environment_map, normal_map





14:24
we have a 0 ~ 1 value for our surface
0 being totaly Dull
1 being very reflective, super shiny



15:11
if we have a normal direction pointing downwards, we can imagine sampling values from the ground chunks
and if you have a normal direction pointing upwards, we can imagine sampling values from the sky chunks
and stuff pointing side ways is looking at its surroundings

so Casey is suggesting a 3-plate scheme?

-	Above
-	Below
-	Side (we will ignore this for now)


17:26 
proposes a special type of encoding in the normal map
assuming we have x, y, z values in our normal map 

will be using z to indicate which plate to sample from

if z = 1, we sample from the above plate 
if z = -1, sample from the Below plate 
if z = 0.5, sample from the middle plate	
if z is somewhere in the middle, we get a blend of the two plates (above ~ middle, or middle ~ below);

0 to 255 maps to -1 to 1








				256 x 256		1
				128 x 128		0.66
				 64 x 64		0.33
				 32 x 32		0



25:45
starting to implement this. 
Again, we are only testing the normal functionality in the "render_entry_coordinate_system" draw call 

so first, we added the concep of a normal map in the render_entry

				handmade_render_group.h

				struct render_entry_coordinate_system
				{
				    v2 Origin;
				    v2 XAxis;
				    v2 YAxis;
				    v4 Color;
				    loaded_bitmap *Texture;
				    loaded_bitmap *NormalMap;

				    environment_map *Top;
				    environment_map *Middle;
				    environment_map *Bottom;
				};





31:51
then in the acutal function that we render, make the changes 
notice we added an if statement for the "NormalMap". Essentially is is the same thing of rendering regular pixel colors 

-	we get the four corresponding pixels from the normal map, we do a bilinear interpolation

-	after we got our interpolated normal, we use it to do a lighting look up from the environment_map (more on this later);
	the reason why we do not use "loaded_bitmap" for the Top, Middle and Bottom is becuz we may want different resolutions of the 
	environment_map to fake the blurriness

-	Casey did warn us that this brute force implementation is incredibly slow 

-	so once we have our normal, the idea is to pick to normal maps. We already know one of the environment_map is going to be 
	Middle, so the question left is to choose either Top or Bottom depending on Normal.z

				tFarMap = 1.0f - (tEnvMap / 0.25f);
	that is just trying to produce a value between 0 ~ 1, we use tFarMap to blend between Middle ~ Top, or Bottom ~ Middle


				internal void
				DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
				                    loaded_bitmap *Texture, loaded_bitmap *NormalMap,
				                    environment_map *Top,
				                    environment_map *Middle,
				                    environment_map *Bottom)
				{
					...
					...


	                v4 TexelA = Unpack4x8(TexelPtrA);
	                v4 TexelB = Unpack4x8(TexelPtrB);
	                v4 TexelC = Unpack4x8(TexelPtrC);
	                v4 TexelD = Unpack4x8(TexelPtrD);

	                ...
	                ...
	                doing bilinear interpolation on these four texels 
	                ...
	                ...


	                if(NormalMap)
	                {
	                    uint8 *NormalPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
	                    uint32 NormalPtrA = *(uint32 *)(NormalPtr);
	                    uint32 NormalPtrB = *(uint32 *)(NormalPtr + sizeof(uint32));
	                    uint32 NormalPtrC = *(uint32 *)(NormalPtr + Texture->Pitch);
	                    uint32 NormalPtrD = *(uint32 *)(NormalPtr + Texture->Pitch + sizeof(uint32));

	                    v4 NormalA = Unpack4x8(NormalPtrA);
	                    v4 NormalB = Unpack4x8(NormalPtrB);
	                    v4 NormalC = Unpack4x8(NormalPtrC);
	                    v4 NormalD = Unpack4x8(NormalPtrD);

	                    v4 Normal = Lerp(Lerp(NormalA, fX, NormalB),
	                                     fY,
	                                     Lerp(NormalC, fX, NormalD));

	                    environment_map *FarMap = 0;
	                    real32 tEnvMap = Normal.z;
	                    real32 tFarMap = 0.0f;
	                    if(tEnvMap < 0.25f)
	                    {
	                        FarMap = Bottom;
	                        tFarMap = 1.0f - (tEnvMap / 0.25f);
	                    }
	                    else if(tEnvMap > 0.75f)
	                    {
	                        FarMap = Top;
	                        tFarMap = (1.0f - tEnvMap) / 0.25f;
	                    }

	                    v3 LightColor = SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, Middle);
	                    if(FarMap)
	                    {
	                        v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, Middle);
	                        LightColor = Lerp(LightColor, tFarMap, FarMapColor);
	                    }
	                
	                    Texel.rgb = Hadamard(Texel.rgb, LightColor);                    
	                }
	                
	                ...
	                ...

	                // NOTE(casey): Go from "linear" brightness space to sRGB
	                v4 Blended255 = Linear1ToSRGB255(Blended);

	                *Pixel = (((uint32)(Blended255.a + 0.5f) << 24) |
	                          ((uint32)(Blended255.r + 0.5f) << 16) |
	                          ((uint32)(Blended255.g + 0.5f) << 8) |
	                          ((uint32)(Blended255.b + 0.5f) << 0));


					...
					...

				}








32:26
notice that we mention that our normal maps mostly store x, y, z but when we get the pixels from the normal map,
we are still calling Unpack4x8(NormalPtrA){};

                uint8 *NormalPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
                uint32 NormalPtrA = *(uint32 *)(NormalPtr);
                uint32 NormalPtrB = *(uint32 *)(NormalPtr + sizeof(uint32));
                uint32 NormalPtrC = *(uint32 *)(NormalPtr + Texture->Pitch);
                uint32 NormalPtrD = *(uint32 *)(NormalPtr + Texture->Pitch + sizeof(uint32));

                v4 NormalA = Unpack4x8(NormalPtrA);
                v4 NormalB = Unpack4x8(NormalPtrB);
                v4 NormalC = Unpack4x8(NormalPtrC);
                v4 NormalD = Unpack4x8(NormalPtrD);

so what ist he fourth channel for?

Casey mentioned, we tend to bake the "Description of the surface" into the fourth channel

in our case, we store the "glossiness" of the surface is 




34:27
we added the environment_map class. 
like what we mentioned above, this has multiple blurry levels
-	LOD means levels of details
-	notice we encode the width and height. We go down from level 0

	LOD[0] =  2^WidthPow2 x 2^HeightPow2
	LOD[1] =  2^(WidthPow2-1) x 2^(HeightPow2-1)
	LOD[2] =  2^(WidthPow2-2) x 2^(HeightPow2-2)
	...
	...
				handmade_render_group.h

				struct environment_map
				{
				    // NOTE(casey): LOD[0] is 2^WidthPow2 x 2^HeightPow2
				    uint32 WidthPow2;
				    uint32 HeightPow2;
				    loaded_bitmap *LOD[4];
				};




44:26

so notice in the call, we have to pass in ScreenSpaceUV to the SampleEnvironmentMap(); function
that is cuz we need to know where to sample the envirnoment_map from


				v3 LightColor = SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, Middle);

				
-	at the end, we modify the pixel color by doing 

			    Texel.rgb = Hadamard(Texel.rgb, LightColor);       

	we tinit the texel color by the light contribution



				internal void
				DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
				                    loaded_bitmap *Texture, loaded_bitmap *NormalMap,
				                    environment_map *Top,
				                    environment_map *Middle,
				                    environment_map *Bottom)
				{



	                v2 ScreenSpaceUV = {InvWidthMax*(real32)X, InvHeightMax*(real32)Y};
	                
	                real32 U = InvXAxisLengthSq*Inner(d, XAxis);
	                real32 V = InvYAxisLengthSq*Inner(d, YAxis);

					...
					...

	                if(NormalMap)
	                {
	                    uint8 *NormalPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
	                    uint32 NormalPtrA = *(uint32 *)(NormalPtr);
	                    uint32 NormalPtrB = *(uint32 *)(NormalPtr + sizeof(uint32));
	                    uint32 NormalPtrC = *(uint32 *)(NormalPtr + Texture->Pitch);
	                    uint32 NormalPtrD = *(uint32 *)(NormalPtr + Texture->Pitch + sizeof(uint32));

	                    v4 NormalA = Unpack4x8(NormalPtrA);
	                    v4 NormalB = Unpack4x8(NormalPtrB);
	                    v4 NormalC = Unpack4x8(NormalPtrC);
	                    v4 NormalD = Unpack4x8(NormalPtrD);

	                    v4 Normal = Lerp(Lerp(NormalA, fX, NormalB),
	                                     fY,
	                                     Lerp(NormalC, fX, NormalD));

	                    environment_map *FarMap = 0;
	                    real32 tEnvMap = Normal.z;
	                    real32 tFarMap = 0.0f;
	                    if(tEnvMap < 0.25f)
	                    {
	                        FarMap = Bottom;
	                        tFarMap = 1.0f - (tEnvMap / 0.25f);
	                    }
	                    else if(tEnvMap > 0.75f)
	                    {
	                        FarMap = Top;
	                        tFarMap = (1.0f - tEnvMap) / 0.25f;
	                    }

	                    v3 LightColor = SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, Middle);
	                    if(FarMap)
	                    {
	                        v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, FarMap);
	                        LightColor = Lerp(LightColor, tFarMap, FarMapColor);
	                    }
	                
	                    Texel.rgb = Hadamard(Texel.rgb, LightColor);                    
	                }

	                ...
	                ...
				}






48:44

for now, the we are just doing this for now 


				inline v3
				SampleEnvironmentMap(v2 ScreenSpaceUV, v3 Normal, real32 Roughness, environment_map *Map)
				{
				    v3 Result = Normal;
				    
				    return(Result);
				}


50:14

added the xyz to the vector4

				union v4
				{
				    struct
				    {
				        union
				        {
				            v3 xyz;
				            struct
				            {
				                real32 x, y, z;
				            };
				        };
				        
				        real32 w;        
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


51:38
you will find that when you into more complicated rendering stuff, typically the hard part is not writing the equations,
its the plumbing, its making sure everything moves through the rendering pipeline properly.



54:30
Casey will programmatically generate a sphere normal map for the tree.


-	we first compute BitmapUV. That is in [0 ~ 1]

-	then we get normal, 
				v3 Normal = {2.0f*BitmapUV.x - 1.0f, 2.0f*BitmapUV.y - 1.0f, 0.0f};
	which is in 
				x = [-1 ~ 1]
				y = [-1 ~ 1]
				z = [0]

	
	for those who are not familar with the math:

	2.0f*BitmapUV.x makes it so that its [0 ~ 2]
	-1 makes it so that it is [-1 ~ 1]			


-	then we have to put the Color values back in the range of [0 ~ 255]

	            v4 Color = {255.0f*(0.5f*(Normal.x + 1.0f)),
	                        255.0f*(0.5f*(Normal.y + 1.0f)),
	                        127.0f*Normal.z,
	                        255.0f*Roughness};

	essentially he just reversed the what he just did


				
				handmade.cpp

				internal void
				MakeSphereNormalMap(loaded_bitmap *Bitmap, real32 Roughness)
				{
				    real32 InvWidth = 1.0f / (1.0f - Bitmap->Width);
				    real32 InvHeight = 1.0f / (1.0f - Bitmap->Height);
				    
				    uint8 *Row = (uint8 *)Bitmap->Memory;
				    for(int32 Y = 0; Y < Bitmap->Height; ++Y)
				    {
				        uint32 *Pixel = (uint32 *)Row;
				        for(int32 X = 0; X < Bitmap->Width; ++X)
				        {
				            v2 BitmapUV = {InvWidth*(real32)X, InvHeight*(real32)Y};

				            // TODO(casey): Actually generate sphere!!
				            v3 Normal = {2.0f*BitmapUV.x - 1.0f, 2.0f*BitmapUV.y - 1.0f, 0.0f};
				            Normal.z = SquareRoot(1.0f - Minimum(1.0f, Square(Normal.x) + Square(Normal.y)));

				            v4 Color = {255.0f*(0.5f*(Normal.x + 1.0f)),
				                        255.0f*(0.5f*(Normal.y + 1.0f)),
				                        127.0f*Normal.z,
				                        255.0f*Roughness};

				            *Pixel = (((uint32)(Color.a + 0.5f) << 24) |
				                      ((uint32)(Color.r + 0.5f) << 16) |
				                      ((uint32)(Color.g + 0.5f) << 8) |
				                      ((uint32)(Color.b + 0.5f) << 0));
				        }

				        Row += Bitmap->Pitch;
				    }
				}



1:10:15
	

initially we have 


	            v3 Normal = {2.0f*BitmapUV.x - 1.0f, 2.0f*BitmapUV.y - 1.0f, 0.0f};
	            Normal = Normalize(Normal);

with 

				inline v3
				Normalize(v3 A)
				{
				    v3 Result = A * (1.0f / Length(A));

				    return(Result);
				}



some guy from the Q/A said 

	            v3 Normal = {2.0f*BitmapUV.x - 1.0f, 2.0f*BitmapUV.y - 1.0f, 0.0f};
	            Normal.z = SquareRoot(1.0f - Minimum(1.0f, Square(Normal.x) + Square(Normal.y)));
