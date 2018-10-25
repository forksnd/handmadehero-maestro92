Handmade Hero Day 108 - Perspective Projection

Summary:

Faking 3D perspective motion for levels fading in and out 
(levels that are distance away, will fade in slower)

Derive the equation for the perspective Projection

put that formula in the GetRenderEntityBasisP(); function

In Q/A, mentioned how when you scale down, you may see some artifact, and how mipmapping
can solve this problem 

Keyword:
Rendering, Perspective Projection,



3:59
while showing the different levels fading in and out,
Casey mentioned that for levels that are distance away, they appear to be traveling faster 

plans to do the math calculation so that the entities have realistic motion



talked about Orthorgraphic and Perspective in 3D 



28:22
graphical explanation of the equation for Perspective




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




derives the equation for perspective 

Cz - Pz = Camera.z - P.z

note that Z axis goes from left to right
so Camera.z is hier


				P_			d
			________ =  ___________

				P 		  Cz - Pz



				d * P
		   ______________  = P_

			  Cz - Pz 



35:25
putting this formula into our code, we edit the GetRenderEntityBasisP(); function

-	CameraDistanceAboveTarget is Cz 
-	EntityBaseP is Pz
-	DistanceToPZ is Cz - Pz
-	FocalLength is d
-	the NearClipPlane is essentially what zNear is in the gluProject function


				 P				   |             near clip plane
					o			   | P_           |  
								   o          <---|
								   |			  |	   camera
				-------------------|--------------|----o---------------> Z
			 	     			   |              |
								   |      d (monitor to camera)       
								   |
								   |
			                       |

			     			   monitor

    essentially we are only rendering things in front of the near clip plane

-	here is the actual function

-	51:49
	had the problem of whether to do this function in pixel space or world space.
	i am skipping that part 

				inline entity_basis_p_result GetRenderEntityBasisP(render_group *RenderGroup, render_entity_basis *EntityBasis,
				                                                   v2 ScreenCenter)
				{
				    entity_basis_p_result Result = {};

				    v3 EntityBaseP = RenderGroup->MetersToPixels*EntityBasis->Basis->P;

				    // TODO(casey): The values of 20 and 20 seem wrong - did I mess something up here?
				    real32 FocalLength = RenderGroup->MetersToPixels*20.0f;
				    real32 CameraDistanceAboveTarget = RenderGroup->MetersToPixels*20.0f;
				    real32 DistanceToPZ = (CameraDistanceAboveTarget - EntityBaseP.z); 		
				    real32 NearClipPlane = RenderGroup->MetersToPixels*0.2f;

				    v3 RawXY = V3(EntityBaseP.xy + EntityBasis->Offset.xy, 1.0f);

				    if(DistanceToPZ > NearClipPlane)
				    {
				        v3 ProjectedXY = (1.0f / DistanceToPZ) * FocalLength*RawXY;        
				        Result.P = ScreenCenter + ProjectedXY.xy;
				        Result.Scale = ProjectedXY.z;
				        Result.Valid = true;
				    }
				    
				    return(Result);
				}



-	GetRenderEntityBasisP(); function gets called in the RenderGroupToOutput(); function;

				internal void
				RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
				{
		
				    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize;  )
				    {

						...
						...
				        switch(Header->Type)
				        {
				        	
				        	...

			            	case RenderGroupEntryType_render_entry_bitmap:
				            {
				                render_entry_bitmap *Entry = (render_entry_bitmap *)Data;

				                entity_basis_p_result Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCenter);
				                Assert(Entry->Bitmap);

				            }
				            ...
				            ...
				        }
				}




1:09:57
explained what Field of View is in Q/A

that is essentially the angle argument you pass in to gluProject

Casey explains how to to add FOV into this current code



1:19:57
once you scale down to a certain size, you will notice that the trees dont look smooth
the pixels on the edge dont look as good 

whereas if you render in the normal size, the pixels look nice and smooth.

The reason why is becuz we are only doing bilinear filtering, which means once you scale below half size,
anything from half size down, you are no longer sampling all of the pixels from original bitmap. 
This is where you need mipmapping


