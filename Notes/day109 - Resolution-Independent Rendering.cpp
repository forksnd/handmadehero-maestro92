Handmade Hero Day 109 - Resolution-Independent Rendering

Summary:

decoupled GameState->MeterToPixels from the rendering pipeline

Brifely explained how transformations work 3D rendering pipeline
projection ---> clip space ---> NDC ---> screen space

decided for this 2D game, will go straight from
world to screen space 

I really like his approach of tackling this rendering transformation pipeline problem
makes everything decoupled

Keyword:
rendering, coordiante system in the rendering pipeline



12:12
mentioned that the usage of GameState->MeterToPixels is completely wrong
aims to get rid of MeterToPixels from the rendering pipeline


15:08
previously our loaded_bitmap stores the alignment in pixels 

				struct loaded_bitmap
				{
				    v2 Align;
				    
				    int32 Width;
				    int32 Height;
				    int32 Pitch;
				    void *Memory;
				};

to decouple MeterToPixels, we are changing that to percentage

				struct loaded_bitmap
				{
				    v2 AlignPercentage;
				    real32 WidthOverHeight;
				    
				    int32 Width;
				    int32 Height;
				    int32 Pitch;
				    void *Memory;
				};


16:45
now when we call PushBitmap(); we calculate the alignment without MeterToPixels


				inline void
				PushBitmap(render_group *Group, loaded_bitmap *Bitmap, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
				{
				    render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap);
				    if(Entry)
				    {
				        Entry->EntityBasis.Basis = Group->DefaultBasis;
				        Entry->Bitmap = Bitmap;
				        v2 Align = Hadamard(Bitmap->AlignPercentage, V2i(Bitmap->Width, Bitmap->Height));
				        Entry->EntityBasis.Offset = Offset - V3(Align, 0);
				        Entry->Color = Group->GlobalAlpha*Color;
				    }
				}


33:31
explaining in 3D, how things gets rendered to the screen

pretty much he summarized my "OpenGL Project Matrix" Goodle Doc Article 

essentially how
projection converts your scene into a unit cube.

Then the GPU converts the unit cube to screen space
essentially taking the [-1, 1] range to [0, screen width] range

My google doc does a better job explaining


37:43
we will just skip the clip space and NDC space, we will just do 
world  ---> projection ---> screen

this will be the only place we use MetersToPixels




55:12
mimicing the NDC to screen transformation in the RnederGroupToOutput(); function

mentioned that he wants the dimensions of the OutputTarget

-	and we calcualte MetersToPixels depending on the screen dimensions
	we then pass the ScreenDim and MetersToPixels to 


				handmade_render_group.cpp

				internal void
				RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
				{
				    v2 ScreenDim = {(real32)OutputTarget->Width,
				                    (real32)OutputTarget->Height};

				    // TODO(casey): Remove this :)
				    real32 MetersToPixels = ScreenDim.x / 20.0f;
				    real32 PixelsToMeters = 1.0f / MetersToPixels;


				    case xxx:

				    	break;

		            case RenderGroupEntryType_render_entry_bitmap:
		            {
		                render_entry_bitmap *Entry = (render_entry_bitmap *)Data;

		                entity_basis_p_result Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim,
		                                                                    MetersToPixels);

		                ...
		                ...
		                break;
				    	

				    	...
				    	...

				    }
				}


41:53
then inside the GetRenderEntityBasis(); function, this is only place we will now use MetersToPixels, which we 
is are going from world to screen

also the screenCenter is also calculated based on the ScreenDim that is passed in.

			    v2 ScreenCenter = 0.5f*ScreenDim;

			    ...
			    ...

		        Result.P = ScreenCenter + MetersToPixels*ProjectedXY.xy;
		        Result.Scale = MetersToPixels*ProjectedXY.z;


this way the transformation pipeline is alot more clear, it is just like OpenGLs 3D pipeline

code below:
				handmade_render_group.cpp

				inline entity_basis_p_result GetRenderEntityBasisP(render_group *RenderGroup, render_entity_basis *EntityBasis,
				                                                   v2 ScreenDim, real32 MetersToPixels)
				{
				    v2 ScreenCenter = 0.5f*ScreenDim;
				    
				    entity_basis_p_result Result = {};

				    v3 EntityBaseP = EntityBasis->Basis->P;

				    real32 FocalLength = 6.0f;
				    real32 CameraDistanceAboveTarget = 5.0f;
				    real32 DistanceToPZ = (CameraDistanceAboveTarget - EntityBaseP.z);
				    real32 NearClipPlane = 0.2f;

				    v3 RawXY = V3(EntityBaseP.xy + EntityBasis->Offset.xy, 1.0f);

				    if(DistanceToPZ > NearClipPlane)
				    {
				        v3 ProjectedXY = (1.0f / DistanceToPZ) * FocalLength*RawXY;        
				        Result.P = ScreenCenter + MetersToPixels*ProjectedXY.xy;
				        Result.Scale = MetersToPixels*ProjectedXY.z;
				        Result.Valid = true;
				    }
				    
				    return(Result);
				}






1:01:05
tested this by changing different resolutions 
and it surely does work :)

							
				int CALLBACK
				WinMain(HINSTANCE Instance,
				        HINSTANCE PrevInstance,
				        LPSTR CommandLine,
				        int ShowCode)
				{
					...
					...

				    Win32ResizeDIBSection(&GlobalBackbuffer, 960, 540);
				//    Win32ResizeDIBSection(&GlobalBackbuffer, 1920, 1080);

				    ...
				    ...
				}


1:17:19
someone mentioned in the Q/A
is the Hero supposed to shrink on the stairs?

Casey says the reason is becuz the camera position is always one frame behind

the solution is to inform the renderer about camera location at the end of the frame,
so there isnt a frame of lag in camera updating compared to the hero



