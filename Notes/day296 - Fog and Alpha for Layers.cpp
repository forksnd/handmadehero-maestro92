Handmade Hero Day 296 - Fog and Alpha for Layers

Summary:
making rendering API cleaner
fixing a sRGB related bug in the OpenGL rendering path

Keyword:
rendering code clean up


8:19
we would like the notion that floors above us alpha in as they come down, but those far away floors 
we dont want them to alpha. we want to fog them out instead of alpha because they dont get transparent.
we dont start seeing through them, instead they just become dimmer, and they get a bit of a haze effect.




9:49
Casey mentioned that in 

                struct render_group
                {
                    struct game_assets *Assets; 
    ----------->    real32 GlobalAlpha;

                    v2 MonitorHalfDimInMeters;

                    camera_transform CameraTransform;

                    uint32 MissingResourceCount;
                    b32 RendersInBackground;

                    u32 CurrentClipRectIndex;
                    
                    u32 GenerationID;
                    game_render_commands *Commands;
                };

20:00 ~ 28:20
1:03:39
Casey cleaning up and discussing how he wants to pass the Color value in the rendering system to achieve for desired alpha effect
eventually the code looks like:

                handmade_world_mode.cpp

                internal b32 UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState,
                                     game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
                {

                    ...
                    ...

                    if(Entity->Updatable)
                    {
                        // TODO(casey): Probably indicates we want to separate update and render
                        // for entities sometime soon?
                        v3 CameraRelativeGroundP = GetEntityGroundPoint(Entity) - CameraP;

                        if(CameraRelativeGroundP.z > FadeTopStartZ)
                        {
                            r32 t = Clamp01MapToRange(FadeTopStartZ, CameraRelativeGroundP.z, FadeTopEndZ);
                            RenderGroup->tGlobalColor = V4(0, 0, 0, t);
                            RenderGroup->GlobalColor = V4(0, 0, 0, 0);
                        }
                        else if(CameraRelativeGroundP.z < FadeBottomStartZ)
                        {
                            r32 t = Clamp01MapToRange(FadeBottomStartZ, CameraRelativeGroundP.z, FadeBottomEndZ);
                            RenderGroup->tGlobalColor = V4(t, t, t, 0.0f);
                            RenderGroup->GlobalColor = BackgroundColor;
                        }
                        else
                        {   
                            RenderGroup->tGlobalColor = V4(0, 0, 0, 0);
                        }

                        ...
                        ...
                    }

                    ...
                }


30:55
Casey wonders if the GL colors are getting premultiplied automatically

lots of debugging



57:11
Casey mentioned that he likes to use a trick to debug when you dont have good debugging facilities.
so when we do a render entry, 


Casey adds a DebugTag in the render_group_entry_header

                handmade_render_group.h

                struct render_group_entry_header // TODO(casey): Don't store type here, store in sort index?
                {
                    u16 Type;
                    u16 ClipRectIndex;
                #if HANDMADE_SLOW
                    u32 DebugTag;
                #endif
                };

and then we specifically, set a condition in OpenGLRenderCommands(); to help debugging

                handmade_opengl.cpp

                internal void OpenGLRenderCommands(game_render_commands *Commands, s32 WindowWidth, s32 WindowHeight)
                {    
                    ...
                    ...

                    u32 ClipRectIndex = 0xFFFFFFFF;
                    sort_entry *Entry = SortEntries;
                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        ...
                        ...

                        if(Header->DebugTag == 1)
                        {
                            int BreakHere = true;
                        }

                        ...
                        ...
                    }
                }



nothing interesting in Q/A