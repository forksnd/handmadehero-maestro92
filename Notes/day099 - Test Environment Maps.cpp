Handmade Hero Day 099 - Test Environment Maps

Summary:
did more code for the environment_map.

first programmtically created three plan Red, Green, Blue textures for the environment_map.

showed a neat setup for debugging texture related rendering work

rendered the the textures onscreen, while rendering the sphere as well

then programmtically created three checker board textures to test texture sampling code.
the texture sampling testing will be done on episode day 100.

demonstrated how to change the saturation values without doing rgb -> HSV conversions

Keyword:
rendering, environment_maps, debugging rendering, pixel color saturation 



3:45
added a graphical explanation of how we want to use Top, Middle, Bottom Environment Maps





8:33
added the environment_map in the transient_state 

				handmade.h

				struct transient_state
				{
					...
					...

				    uint32 EnvMapWidth;
				    uint32 EnvMapHeight;
				    // NOTE(casey): 0 is bottom, 1 is middle, 2 is top
				    environment_map EnvMaps[3];
				};




10:16
we initialize the environment_map in Transient state initialization code. Pretty Straight forward code.

				handmade.cpp

			    if(!TranState->IsInitialized)
			    {
			        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
			                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));

			        ...
			        ...

			        GameState->TestDiffuse = MakeEmptyBitmap(&TranState->TranArena, 256, 256, false);
			        DrawRectangle(&GameState->TestDiffuse, V2(0, 0), V2i(GameState->TestDiffuse.Width, GameState->TestDiffuse.Height), V4(0.5f, 0.5f, 0.5f, 1.0f));
			        GameState->TestNormal = MakeEmptyBitmap(&TranState->TranArena, GameState->TestDiffuse.Width, GameState->TestDiffuse.Height, false);
			        MakeSphereNormalMap(&GameState->TestNormal, 0.0f);

			        TranState->EnvMapWidth = 512;
			        TranState->EnvMapHeight = 256;
			        for(uint32 MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex)
			        {
			            environment_map *Map = TranState->EnvMaps + MapIndex;
			            uint32 Width = TranState->EnvMapWidth;
			            uint32 Height = TranState->EnvMapHeight;
			            for(uint32 LODIndex = 0; LODIndex < ArrayCount(Map->LOD); ++LODIndex)
			            {
			                Map->LOD[LODIndex] = MakeEmptyBitmap(&TranState->TranArena, Width, Height, false);
			                Width >>= 1;
			                Height >>= 1;
			            }
			        }

			        TranState->IsInitialized = true;
			    }




15:24
modified the environment_map class. Made it so that LOD are no longer pointers

				struct environment_map
				{
				    loaded_bitmap LOD[4];
				};



18:35
decided to render the texture on screen while having the tree rendered in the world 
very smart setup. Very helpful for debugging especially when you are rendering textures


26:10
take a look at the screen shot of how Casey debugs rendering a certain texture


26:40 
we can see that the bottom is sampling the blue properly, while the red on top is inverted


27:04
apparently, the red is supposed to be our ground plane, while the blue is our top plane, so the blue 
and red is flipped as well


30:51
we want our bitmaps to be bottom up, not top down. That is becuz most graphics api including OpenGL, they want 
the bitmaps to be bottom up as well



34:17

made the following changes for rendering the normal map 

initially, we had 

                environment_map *FarMap = 0;
                real32 tEnvMap = Normal.y;
                real32 tFarMap = 0.0f;
                if(tEnvMap < -0.5f)
                {
                    FarMap = Bottom;
                    tFarMap = 2.0f*(tEnvMap + 1.0f)
                }
                else if(tEnvMap > 0.5f)
                {
                    FarMap = Top;
                    tFarMap = 2.0f*(tEnvMap - 0.5f);
                }

we fix the tEnvMap < -0.5f case 

                environment_map *FarMap = 0;
                real32 tEnvMap = Normal.y;
                real32 tFarMap = 0.0f;
                if(tEnvMap < -0.5f)
                {
                    FarMap = Bottom;
                    tFarMap = -1.0f - 2.0f*tEnvMap;
                }
                else if(tEnvMap > 0.5f)
                {
                    FarMap = Top;
                    tFarMap = 2.0f*(tEnvMap - 0.5f);
                }

     
and you can see the correct results then



35:24
mentions that we need to write more tests to ensure we are sampling from the environment_map accurately.
Since the RGB texture test is not enough

mentions that he will programmtcailly create a checker board texture to test




36:54
writing code to produce the checker board texture, pretty straightforward

			    v3 MapColor[] =
			    {
			        {1, 0, 0},
			        {0, 1, 0},
			        {0, 0, 1},
			    };
			    for(uint32 MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex)
			    {
			        environment_map *Map = TranState->EnvMaps + MapIndex;
			        loaded_bitmap *LOD = Map->LOD + 0;
			        bool32 RowCheckerOn = false;
			        int32 CheckerWidth = 16;
			        int32 CheckerHeight = 16;
			        for(int32 Y = 0; Y < LOD->Height; Y += CheckerHeight)
			        {
			            bool32 CheckerOn = RowCheckerOn;
			            for(int32 X = 0; X < LOD->Width; X += CheckerWidth)
			            {
			                v4 Color = CheckerOn ? ToV4(MapColor[MapIndex], 1.0f) : V4(0, 0, 0, 1);
			                v2 MinP = V2i(X, Y);
			                v2 MaxP = MinP + V2i(CheckerWidth, CheckerHeight);
			                DrawRectangle(LOD, MinP, MaxP, Color);
			                CheckerOn = !CheckerOn;
			            }
			            RowCheckerOn = !RowCheckerOn;
			        }
			    }

43:57 
shows the checker board texture drawn on screen

44:58
will proceed to test the sampling code. Casey mentions that he wants to clear the tree, becuz he rather 
see just the normal map

so instead of rendering a tree, he will render a testDiffuse Bitmap and a testNormal bitmap



53:11
someone asks will this technique let us have a pool of water reflect a starry sky?

Casey offers a graphical explanation. The technique works, but the reflection will only sample from the environment_map sparsely.
so depending on the resolution, there is no gurantee that you wont see artifacts.

Casey says he cant gurantee that he will solve that problem, cuz what he is looking for is more lighting, but not reflections


1:09:52
Someone in the Q/A asked how to change the saturation

				struct render_entry_saturation
				{
				    real32 Level;
				};




added the case for render_entry_saturation

	            case RenderGroupEntryType_render_entry_saturation:
	            {
	                render_entry_saturation *Entry = (render_entry_saturation *)Data;

	                ChangeSaturation(OutputTarget, Entry->Level);

	                BaseAddress += sizeof(*Entry);
	            } break;





1:13:03
the saturation is like how far from gray your color is. 
The formal way is to do a converstion from RGB to HSV, then convert them back.

But there are different hacks you can do.

so one way you can do is average of your R,G,B 


							R + G + B 
				avg	 =	__________________

								3

imagine your R G B values 

				dr = R - avg 
				dg = G - avg 
				dB = B - avg

and we multiply the deltas by the saturation levels 


1:14:23
the idea is that we go through all the pixels on the output buffer and change the saturation level 


				internal void
				ChangeSaturation(loaded_bitmap *Buffer, real32 Level)
				{
				    uint8 *DestRow = (uint8 *)Buffer->Memory;
				    for(int Y = 0; Y < Buffer->Height; ++Y)
				    {
				        uint32 *Dest = (uint32 *)DestRow;
				        for(int X = 0; X < Buffer->Width; ++X)
				        {
				            v4 D = {(real32)((*Dest >> 16) & 0xFF),
				                    (real32)((*Dest >> 8) & 0xFF),
				                    (real32)((*Dest >> 0) & 0xFF),
				                    (real32)((*Dest >> 24) & 0xFF)};

				            D = SRGB255ToLinear1(D);

				            real32 Avg = (1.0f / 3.0f) * (D.r + D.g + D.b);
				            v3 Delta = V3(D.r - Avg, D.g - Avg, D.b - Avg);
				            
				            v4 Result = ToV4(V3(Avg, Avg, Avg) + Level*Delta, D.a);

				            Result = Linear1ToSRGB255(Result);

				            *Dest = (((uint32)(Result.a + 0.5f) << 24) |
				                     ((uint32)(Result.r + 0.5f) << 16) |
				                     ((uint32)(Result.g + 0.5f) << 8) |
				                     ((uint32)(Result.b + 0.5f) << 0));
				            
				            ++Dest;
				        }

				        DestRow += Buffer->Pitch;
				    }
				}