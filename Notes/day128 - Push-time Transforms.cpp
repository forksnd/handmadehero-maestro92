Handmade Hero Day 128 - Push-time Transforms

Summary:

mentioned cache line problems in our current multi-rendering system

made the render_group in the renderer more 3D ish.
introduced the concept of render_transform for render_groups
placed all the position related variables within render_groups

transforms are not calculated at Push-time (when we call PushBitmap(); or other Push functions)

Keyword:
renderer


2:45
we want to make all the tiles cache aligned.

    
4:32 
Casey mentioned the contentions the cores may have on the edge.

             ___________________
            | 0  | 1  |    |    |
            |____|____|____|____|
            | 2  | 3  |    |    |
            |____|____|____|____|
            |    |    |    |    |
            |____|____|____|____|
            |    |    |    |    |
            |____|____|____|____|


depending on the processor, the cache line size is different.


7:02
as long as the cache line end on the tile boundary, we will never have contentions between Cores.

if we do not have the cache line end on the tile boundary, then we will have a cache line that span across tiles 

                                     .
                 tile 0              .     tile 1
                                     .
             ____________________________________________
            | 0  |    |    |    |    | 1  |    |    |    |
            |____|____|____|____|____|____|____|____|____|
                    .                .                 .
                    ....................................
                                     .

while we dont have a bug in terms of contention, but causes complications.
assume that both core 0 and core 1 pulls in the cache.
whenever core 0 writes to the cache, it will also have to make core 1 cache line invalid.

so when core 1 starts doing work, it has to re pull the cache line from memory or snoop it out of core 0 cache 


19:50
the current rendering logic and entity game logic is as follow:
we try to do pre-tick game logic and render at the same time.
and then we tick

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
                    {
                        sim_entity *Entity = SimRegion->Entities + EntityIndex;
 
                        switch(Entity->Type)
                        {
                            case EntityType_Hero:
                            {
                                ...
                                ... add game logic ...

                                PushBitmap(RenderGroup, &GameState->Shadow, HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));

                                ...
                                ...
                            } break;

                            ...
                            ...
                        }


                        ... we tick here ...
                        if(!IsSet(Entity, EntityFlag_Nonspatial) &&
                           IsSet(Entity, EntityFlag_Moveable))
                        {
                            MoveEntity(GameState, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
                        }

                        Basis->P = GetEntityGroundPoint(Entity);
                    
                    }

this is bad cuz that means our rendered position is delayed for one frame

what we rather want is we tick the entity, and then we render.

so the code now is:
                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
                    {
                        sim_entity *Entity = SimRegion->Entities + EntityIndex;

                        switch(Entity->Type)
                        {
                            case EntityType_Hero:
                            {
                                ... do pre tick game logic ...
                            }
                        }

                        ... we tick ...
                        if(!IsSet(Entity, EntityFlag_Nonspatial) &&
                           IsSet(Entity, EntityFlag_Moveable))
                        {
                            MoveEntity(GameState, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
                        }

                        Basis->P = GetEntityGroundPoint(Entity);
                    

                        ... we render ... 
                        switch(Entity->Type)
                        {
                            case EntityType_Hero:
                            {
                                ...
                                ...

                                // TODO(casey): Z!!!
                                real32 HeroSizeC = 2.5f;
                                PushBitmap(RenderGroup, &GameState->Shadow, HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));

                                ...
                                ...
                            } break;

                            case EntityType_Wall:
                            {
                                PushBitmap(RenderGroup, &GameState->Tree, 2.5f, V3(0, 0, 0));
                            } break;
                        }

                    }

25:39
proceeds to make the render_group system in the renderer to be more like 3D
so now render_group gets a transform

                struct render_transform
                {
                    // NOTE(casey): Camera parameters
                    real32 MetersToPixels; // NOTE(casey): This translates meters _on the monitor_ into pixels _on the monitor_
                    v2 ScreenCenter;

                    real32 FocalLength;
                    real32 DistanceAboveTarget;

                    v3 OffsetP;
                    real32 Scale;
                };

                struct render_group
                {
                    ...
                    ...
                    render_transform Transform;

                    uint32 MaxPushBufferSize;
                    uint32 PushBufferSize;
                    uint8 *PushBufferBase;
                };

and we calculate the transform when we push bitmaps to our renderer

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    ...
                    ...

                    
                    if(!IsSet(Entity, EntityFlag_Nonspatial) &&
                       IsSet(Entity, EntityFlag_Moveable))
                    {
                        MoveEntity(GameState, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
                    }


                    RenderGroup->Transform.OffsetP = GetEntityGroundPoint(Entity);


                    ... we render ... 
                    switch(Entity->Type)
                    {
                        case EntityType_Hero:
                        {
                            ...
                            ...

                            // TODO(casey): Z!!!
                            real32 HeroSizeC = 2.5f;
                            PushBitmap(RenderGroup, &GameState->Shadow, HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));

                            ...
                            ...
                        } break;

                        case EntityType_Wall:
                        {
                            PushBitmap(RenderGroup, &GameState->Tree, 2.5f, V3(0, 0, 0));
                        } break;
                    }

                }


