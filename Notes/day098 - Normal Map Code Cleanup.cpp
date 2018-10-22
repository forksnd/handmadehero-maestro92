Handmade Hero Day 098 - Normal Map Code Cleanup

Summary:
follow up on Day 097

Cleaned up the normal map code. Fixed a bunch of errors from day 097.
Revisited and rewrote some of the functions
we dont actually generate the environment_map in this episode.

mentioned there will be two types of normal maps in our game: "front facing" and "up facing"

wrote proper code for the SampleEnvironmentMap(); function

mentioned normal map vs bump map

Keyword:
normal map, rendering, bump map





5:04
we wrote the MakeSphereNormalMap last time, we will now hook up this function
as we are currently rendering only the one tree, we will test the normal map functionality on this tree.
we first added a TreeNormal map for the game 

				struct game_state
				{
					...
					...

				    loaded_bitmap Tree;
				    loaded_bitmap TreeNormal;

				    ...
				    ...
				};




5:52
we allocated an emptyBitmap for the normal, and we populated with it by call the MakeSphereNormalMap(); function 
not entirely sure why he allocated this in the transient memory, but i guess it doesnt really matter.

			    transient_state *TranState = (transient_state *)Memory->TransientStorage;
			    if(!TranState->IsInitialized)
			    {
			    	...
			    	...

			        GameState->TreeNormal = MakeEmptyBitmap(&TranState->TranArena, GameState->Tree.Width, GameState->Tree.Height, false);
			        MakeSphereNormalMap(&GameState->TreeNormal, 0.0f);

			        TranState->IsInitialized = true;
			    }




11:34
mentions how x,y,z in world maps to x,y,z in texture space for a tree

				x,	y,	z
				
				|	|	|
				|	|	|
				v	v	v
				
				x,	z,	y

the x channel in our normal is the x axis of our world.
the y channel in our normal is more like the z axis, where you use it to decide how to blend between
Top, Middle or bottom plates 





for example, if you have a tree as a "front facing" sprite, you dont really have normals pointing backwards

assuming you have the following axis system

				y
				^
				|
				|
				|
				|
				------------> x
			   /
			  /
			 /
			<
		   z

for a "front facing" sprite such as a tree, you will never have a surface that is pointing in the negative -z direction 
that is on the "front face" of our sprite

for a ground, it might be different, bumpbiness on the ground could actually point to all different directions.


so you actually have two types of normal maps 

1.	you have the "front facing" normal map, which has the irregular mapping of the x,y,z axis

				x,	y,	z
				
				|	|	|
				|	|	|
				v	v	v
				
				x,	z,	y


2.	the "up facing" normal map, where the x, y, z means what they mean. 

so we would need 2 texture sampling paths 



14:32
so now we have make the following changes in our way of reading the normal map 
-	after bilinearly lerping the normals, we first call UnscaleAndBiasNormal(); 
	recall in the MakeSphereNormalMap(); our normal map is in [0 ~ 255] range, the UnscaleAndBiasNormal();
	makes it in [-1 ~ 1] range 


- 	Normalize(Normal.xyz); that just makes everything unit length



				internal void
				DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
				                    loaded_bitmap *Texture, loaded_bitmap *NormalMap,
				                    environment_map *Top,
				                    environment_map *Middle,
				                    environment_map *Bottom)
				{

					...
					...

				    for(int Y = YMin; Y <= YMax; ++Y)
				    {
				        for(int X = XMin; X <= XMax; ++X)
				        {
	
							...
							...				                

			                if(NormalMap)
			                {
			                    bilinear_sample NormalSample = BilinearSample(NormalMap, X, Y);

			                    v4 NormalA = Unpack4x8(NormalSample.A);
			                    v4 NormalB = Unpack4x8(NormalSample.B);
			                    v4 NormalC = Unpack4x8(NormalSample.C);
			                    v4 NormalD = Unpack4x8(NormalSample.D);

			                    v4 Normal = Lerp(Lerp(NormalA, fX, NormalB),
			                                     fY,
			                                     Lerp(NormalC, fX, NormalD));

			                    Normal = UnscaleAndBiasNormal(Normal);
			                    // TODO(casey): Do we really need to do this?
			                    Normal.xyz = Normalize(Normal.xyz);

			                    // TODO(casey): ? Actually compute a bounce based on the viewer direction

			                    environment_map *FarMap = 0;
			                    real32 tEnvMap = Normal.y;
			                    real32 tFarMap = 0.0f;
			                    if(tEnvMap < -0.5f)
			                    {
			                        FarMap = Bottom;
			                        tFarMap = 2.0f*(tEnvMap + 1.0f);
			                    }
			                    else if(tEnvMap > 0.5f)
			                    {
			                        FarMap = Top;
			                        tFarMap = 2.0f*(tEnvMap - 0.5f);
			                    }

			                    v3 LightColor = {0, 0, 0}; // SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, Middle);
			                    if(FarMap)
			                    {
			                        v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, FarMap);
			                        LightColor = Lerp(LightColor, tFarMap, FarMapColor);
			                    }

			                    // TODO(casey): ? Actually do a lighting model computation here
			                
			                    Texel.rgb = Texel.rgb + Texel.a*LightColor;
			                }
			                ...
			                ...
			                ...

			                // NOTE(casey): Go from "linear" brightness space to sRGB
			                v4 Blended255 = Linear1ToSRGB255(Blended);

			                *Pixel = (((uint32)(Blended255.a + 0.5f) << 24) |
			                          ((uint32)(Blended255.r + 0.5f) << 16) |
			                          ((uint32)(Blended255.g + 0.5f) << 8) |
			                          ((uint32)(Blended255.b + 0.5f) << 0));
				            }

				            
				            ++Pixel;
				        }
				        
				        Row += Buffer->Pitch;
				    }
				}



15:14
we write the UnscaleAndBiasNormal function 
our Normal is initially in [0 ~ 255]. We change it to [-1 ~ 1]

-	the following first makes it in [0 ~ 1] range 
				real32 Inv255 = 1.0f / 255.0f;

-	then multiplying it by 2 makes in [0 ~ 2] range 

				2.0f*(Inv255*Normal.x);

-	then the -1 makes in in [-1 ~ 1] range 

-	Normal.w, which is the roughness, that just needs to be in [0 ~ 1]


				inline v4
				UnscaleAndBiasNormal(v4 Normal)
				{
				    v4 Result;

				    real32 Inv255 = 1.0f / 255.0f;

				    Result.x = -1.0f + 2.0f*(Inv255*Normal.x);
				    Result.y = -1.0f + 2.0f*(Inv255*Normal.y);
				    Result.z = -1.0f + 2.0f*(Inv255*Normal.z);

				    Result.w = Inv255*Normal.w;

				    return(Result);
				}





17:53 
39:50
so after all of that, we look at how we sample the environment map


-	we change from 

                real32 tEnvMap = Normal.z;			                    
    to                
                real32 tEnvMap = Normal.y;

    as we previously mentioned 


-	note we are currently doing addition for the texel color. Do note that we are multiplying 
the LightColor with Texel alpha, since we are doing premultipled alpha

                // TODO(casey): ? Actually do a lighting model computation here    
                Texel.rgb = Texel.rgb + Texel.a*LightColor;
    
    Casey does plan to change a lighting model. 




			                if(NormalMap)
			                {
			                    bilinear_sample NormalSample = BilinearSample(NormalMap, X, Y);

			                    v4 NormalA = Unpack4x8(NormalSample.A);
			                    v4 NormalB = Unpack4x8(NormalSample.B);
			                    v4 NormalC = Unpack4x8(NormalSample.C);
			                    v4 NormalD = Unpack4x8(NormalSample.D);

			                    v4 Normal = Lerp(Lerp(NormalA, fX, NormalB),
			                                     fY,
			                                     Lerp(NormalC, fX, NormalD));

			                    Normal = UnscaleAndBiasNormal(Normal);
			                    // TODO(casey): Do we really need to do this?
			                    Normal.xyz = Normalize(Normal.xyz);

			                    // TODO(casey): ? Actually compute a bounce based on the viewer direction

			                    environment_map *FarMap = 0;
			                    real32 tEnvMap = Normal.y;
			                    real32 tFarMap = 0.0f;
			                    if(tEnvMap < -0.5f)
			                    {
			                        FarMap = Bottom;
			                        tFarMap = 2.0f*(tEnvMap + 1.0f);
			                    }
			                    else if(tEnvMap > 0.5f)
			                    {
			                        FarMap = Top;
			                        tFarMap = 2.0f*(tEnvMap - 0.5f);
			                    }

			                    v3 LightColor = {0, 0, 0}; // SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, Middle);
			                    if(FarMap)
			                    {
			                        v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, FarMap);
			                        LightColor = Lerp(LightColor, tFarMap, FarMapColor);
			                    }

			                    // TODO(casey): ? Actually do a lighting model computation here
			                
			                    Texel.rgb = Texel.rgb + Texel.a*LightColor;
			                }





22:58

we revisit the MakeSphereNormalMap(); function

we rewrote the formula for calculating Nz. Essentially we are using the formula for unit sphere 

				1 = x^2 + y^2 + z^2
				z = SquareRoot(1 - x^2 - y^2);

full code below. Of course, we only do this if RootTerm is greater than 0. This is becuz
we have to account for the fact that we may be off the sphere 
	
				 ______________
                |	  ####	   |
                |   ########   |
                | ############ |
                |##############|
                | ############ |
                |   ########   |
                |_____####_____|

for example, in this texture of ours, the white spaces are off the sphere. So we have to account for texels there
of course we have to fill up the values there. we can just have them point straight up, (0,0,1), or assign it 
with the closest value on the sphere


				internal void
				MakeSphereNormalMap(loaded_bitmap *Bitmap, real32 Roughness)
				{
				    real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
				    real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);
				    
				    uint8 *Row = (uint8 *)Bitmap->Memory;
				    for(int32 Y = 0;
				        Y < Bitmap->Height;
				        ++Y)
				    {
				        uint32 *Pixel = (uint32 *)Row;
				        for(int32 X = 0;
				            X < Bitmap->Width;
				            ++X)
				        {
				            v2 BitmapUV = {InvWidth*(real32)X, InvHeight*(real32)Y};

				            real32 Nx = 2.0f*BitmapUV.x - 1.0f;
				            real32 Ny = 2.0f*BitmapUV.y - 1.0f;

				            real32 RootTerm = 1.0f - Nx*Nx - Ny*Ny;
				            v3 Normal = {0, 0, 1};
				            real32 Nz = 0.0f;
				            if(RootTerm >= 0.0f)
				            {
				                Nz = SquareRoot(RootTerm);
				                Normal = V3(Nx, Ny, Nz);
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





37:25
added a struct that helps us with bilinear lerp calls

				handmade_render_grou.cpp 

				struct bilinear_sample
				{
				    uint32 A, B, C, D;
				};




46:36
shows a graph of how do we plan to do use the normal to sample from the environment_map


				v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, FarMap);


as we said, we will use the normal to decide which plane we are smapling from 
in our code we had threshold values of 0.5 

                environment_map *FarMap = 0;
                real32 tEnvMap = Normal.y;
                real32 tFarMap = 0.0f;
                if(tEnvMap < -0.5f)
                {
                    FarMap = Bottom;
                    tFarMap = 2.0f*(tEnvMap + 1.0f);
                }
                else if(tEnvMap > 0.5f)
                {
                    FarMap = Top;
                    tFarMap = 2.0f*(tEnvMap - 0.5f);
                }

                v3 LightColor = {0, 0, 0}; // SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, Middle);
                if(FarMap)
                {
                    v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, Normal.xyz, Normal.w, FarMap);
                    LightColor = Lerp(LightColor, tFarMap, FarMapColor);
                }

you can image two cones, any normal pointing inside the cone, we are chosing the top or bottom
otherwise we can too lateral and we have to do something else. 
the 0.5 threshold value pretty much is the size of the cone 

				--------------------------------------- 	top
							 \       /
						      \     /
							   \   /
								\ /
				----------------------------------------	middle 
								/ \
							   /   \
							  /     \
							 /       \
				----------------------------------------	bottom



49:28
we revisit the SampleEnvironmentMap(); function

-	note that we are doing a hack where we are using the Roughness value to pick the Map->LOD

-	the tX, X, fX variables are just setting up for the bilinear interpolation

				inline v3
				SampleEnvironmentMap(v2 ScreenSpaceUV, v3 Normal, real32 Roughness, environment_map *Map)
				{
				    uint32 LODIndex = (uint32)(Roughness*(real32)(ArrayCount(Map->LOD) - 1) + 0.5f);
				    Assert(LODIndex < ArrayCount(Map->LOD));

				    loaded_bitmap *LOD = Map->LOD[LODIndex];

				    // TODO(casey): Do intersection math to determine where we should be!
				    real32 tX = 0.0f;
				    real32 tY = 0.0f;
				    
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


1:00:57
in hardware nowadays, if you are on a PC, usually they have fast inverse square root


1:01:57
all of our light information will be in the texture. 
we are going to write all of our lights in to our environment maps. 



1:06:43
Bump map vs Normal Map

nowadays, no one uses a bump map as oppose to a bump map for lighting. 
if you are using a bump map nowadays, either you are just trying to compress your normal map more than you could 

or you are trying to recreate bump map from bump maps.


Bump Maps gives you Height information
	
				Bump Map
				 _______________________					
                |						|				
                |						|						
                |						|			
                |						|			
                |		Height 			|			
                |						|				
                |						|				
                |						|			 
                |_______________________|


Normal map gives you surface direction

				Normal Map
                 _______________________					
                |						|				
                |						|			
                |						|			
                |						|			
                |		Surface 		|			
                |		Direction		|				
                |						|				
                |						|			 
                |_______________________|


pretty much you never want a bump map, unless you are trying to do something else such as better compression,
or trying to do occlusion 

if you are doing surface lighting, the normal/surface direction is almost always a much higher quality way.
the bump map, you will almost have to look at neighboring values to derive the surface normals


1:12:41
normal maps are basically the more powerful version of bump map
bump map is a worst version of a normal map

