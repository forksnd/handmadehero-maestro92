Handmade Hero Day 297 - Separating Entities into Z Layers

Summary:
starting to fix fading problems for entities and levels below current floor.
planning to fix it by putting render group and entities into slices


Keyword:
Rendering 


0:30
Casey mentions that he wants to prioritize making "slices" functional

essentailly we want the notion that entities on a level and the level itself should be on the same "slice"
essentially everything on one level should fade out as a single image. 

2:49
Casey mentioning the incorrect fade behaviour that he wants to fix.
Mainly, as the main character goes doestairs (going down a level);, the current level fades out 
and currently we can see the tile and the monstar fading out. however, we can see the tile through the monstar

For things on the level above us as our character go does stairs, we dont want the objects that are sitting on tiles to fade out 
as well as the tiles fading out. we just want the whole image to fade out as one image. 


3:41
Casey also mentioned that for our lighting, the rendering pipeline would also want to have an understanding of "slices".


4:40
Casey mentioned the couple of ways he wants to achieve it


9:32
Casey want to make level 0 the floor the player is at. negative numbers to be levels below the player floor
positive numbers are floors above


15:22
so what we do is that we define a few ClipRects. 
and we are pushing all rendered_rects and sprites into the few clipRects 

                handmade_render_group.cpp

                internal void UpdateAndRenderEntities(game_mode_world *WorldMode, sim_region *SimRegion, render_group *RenderGroup, v3 CameraP,
                                        loaded_bitmap *DrawBuffer, v4 BackgroundColor, r32 dt, transient_state *TranState, v2 MouseP)
                {
                    TIMED_FUNCTION();

                    ...
                    ...


                #define MinimumLevelIndex -4
                #define MaximumLevelIndex 1
                    
                    u32 ClipRectIndex[(MaximumLevelIndex - MinimumLevelIndex + 1)];
                    for(u32 LevelIndex = 0; LevelIndex < ArrayCount(ClipRectIndex); ++LevelIndex)
                    {
                        ...
                        ...
                        
                        ClipRectIndex[LevelIndex] = PushClipRect(RenderGroup, 0, 0, DrawBuffer->Width, DrawBuffer->Height, FX);
                    }

                    ...
                    ...
                }








30:32
                handmade_world_mode.cpp

                internal b32 UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState,
                                     game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
                {

                    ...
                    ...

                    UpdateAndRenderEntities(WorldMode, SimRegion, RenderGroup, CameraP,
                            DrawBuffer, BackgroundColor, dt, TranState, MouseP);

                    ...
                    ...
                }







36:10
Now Casey examines the RenderGroup, depending of the render_group_s z offset, Casey puts it in one of the clipRect 
as you can see, we assign the proper clipRectIndex to the RenderGroup 

                handmade_render_group.cpp

                internal void UpdateAndRenderEntities(game_mode_world *WorldMode, sim_region *SimRegion, render_group *RenderGroup, v3 CameraP,
                                        loaded_bitmap *DrawBuffer, v4 BackgroundColor, r32 dt, transient_state *TranState, v2 MouseP)
                {
                    TIMED_FUNCTION();

                    ...
                    ...


                #define MinimumLevelIndex -4
                #define MaximumLevelIndex 1
                    
                    u32 ClipRectIndex[(MaximumLevelIndex - MinimumLevelIndex + 1)];
                    for(u32 LevelIndex = 0; LevelIndex < ArrayCount(ClipRectIndex); ++LevelIndex)
                    {
                        // TODO(casey): Probably indicates we want to separate update and render
                        // for entities sometime soon?
                        s32 RelativeLayerIndex = MinimumLevelIndex + LevelIndex;
                        r32 CameraRelativeGroundZ = SimRegion->Origin.Offset_.z + (r32)RelativeLayerIndex*WorldMode->TypicalFloorHeight;
                        
                        clip_rect_fx FX = {};
                        if(CameraRelativeGroundZ > FadeTopStartZ)
                        {
                            RenderGroup->CurrentClipRectIndex = ClipRectIndex[0];

                            r32 t = Clamp01MapToRange(FadeTopStartZ, CameraRelativeGroundZ, FadeTopEndZ);
                            FX.tColor = V4(0, 0, 0, t);
                        }
                        else if(CameraRelativeGroundZ < FadeBottomStartZ)
                        {
                            RenderGroup->CurrentClipRectIndex = ClipRectIndex[1];

                            r32 t = Clamp01MapToRange(FadeBottomStartZ, CameraRelativeGroundZ, FadeBottomEndZ);
                            FX.tColor = V4(t, t, t, 0.0f);
                            FX.Color = BackgroundColor;
                        }
                        else
                        {   
                            RenderGroup->CurrentClipRectIndex = ClipRectIndex[2];
                        }
                        
                        ClipRectIndex[LevelIndex] = PushClipRect(RenderGroup, 0, 0, DrawBuffer->Width, DrawBuffer->Height, FX);
                    }

                    ...
                    ...
                }


42:37
Casey mentioned that what we need to do for each entity, for each entity, we need to determine
which slice it goes into and the z depth within that slice 



51:16
Casey added the function 

                inline s32 ConvertToLayerRelative(game_mode_world *WorldMode, r32 *Z)
                {
                    s32 RelativeIndex = 0;
                    RecanonicalizeCoord(WorldMode->TypicalFloorHeight, &RelativeIndex, Z);
                    return(RelativeIndex);
                }

and the entity calls into that function to dtermine its slice index

        
                handmade_entity.cpp

                internal void UpdateAndRenderEntities(game_mode_world *WorldMode, sim_region *SimRegion, render_group *RenderGroup, v3 CameraP,
                                        loaded_bitmap *DrawBuffer, v4 BackgroundColor, r32 dt, transient_state *TranState, v2 MouseP)
                {

                    ...
                    ...

                    object_transform EntityTransform = DefaultUprightTransform();
                    EntityTransform.OffsetP = GetEntityGroundPoint(Entity) - CameraP;
                    s32 RelativeLayer = ConvertToLayerRelative(WorldMode, &EntityTransform.OffsetP.z);

                    ...
                    ...
                }    

55:57
Casey mentioned that we just need three layers, one above us, current level, everything below us.
