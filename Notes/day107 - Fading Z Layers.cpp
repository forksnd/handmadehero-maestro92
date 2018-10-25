Handmade Hero Day 107 - Fading Z Layers

Summary:

starting to work on the fading Z Layer. 

Did the stupid approach to get the logic in first.
Will get the Proper render texture approach in later episode 

mentions the pattern to update and render for entities in your engine

mentioned what Z-fighting is in Q/A 

Keyword:
brute force Z Layer fading, Z-fighting








16:58
we want to have the flexiblity to have entities update and render together if it is more efficient that way
or do two passes: one pass to update, another to render 

if we have particle system, maybe update and render is more efficient

its always a good idea to have update and render be togther. Only separate them if you have a clear reason to do so







32:18
wrote the fading function 
-	we loop through all the entities in our sim region

-	then we define the fade behaviour based on our floor height. 
	notice we are changing the values based on CameraRelativeGroundP.z

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{  

					...
					...

				    sim_region *SimRegion = BeginSim(&TranState->TranArena, GameState, GameState->World,
                                     SimCenterP, SimBounds, Input->dtForFrame);

				    // TODO(casey): Move this out into handmade_entity.cpp!
				    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
				    {
				        sim_entity *Entity = SimRegion->Entities + EntityIndex;

			        	...
			        	...

			            RenderGroup->DefaultBasis = Basis;

			            // TODO(casey): Probably indicates we want to separate update and render
			            // for entities sometime soon?
			            v3 CameraRelativeGroundP = GetEntityGroundPoint(Entity) - CameraP;
			            real32 FadeTopEndZ = 0.75f*GameState->TypicalFloorHeight;
			            real32 FadeTopStartZ = 0.5f*GameState->TypicalFloorHeight;
			            real32 FadeBottomStartZ = -2.0f*GameState->TypicalFloorHeight;
			            real32 FadeBottomEndZ = -2.25f*GameState->TypicalFloorHeight;;           
			            RenderGroup->GlobalAlpha = 1.0f;
			            if(CameraRelativeGroundP.z > FadeTopStartZ)
			            {
			                RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeTopEndZ, CameraRelativeGroundP.z, FadeTopStartZ);
			            }
			            else if(CameraRelativeGroundP.z < FadeBottomStartZ)
			            {
			                RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeBottomEndZ, CameraRelativeGroundP.z, FadeBottomStartZ);
			            }
			            
			            hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[Entity->FacingDirection];

			            ...
			            ...



32:48
wrote a map utility function 
- this produces a value between 0 and 1, based on where t falls under betwen Min and Max
				
				handmade_math.h 

				inline real32
				Clamp01MapToRange(real32 Min, real32 t, real32 Max)
				{
				    real32 Result = 0.0f;
				    
				    real32 Range = Max - Min;
				    if(Range != 0.0f)
				    {
				        Result = Clamp01((t - Min) / Range);
				    }

				    return(Result);
				}



1:13:41
mentioned why z fight occurs in the Q/A

this happens when two surfaces are sufficiently far from the camera, 
such that the floating point position cant be distinguished  




things get worst if you slanty

so what you see that the floating point precision may oscillate
so if you have two planes, A and B, at every other point, 
you may see no bits in the separating the mantissa in the floating point number

and you will see this stippling pattern (I saw this while doing shadow mapping)

                / /
             A / /
              / /
             / / B

essentially it is when the precision breaks down and we can no longer tell the difference
