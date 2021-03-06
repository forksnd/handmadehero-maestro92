Handmade Hero Day 078 - Multiple Collision Volumes Per Entity

Summary:

added multiple collisions volumes for each entity.
decoupled entity position with collision volume position

very very briefly mentioned the planned memory layout


Keyword: 
physics collision
memory



5:58
introduced the concept that position != Collision mesh.
Casey emphasized that this is especially true in 3D


Bascially, position is something used for game logic:

AI uses to for pathfinding
or using it for rendering, etc ...




In 3D, we assign a completely separate Collision Mesh for the physics 

or even multiple bounding box for limbs, torso, head, etc..

what we do essentially is that we store a list of all of these collision volume
each is bounding box is stored at a dimension and at some offset relative to your center position


7:15
so the new design behind entity ground point will be that, the ground point for entities will be just on the ground.
Whereas previously, we said the ground point is 1/2 zDim

Then the entity also has a list of offset bounding boxes (as many as he wants);

this design allow us to have multiple collision regions per entity, which is very flexible

so in the case of a entity that looks like below, it will just work

					 _______
					|		|
					|		|
					|		|
				 ___|_______|___
				|	|		|	|
				|	|		|	|
				|___|		|___|


10:21
we store a pointer to sim_entity_collision_volume_group. Right now we put it as a pointer
one advantage of this is that once you defined a collision volume, everyone can share it, so you only
need to allocate memory for one of those. 

				struct sim_entity
				{
				  
					...
					...


				    v3 P;
				    v3 dP;
				    
				    real32 DistanceLimit;

    				sim_entity_collision_volume_group *Collision;

    				...
    				...

				};



13:11
we wrote the struct for sim_entity_collision_volume_group

note that the sim_entity_collision_volume has this OffsetP variable.
that is with respect to sim_entity.P position.




				struct sim_entity_collision_volume
				{
				    v3 OffsetP;
				    v3 Dim;
				};


				struct sim_entity_collision_volume_group
				{
				    sim_entity_collision_volume TotalVolume;

				    // TODO(casey): VolumeCount is always expected to be greater than 0 if the entity
				    // has any volume... in the future, this could be compressed if necessary to say
				    // that the VolumeCount can be 0 if the TotalVolume should be used as the only
				    // collision volume for the entity.
				    uint32 VolumeCount;
				    sim_entity_collision_volume *Volumes;
				};


22:07
we added the 
				
				sim_entity_collision_volume TotalVolume;

to the sim_entity_collision_volume_group. this is the volume that encloses everything.
so if the VolumeCount is zero, that means there is not any more complex shape. If it is above zero
than we want to do sub-testing, testing with more complicated shapes 

sometimes we will just use the TotalVolume for initial collsiion detection so it is cheaper




15:39
now that we made the engineering decision to make the entity ground point a true ground point,
we refactored the GetEntityGroundPoint(); function

previously it was


				handmade_entity.h

				inline v3
				GetEntityGroundPoint(sim_entity *Entity)
				{
				    v3 Result = Entity->P + V3(0, 0, -0.5f*Entity->Dim.Z);

				    return(Result);
				}

now it is 

				handmade_entity.h

				inline v3
				GetEntityGroundPoint(sim_entity *Entity)
				{
				    v3 Result = Entity->P;

				    return(Result);
				}








15:45
we do the same with GetStairsGround();

previously we have 

				inline real32
				GetStairGround(sim_entity *Entity, v3 AtGroundPoint)
				{
				    Assert(Entity->Type == EntityType_Stairwell);
				    
				    rectangle3 RegionRect = RectCenterDim(Entity->P, Entity->Dim);
				    v3 Bary = Clamp01(GetBarycentric(RegionRect, AtGroundPoint));
				    real32 Result = RegionRect.Min.Z + Bary.Y*Entity->WalkableHeight;

				    return(Result);
				}

now we have this. The idea is that the ground point of a stairwell depends on where you are in the stairwell rect,
what means which decide z from the xy coordinate

		y axis

			^
			|
			|
			|	 _______
			|	|_______|	^
			|	|_______|	|
			|	|_______|	|
			|	|_______|	|		going up stairs this way
			|	|_______|  	|
			|
			---------------------------->	x axis


the higher y you have, the higher up you are on the ladder. so we first get the BaryCentric coordinate of y
then we calculate z from it


				inline real32
				GetStairGround(sim_entity *Entity, v3 AtGroundPoint)
				{
				    Assert(Entity->Type == EntityType_Stairwell);
				    
				    rectangle2 RegionRect = RectCenterDim(Entity->P.XY, Entity->WalkableDim);
				    v2 Bary = Clamp01(GetBarycentric(RegionRect, AtGroundPoint.XY));
				    real32 Result = Entity->P.Z + Bary.Y*Entity->WalkableHeight;

				    return(Result);
				}







27:31

in the MoveEntity code, we change it so that we are looping and testing all the Collision volume

note that you have two entities, each with a list of collision volumes. so this is essentially two loops, an N^2 problem


so what we do alot of times is that we eliminate number of test pairs in physics collision logic
either through spatial partitioning or early out and so on


				internal void
				MoveEntity(game_state *GameState, sim_region *SimRegion, sim_entity *Entity, real32 dt,
				           move_spec *MoveSpec, v3 ddP)
				{

					...
					...

				    for(uint32 Iteration = 0; Iteration < 4; ++Iteration)
				    {

				    	...
				    	...

		                for(uint32 TestHighEntityIndex = 0; TestHighEntityIndex < SimRegion->EntityCount; ++TestHighEntityIndex)
		                {

		                	...
		                	...

	                        for(uint32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; ++VolumeIndex)
	                        {                      
	                            sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;
	                            
	                            ...
	                            ...


	                            for(uint32 TestVolumeIndex = 0; TestVolumeIndex < TestEntity->Collision->VolumeCount; ++TestVolumeIndex)
	                            {
	                                sim_entity_collision_volume *TestVolume = TestEntity->Collision->Volumes + TestVolumeIndex;

	                                v3 MinkowskiDiameter = {TestVolume->Dim.X + Volume->Dim.X,
	                                                        TestVolume->Dim.Y + Volume->Dim.Y,
	                                                        TestVolume->Dim.Z + Volume->Dim.Z};


	                                ...
	                                ...

	                                testing logic

								}

							}
						}

				    }
				}


38:11
we define all the types of sim_entity_collision_volume_group for people to use

notice that we defined the NullCollision. that is for anything that does not want to collide at all

				handmade.h

				struct game_state
				{
				    memory_arena WorldArena;
				    world *World;

				    ...
				    ...

				    // TODO(casey): Must be power of two
				    pairwise_collision_rule *CollisionRuleHash[256];
				    pairwise_collision_rule *FirstFreeCollisionRule;

				    sim_entity_collision_volume_group *NullCollision;
				    sim_entity_collision_volume_group *SwordCollision;
				    sim_entity_collision_volume_group *StairCollision;
				    sim_entity_collision_volume_group *PlayerCollision;
				    sim_entity_collision_volume_group *MonstarCollision;
				    sim_entity_collision_volume_group *FamiliarCollision;
				    sim_entity_collision_volume_group *WallCollision;
				};




39:01

then we do the initialization here 
				
				handmade.cpp


				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{    

					...
					...

				    
				    game_state *GameState = (game_state *)Memory->PermanentStorage;
				    if(!Memory->IsInitialized)
				    {

				    	...
				    	...


				        GameState->NullCollision = MakeNullCollision(GameState);
				        GameState->SwordCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.1f);
				        GameState->StairCollision = MakeSimpleGroundedCollision(GameState,
				                                                                GameState->World->TileSideInMeters,
				                                                                2.0f*GameState->World->TileSideInMeters,
				                                                                1.1f*GameState->World->TileDepthInMeters);
				        GameState->PlayerCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 1.2f);
				        GameState->MonstarCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.5f);
				        GameState->FamiliarCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.5f);
				        GameState->WallCollision = MakeSimpleGroundedCollision(GameState,
				                                                               GameState->World->TileSideInMeters,
				                                                               GameState->World->TileSideInMeters,
				                                                               GameState->World->TileDepthInMeters);

				        



50:07
for now we are double storing the collision volume. 
once as the totalVolume,
twice as the first index in the list of sub Volumes 

				handmade.cpp

				sim_entity_collision_volume_group *
				MakeSimpleGroundedCollision(game_state *GameState, real32 DimX, real32 DimY, real32 DimZ)
				{
				    // TODO(casey): NOT WORLD ARENA!  Change to using the fundamental types arena, etc.
				    sim_entity_collision_volume_group *Group = PushStruct(&GameState->WorldArena, sim_entity_collision_volume_group);
				    Group->VolumeCount = 1;
				    Group->Volumes = PushArray(&GameState->WorldArena, Group->VolumeCount, sim_entity_collision_volume);
				    Group->TotalVolume.OffsetP = V3(0, 0, 0.5f*DimZ);
				    Group->TotalVolume.Dim = V3(DimX, DimY, DimZ);
				    Group->Volumes[0] = Group->TotalVolume;

				    return(Group);
				}






59:03
wrote the MakeNullCollision() function
we made so that in game logic, people do not have to always check if this guys collision volume group is null all the time

				handmade.cpp

				sim_entity_collision_volume_group * MakeNullCollision(game_state *GameState)
				{
				    // TODO(casey): NOT WORLD ARENA!  Change to using the fundamental types arena, etc.
				    sim_entity_collision_volume_group *Group = PushStruct(&GameState->WorldArena, sim_entity_collision_volume_group);
				    Group->VolumeCount = 0;
				    Group->Volumes = 0;
				    Group->TotalVolume.OffsetP = V3(0, 0, 0);
				    // TODO(casey): Should this be negative?
				    Group->TotalVolume.Dim = V3(0, 0, 0);

				    return(Group);
				}



1:03:51

in the memory, we plan to have the following layout 
more on this later
	
				 ___________________	
				|					|
				|	constant		|
				|	stuff			|
				|					|
				|___________________|	
				|					|
				|	World 			|
				|	stuff			|
				|					|
				|___________________|	
				|					|
				|					|
				|					|
				|					|
				|___________________|	
