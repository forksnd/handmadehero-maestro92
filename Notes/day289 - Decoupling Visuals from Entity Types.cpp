Handmade Hero Day 289 - Decoupling Visuals from Entity Types

Summary:
reorganized the code a bit. moved all the brain related logic to handmade_brain.h and handmade_brain.cpp

started the idea of having entity_visible_piece in the rendering path so that we dont have to do rendering logic 
by entity_types.

Keyword:
entity system 



5:00
moved all the brain related code to handmade_brain.h


Casey also made it so that in our main tick function our code looks like 

                
    
                // NOTE(casey): Run all brains
                for(u32 BrainIndex = 0; BrainIndex < SimRegion->BrainCount; ++BrainIndex)
                {
                    brain *Brain = SimRegion->Brains + BrainIndex;
                    ExecuteBrain(GameState, Input, SimRegion, Brain, dt);
                }

                for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
                {
                    entity *Entity = SimRegion->Entities + EntityIndex;

                    ...
                    ...
                }

6:45
Casey also moved all the brain related logic to the ExecuteBrain(); function

                handmade_brain.cpp

                inline void ExecuteBrain(game_state *GameState, game_input *Input, 
                             sim_region *SimRegion, brain *Brain, r32 dt)
                {
                  
                    ...
                    ...
                }



22:45
Casey mentions that some entity should persist, some shouldnt. So Casey clears data that are not supposed to persist 

                handmade_world.cpp 

                internal void PackEntityIntoChunk(world *World, sim_region *SimRegion, entity *Source, world_chunk *Chunk)
                {
                    ...
                    ...
                    
                    DestE->ddP = V3(0, 0, 0);
                    DestE->ddtBob = 0.0f;
                }


40:02
Casey mentions that he now wants to find a way to get rid of the entity type in the rendering code 
since we are currently doing 

                switch(Entity->Type)
                {
                    case type1:
                        render type1;
                        break;
                    case type2:
                        render type2;
                        break;

                    ...
                    ...
                }

what Casey wants is to have the entity itself to say that it is made out of certain pieces.
And so when the entity is created, it can then be created out of these pieces. 



46:07
so Casey added the entity_visible_piece struct 
                
                handmade_entity.h

                struct entity_visible_piece
                {
                    v4 Color;
                    asset_type_id AssetType;
                    r32 Height;
                };






51:45
Casey also added an array of that in the entity struct 

                handmade_entity.h

                struct entity
                {
                    ...
                    ...

                    u32 PieceCount;
                    entity_visible_piece Pieces[4];
                    
                    // TODO(casey): Generation index so we know how "up to date" this entity is.
                };



Then Casey added the AddPiece Function
                
                handmade_world_mode.cpp

                internal void AddPiece(entity *Entity, asset_type_id AssetType, r32 Height, v4 Color)
                {
                    Assert(Entity->PieceCount < ArrayCount(Entity->Pieces));
                    entity_visible_piece *Piece = Entity->Pieces + Entity->PieceCount++;
                    Piece->AssetType = AssetType;
                    Piece->Height = Height;
                    Piece->Color = Color;
                }




then as an example, in the AddWall(); function, we call AddPiece();

                handmade_world_mode.cpp

                internal void AddWall(game_mode_world *WorldMode, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
                {
                    world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX, AbsTileY, AbsTileZ);
                    entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Wall,
                        WorldMode->WallCollision);
                    AddFlags(Entity, EntityFlag_Collides);

    ----------->    AddPiece(Entity, Asset_Tree, 2.5f, V4(1, 1, 1, 1));
                    
                    EndEntity(WorldMode, Entity, P);
                }


so now as an example of slowly migrating to rendering using the entity_visible_pieces system, we have the following code

            real32 HeroSizeC = 3.0f;
            switch(Entity->Type)
            {
                case EntityType_HeroBody:
                {
                    ...
                } break;

                case EntityType_HeroHead:
                {                    
                    ...
                } break;

                ...
                ...
            }

            for(u32 PieceIndex = 0;
                PieceIndex < Entity->PieceCount;
                ++PieceIndex)
            {
                entity_visible_piece *Piece = Entity->Pieces + PieceIndex;
                bitmap_id BitmapID = 
                    GetBestMatchBitmapFrom(TranState->Assets, 
                                           Piece->AssetType, &MatchVector, &WeightVector);
                PushBitmap(RenderGroup, EntityTransform, BitmapID, Piece->Height, V3(0, 0, 0), Piece->Color);
            }

Q/A
1:13:39
Casey mentioned the flexibility of this new system
now you can miss and match and nothing has to be tied to the entity type.
you can now have something that controls like a hero, but renders like a tree. 