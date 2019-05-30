Handmade Hero Day 290 - Finishing Separated Rendering

Summary:
changed Hero and Familiar to use the entity_visible_piece rendering path.

added PieceMove_AxesDeform and PieceMove_BobOffset flags to entity_visible_piece so that we can animate them.

preceeds to remove all entityType dependent logic from the game

Keyword:
entity system 


1:43
Casey mentioned a bug that is currently lurking in our entity system 

so currently in our BeginSim(); function, we load Chunks in, and we pull in all the entities from them,
and then we write the entities back out to the chunks. 

recall we currently have something like this:
                
                for (every .... Chunk .....)
                {
                    ...
                    ...

                    for(uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex)
                    {                        
                        entity *Source = (entity *)Block->EntityData + EntityIndex;
                        v3 SimSpaceP = Source->P + ChunkDelta;
    ---------------->   if(EntityOverlapsRectangle(SimSpaceP, Source->Collision->TotalVolume, SimRegion->Bounds))
                        {
                            ...
                            ...
                        }
                    }

                    ...
                    ...
                }

and we are doing this EntityOverlapsRectangle(); thing. which means some entities that fail this check
wont get streamed out. So in the end they will get silently deleted.
     _______________                                                                                      _______________
    |               |                                                                                    |               |
    |  A            |             A fails the test,                 simulate B and C,                    |               |
    |     B         |  ------->   only stream out B, C  -------->   put it back in the chunk ----------> |     B         |
    |          C    |                                                                                    |         C     |
    |_______________|                                                                                    |_______________|


Obviously, we dont want this to happen, so we just get rid of this EntityOverlapsRectangle(); check


15:10
So As Casey reorganizes the rendering code to use entity_visible_pieces instead of by entity types,
Casey mentions that the hard ones are actually case EntityType_HeroBody, and case EntityType_Familiar.
Because these two actually has some animation to it.


for example, the hero head had to bob up and down.
what we can do here is to create an infrastructure for when people move, we have a certain understanding 
of what their parameters of motions are, 

its kind of like skeletal animation, and here we are trying to systemize 
so in AddPlayer(); we add the separate pieces 


                internal void AddPlayer(game_mode_world *WorldMode, sim_region *SimRegion, traversable_reference StandingOn,
                          brain_id BrainID)
                {
                    ...
                    ...

                    entity_id Result = Head->ID;

                    v4 Color = {1, 1, 1, 1};
                    real32 HeroSizeC = 3.0f;
                    AddPiece(Body, Asset_Shadow, HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                    AddPiece(Body, Asset_Torso, HeroSizeC*1.2f, V3(0, 0.0f, -0.002f), Color); 
                    AddPiece(Body, Asset_Cape, HeroSizeC*1.2f, V3(0, -0.1f, -0.001f), Color);

                    AddPiece(Head, Asset_Head, HeroSizeC*1.2f, V3(0, -0.7f, 0), Color);

                    ...
                    ...
                }

23:24
so now to give the entity_visible_pieces a notion of animation, Case created an enum

    
                handmade_entity.h

                enum entity_visible_piece_flag
                {
                    PieceMove_AxesDeform = 0x1,
                    PieceMove_BobOffset = 0x2,
                };


24:40
so for the hero, it will have a bobbing head and a bobbing torso 

                internal void AddPlayer(game_mode_world *WorldMode, sim_region *SimRegion, traversable_reference StandingOn,
                          brain_id BrainID)
                {
                    ...
                    ...

                    entity_id Result = Head->ID;

                    v4 Color = {1, 1, 1, 1};
                    real32 HeroSizeC = 3.0f;
                    AddPiece(Body, Asset_Shadow, HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                    AddPiece(Body, Asset_Torso, HeroSizeC*1.2f, V3(0, 0.0f, -0.002f), Color, PieceMove_AxesDeform); 
                    AddPiece(Body, Asset_Cape, HeroSizeC*1.2f, V3(0, -0.1f, -0.001f), Color, PieceMove_AxesDeform|PieceMove_BobOffset);

                    AddPiece(Head, Asset_Head, HeroSizeC*1.2f, V3(0, -0.7f, 0), Color);

                    ...
                    ...
                }

and Casey also added a movementFlags on the entity_visible_piece

                handmade_entity.h

                struct entity_visible_piece
                {
                    v4 Color;
                    v3 Offset;
                    asset_type_id AssetType;
                    r32 Height;
    ----------->    u32 Flags;
                };


26:40
then in the rendering loop, we check if the entity_visible_piece has either of these flags 

                for(u32 PieceIndex = 0; PieceIndex < Entity->PieceCount; ++PieceIndex)
                {
                    entity_visible_piece *Piece = Entity->Pieces + PieceIndex;
                    bitmap_id BitmapID = 
                        GetBestMatchBitmapFrom(TranState->Assets, 
                                               Piece->AssetType, &MatchVector, &WeightVector);

                    v2 XAxis = {1, 0};
                    v2 YAxis = {0, 1};
                    if(Piece->Flags & PieceMove_AxesDeform)
                    {
                        XAxis = Entity->XAxis;
                        YAxis = Entity->YAxis;
                    }
                   
                    r32 tBob = 0.0f;
                    v3 Offset = {};
                    if(Piece->Flags & PieceMove_BobOffset)
                    {
                        tBob = Entity->tBob;
                        Offset = V3(Entity->FloorDisplace, 0.0f);
                        Offset.y += Entity->tBob;
                    }

                    PushBitmap(RenderGroup, EntityTransform, BitmapID, Piece->Height, Offset + Piece->Offset, Piece->Color, 1.0f, XAxis, YAxis);
                }



so for example, in the PieceMove_BobOffset motion, we have  

                r32 tBob = 0.0f;
                v3 Offset = {};
                if(Piece->Flags & PieceMove_BobOffset)
                {
                    tBob = Entity->tBob;
    ----------->    Offset = V3(Entity->FloorDisplace, 0.0f);
    ----------->    Offset.y += Entity->tBob;
                }

the Entity->FloorDisplace and Entity->tBob were calcuated in the Type_brain_hero controller logic 
so the flow looks like 

                Type_brain_hero controller logic 
                    updates Entity->tBob and Entity->FloorDisplace

                rendering logic:
                    do rendering with Entity->tBob and Entity->FloorDisplace


52:17
Casey mentioned that we can make the BrainSlotFor, a more extensive macro set, that can reduce our chances
of making an assignment error.
currently we have: 

                #define BrainSlotFor(type, Member) BrainSlotFor_(Type_##type, &(((type *)0)->Member) - (entity **)0)
                inline brain_slot
                BrainSlotFor_(brain_type Type, u16 PackValue)
                {
                    brain_slot Result = {(u16)Type, PackValue};

                    return(Result);
                }

Casey made some changes so that brain_slot has both Type and Index

                enum brain_type
                {
                    Type_brain_hero,

                    // NOTE(casey): Test brains!
                    Type_brain_snake,
                    Type_brain_familiar,
                    Type_brain_floaty_thing_for_now,
                    Type_brain_monstar,

                    Type_brain_count,
                };

                struct brain_slot 
                {
                    u16 Type;
                    u16 Index;
                };

Q/A
1:10:20
someone asked how to take control of a nearby entity with our current architecture?

Casey demonstrated added the brain swapping code in the 
essentially the tricky part is that we swap the brain on the entities 

                inline void ExecuteBrain(game_state *GameState, game_input *Input, 
                             sim_region *SimRegion, brain *Brain, r32 dt)
                {
                    switch(Brain->Type)
                    {
                        case Type_brain_hero:
                        {
                            ...
                            ...

                            if(Head && WasPressed(Controller->Start))
                            {
                                entity *ClosestHero = 0;
                                real32 ClosestHeroDSq = Square(10.0f); // NOTE(casey): Ten meter maximum search!
                                entity *TestEntity = SimRegion->Entities;
                                for(uint32 TestEntityIndex = 0;
                                    TestEntityIndex < SimRegion->EntityCount;
                                    ++TestEntityIndex, ++TestEntity)
                                {
                                    if((TestEntity->BrainID.Value != Head->BrainID.Value) &&
                                       TestEntity->BrainID.Value)
                                    {            
                                        real32 TestDSq = LengthSq(TestEntity->P - Head->P);            
                                        if(ClosestHeroDSq > TestDSq)
                                        {
                                            ClosestHero = TestEntity;
                                            ClosestHeroDSq = TestDSq;
                                        }
                                    }
                                }
                                
                                if(ClosestHero)
                                {
    ----------------------->        brain_id OldBrainID = Head->BrainID;
                                    brain_slot OldBrainSlot = Head->BrainSlot;
                                    Head->BrainID = ClosestHero->BrainID;
                                    Head->BrainSlot = ClosestHero->BrainSlot;
                                    ClosestHero->BrainID = OldBrainID;
                                    ClosestHero->BrainSlot = OldBrainSlot;
                                }
                            }
                            
                            ...
                            ...
                        }
                    }
                }