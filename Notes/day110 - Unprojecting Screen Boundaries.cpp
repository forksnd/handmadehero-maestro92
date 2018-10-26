Handmade Hero Day 110 - Unprojecting Screen Boundaries

Summary:

wrote the unproject function, in which takes in a camera region and outputs a region in world space 
essentially going from screen space to world space 

it is the same formula mentioned in day 108

makes it so that the Renderer gave us a screen space bound, we convert that screen space bound 
to sim space bound, and we tick the simulation within that bound

Not sure If I like this decision, becuz you are letting the renderer to decide its sim region?
this is allowing the renderer to affect game logic.


Keyword:

Rendering, Unproject


4:55

was looking at places that we are using PixelsToMeters

-	previously we are still using PixelsToMeters to determine the CameraBoundsInMeters.
the CameraBoundsInMeters determines the simulation boundaries, as shown below

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{   


					...
					...

				    
				    loaded_bitmap DrawBuffer_ = {};
				    loaded_bitmap *DrawBuffer = &DrawBuffer_;
				    DrawBuffer->Width = Buffer->Width;
				    DrawBuffer->Height = Buffer->Height;
				    DrawBuffer->Pitch = Buffer->Pitch;
				    DrawBuffer->Memory = Buffer->Memory;

				    Clear(RenderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

				    v2 ScreenCenter = {0.5f*(real32)DrawBuffer->Width,
				                       0.5f*(real32)DrawBuffer->Height};

				    real32 ScreenWidthInMeters = DrawBuffer->Width*PixelsToMeters;
				    real32 ScreenHeightInMeters = DrawBuffer->Height*PixelsToMeters;
				    rectangle3 CameraBoundsInMeters = RectCenterDim(V3(0, 0, 0),
				                                                    V3(ScreenWidthInMeters, ScreenHeightInMeters, 0.0f));
				    CameraBoundsInMeters.Min.z = -3.0f*GameState->TypicalFloorHeight;
				    CameraBoundsInMeters.Max.z =  1.0f*GameState->TypicalFloorHeight;

				    ...
				    ...

				    rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundsExpansion);
				    temporary_memory SimMemory = BeginTemporaryMemory(&TranState->TranArena);
				
				    ...
				    ...
				}



Casey says we want the renderer be the one responsible telling us what that is.
it can decide whether it wants to use a fixed screen width or height, or use a fixed aspect ratio

now we changed it to:


				    rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
				    rectangle3 CameraBoundsInMeters = RectMinMax(V3(ScreenBounds.Min, 0.0f), V3(ScreenBounds.Max, 0.0f));
				    CameraBoundsInMeters.Min.z = -3.0f*GameState->TypicalFloorHeight;
				    CameraBoundsInMeters.Max.z =  1.0f*GameState->TypicalFloorHeight;    


Not sure I like this decision, becuz you are letting the renderer to decide its sim region?
this is allowing the renderer to affect game logic.

essentially what happened is that Renderer gave us a screen space bound, we convert that screen space bound 
to sim space bound, and we did tick the simulation in that bound





7:25
20:05
we added these few relevant functions in handmade_render_group.h that converst screen space bounds to sim space bounds

				inline v2
				Unproject(render_group *Group, v2 ProjectedXY, real32 AtDistanceFromCamera)
				{
				    v2 WorldXY = (AtDistanceFromCamera / Group->GameCamera.FocalLength)*ProjectedXY;
				    return(WorldXY);
				}

				inline rectangle2
				GetCameraRectangleAtDistance(render_group *Group, real32 DistanceFromCamera)
				{
				    v2 RawXY = Unproject(Group, Group->MonitorHalfDimInMeters, DistanceFromCamera);

				    rectangle2 Result = RectCenterHalfDim(V2(0, 0), RawXY);
				    
				    return(Result);
				}

				inline rectangle2
				GetCameraRectangleAtTarget(render_group *Group)
				{
				    rectangle2 Result = GetCameraRectangleAtDistance(Group, Group->GameCamera.DistanceAboveTarget);
				    
				    return(Result);
				}


9:32 
explaining what unproject works 

the equation is exactly the same with the one mentioned in day108, Perspective Projection




				 P				   |
					o			   | P_
								   o
								   |				camera
				-------------------|-------------------o---------------> Z
			 	     			   |
								   |      d (monitor to camera)       
								   |
								   |
			                       |

			     			   monitor



					d * P
			   ______________  = P_

				  Cz - Pz 


in day 108, we are going from P to P_. Here we are just going from P_ to P




1:13:09

added the u,v in the vector2, vector3 class

				union v2
				{
				    struct
				    {
				        real32 x, y;
				    };
				    struct
				    {
				        real32 u, v;
				    };
				    real32 E[2];
				};

				union v3
				{
				    struct
				    {
				        real32 x, y, z;
				    };
				    struct
				    {
				        real32 u, v, w;
				    };
				    struct
				    {
				        real32 r, g, b;
				    };
				    struct
				    {
				        v2 xy;
				        real32 Ignored0_;
				    };
				    struct
				    {
				        real32 Ignored1_;
				        v2 yz;
				    };
				    struct
				    {
				        v2 uv;
				        real32 Ignored2_;
				    };
				    struct
				    {
				        real32 Ignored3_;
				        v2 vw;
				    };
				    real32 E[3];
				};

				