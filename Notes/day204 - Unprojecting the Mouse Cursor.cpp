Handmade Hero Day 204 - Unprojecting the Mouse Cursor

Summary:
mentions that Casey wants to make mouse picking work, so we can mouse pick an entity and debug examine that entity.
debugging project and unproject 

Keyword:
project and unproject


5:58
Casey wants to take a look at our debug_variable data
right now the hierarchy is explicitly constructed and explicitly defined, and  they exist all the time in memory 

meaning in the DEBUGStart(); function where we initalize all of our debug stuff,
we explicit build our hierarchy

                handmade_debug.cpp

                internal void DEBUGStart(debug_state *DebugState, game_assets *Assets, u32 Width, u32 Height)
                {

                    if(!DebugState->Initialized)
                    {
                        DebugState->HighPriorityQueue = DebugGlobalMemory->HighPriorityQueue;
                        DebugState->TreeSentinel.Next = &DebugState->TreeSentinel;
                        DebugState->TreeSentinel.Prev = &DebugState->TreeSentinel;
                        DebugState->TreeSentinel.Group = 0;
                        
                        InitializeArena(&DebugState->DebugArena, DebugGlobalMemory->DebugStorageSize - sizeof(debug_state), DebugState + 1);

                        debug_variable_definition_context Context = {};
                        Context.State = DebugState;
                        Context.Arena = &DebugState->DebugArena;
                        Context.GroupStack[0] = 0;

                        DebugState->RootGroup = DEBUGBeginVariableGroup(&Context, "Root");
                        DEBUGBeginVariableGroup(&Context, "Debugging");

                        DEBUGCreateVariables(&Context);
                        DEBUGBeginVariableGroup(&Context, "Profile");
                        DEBUGBeginVariableGroup(&Context, "By Thread");
                        DEBUGAddVariable(&Context, DebugVariableType_CounterThreadList, "");
                        DEBUGEndVariableGroup(&Context);
                        DEBUGBeginVariableGroup(&Context, "By Function");
                        DEBUGAddVariable(&Context, DebugVariableType_CounterThreadList, "");
                        DEBUGEndVariableGroup(&Context);
                        DEBUGEndVariableGroup(&Context);

                        ...
                        ...
                    }
                }

6:50
the problem with our current structure is that it makes it kind of annoying for code that wants to use this
to quickly expose debug variables to the system

now anyone who wants to expose debug variables has to go through the trouble of creating them and putting them somewhere.


8:24
Casey wants to add the functionality to inspect an entity


13:14
Casey now wants to render the sim_entity Collision volumes so we can see them 
recall in our sim_entity struct, we have the Collisions as their physics bounding volume 

                handmade_sim_region.h

                struct sim_entity 
                {
                    ...
                    ...

                    sim_entity_collision_volume_group *Collision;

                    ...
                };



16:08
so previously in our rendering code, we would render the outlines. We added a 
DEBUGUI_DrawEntityOutlines flag, and we render the outlines


                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    // TODO(casey): Move this out into handmade_entity.cpp!
                    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
                    {
                        switch(Entity->Type)
                        {
                            ...
                            ...
                        }
            
#if DEBUGUI_DrawEntityOutlines
                        for(uint32 VolumeIndex = 0;
                            VolumeIndex < Entity->Collision->VolumeCount;
                            ++VolumeIndex)
                        {
                            sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        
                            PushRectOutline(RenderGroup, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, V4(0, 0.5f, 1.0f, 1));
                        }
#endif

                    }
                }


26:07
Casey explaining mouse picking.
one way to think of it is to find all entities that intersect that mouse picking line 


                    
27:13
Casey proceeds to write the code for the mouse picking 

Casey will now highlight world entities if the mouse is hovering it ontop of it.


                v2 MetersMouseP = MouseP*(1.0f / RenderGroup->Transform.MetersToPixels);
                r32 LocalZ = 10.0f;
                v2 WorldMouseP = Unproject(RenderGroup, MetersMouseP, LocalZ);
                RenderGroup->Transform.OffsetP = V3(WorldMouseP, RenderGroup->Transform.DistanceAboveTarget - LocalZ);
                PushRect(RenderGroup, V3(0, 0, 0), V2(1.0f, 1.0f),
                         V4(0.0f, 1.0f, 1.0f, 1.0f));

                if(EntityIndex == 10)
                {
                    for(uint32 VolumeIndex = 0;
                        VolumeIndex < Entity->Collision->VolumeCount;
                        ++VolumeIndex)
                    {
                        sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        

                        real32 LocalZ = RenderGroup->Transform.OffsetP.z + Volume->OffsetP.z;
                        v4 OutlineColor = V4(1, 0, 1, 1);
                        v2 LocalMouseP = (Unproject(RenderGroup, MetersMouseP, LocalZ) -
                                          (RenderGroup->Transform.OffsetP.xy + Volume->OffsetP.xy));
                        PushRect(RenderGroup, V3(LocalMouseP, Volume->OffsetP.z), V2(1.0f, 1.0f),
                                 V4(0.0f, 1.0f, 1.0f, 1.0f));

                        if((LocalMouseP.x > -0.5f*Volume->Dim.x) && (LocalMouseP.x < 0.5f*Volume->Dim.x) &&
                           (LocalMouseP.y > -0.5f*Volume->Dim.y) && (LocalMouseP.y < 0.5f*Volume->Dim.y))
                        {
                            OutlineColor = V4(1, 1, 0, 1);
                        }


                        PushRectOutline(RenderGroup, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, OutlineColor, 0.05f);
                    }
                }




the rest of the stream is pretty much debugging Project(); and UnProject();


1:14:51
Casey writing the CompleteUnproject(); function

                inline v2 CompleteUnproject()
                {
                    if(Transform->Orthographic)
                    {
                        Result.P = Transform->ScreenCenter + Transform->MetersToPixels*P.xy;
                    }
                    else
                    {
                        v2 A = (FinalP - Transform->ScreenCenter) / Transform->MetersToPixels;
                        v2 Result = ((Transform->DistanceAboveTarget - P.z)/Transform->FocalLength)*A; 
                    }

                    Result -= Transform->OffsetP;
                }