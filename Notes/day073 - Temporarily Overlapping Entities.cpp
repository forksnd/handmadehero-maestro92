Handmade Hero Day 073 - Temporarily Overlapping Entities

Summary:
introduces the concept of inside/outside into the collision system

depending on whether you are inside/outside, the collision system has different logic whether entity A is "entering" or "exiting"
entity B.

Wrote two ways to check if Rectangles Intersect: the Minkowski way and the formal AABB way

plans to refactor the collision rules system
introduces the concept of permenant and transient collision rules 

Keyword: 
Physics, Collision Detection, Collision Handling


10:11
added stairwells backinto the game as entities



18:45
introduces the concept of inside and oustide in the collision system.
This is needed cuz
1.	game logic behaviour
	example: when character touches the stairwell entity, only trigger the going downstairs logic when it overlaps with the bottom 1/3 of the entity

2.	robustness
	example: when character collides with a wall, we currently tries to displace you by the intepenetration amount.
	however, due to floating point situations, it is possible to get "inside" wall. so we need to 
	prevent that from happening

	once we have this inside/outside concept, even if we are inside an entity, we can project the character
	 to the closest point outside the wall





23:40
added a section in the MoveEntity code which tests whether you are touching an entity or not
we record an array of overlapping entities

-	we first get the rectangles that bound both entity and test entity

				rectangle3 EntityRect = RectCenterDim(Entity->P, Entity->Dim) 

				rectangle3 TestEntityRect = RectCenterDim(TestEntity->P, TestEntity->Dim);

-	then we call the function 

				RectanglesIntersect(EntityRect, TestEntityRect)



full code is below

				handmade_sim_region.cpp

				internal void
				MoveEntity(game_state *GameState, sim_region *SimRegion, sim_entity *Entity, real32 dt, move_spec *MoveSpec, v3 ddP)
				{
				    ...

				    v3 NewPlayerP = OldPlayerP + PlayerDelta;
				    
				    ...

				    // NOTE(casey): Check for initial inclusion
				    uint32 OverlappingCount = 0;
				    sim_entity *OverlappingEntities[16];
				    {
				        rectangle3 EntityRect = RectCenterDim(Entity->P, Entity->Dim); 

				        // TODO(casey): Spatial partition here!
				        for(uint32 TestHighEntityIndex = 0; TestHighEntityIndex < SimRegion->EntityCount; ++TestHighEntityIndex)
				        {
				            sim_entity *TestEntity = SimRegion->Entities + TestHighEntityIndex;
				            if(ShouldCollide(GameState, Entity, TestEntity))
				            {
				                rectangle3 TestEntityRect = RectCenterDim(TestEntity->P, TestEntity->Dim);
				                if(RectanglesIntersect(EntityRect, TestEntityRect))
				                {
				                    if(OverlappingCount < ArrayCount(OverlappingEntities))
				                    {
			                            OverlappingEntities[OverlappingCount++] = TestEntity;
				                    }
				                    else
				                    {
				                        InvalidCodePath;
				                    }
				                }
				            }
				        }    

				    }
				    

				    for(uint32 Iteration = 0; Iteration < 4; ++Iteration)
				    {
						...
						...
				    }    

				    ...
				    ...
				}






30:08
Now then we have this list of overlapping entities, within the movement iteration loop, 
we always keep track whether we are "entering" or "exiting" an entity

we first check if the HitEntity was already in the overlapping entity list, this tells us if we were previously overlapping
this WasOverlapping flag tells us whether we are "entering" or "exiting"

-	in the HandleCollsion function, we are passing the "WasOverlapping" to function.
the idea is that HandleCollsion function will handle the entity collision depending whether you are "entering" or "exiting"

-	in the StopsOnCollision if condition, if we are supposed to "StopsOnCollision", the logic prevents you from entering that entity

-	then in the else condition, 
	if we were previously overlapping, that means we are exiting this entity
			hence we decrement the overlappingEntities array 

	else if we were not overlapping, that means we are entering this entity
			then we add it to the overlappingEntities array




			    for(uint32 Iteration = 0; Iteration < 4; ++Iteration)
			    {
			       	...
			       	...

			               
		            Entity->P += tMin*PlayerDelta;
		            DistanceRemaining -= tMin*PlayerDeltaLength;            
		            if(HitEntity)
		            {
		                PlayerDelta = DesiredPosition - Entity->P;                
		                
		                uint32 OverlapIndex = OverlappingCount;
		                for(uint32 TestOverlapIndex = 0; TestOverlapIndex < OverlappingCount; ++TestOverlapIndex)
		                {
		                    if(HitEntity == OverlappingEntities[TestOverlapIndex])
		                    {
		                        OverlapIndex = TestOverlapIndex;
		                        break;
		                    }
		                }

		                bool32 WasOverlapping = (OverlapIndex != OverlappingCount);
		                bool32 StopsOnCollision = HandleCollision(GameState, Entity, HitEntity, WasOverlapping);
		                if(StopsOnCollision)
		                {
		                    PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
		                    Entity->dP = Entity->dP - 1*Inner(Entity->dP, WallNormal)*WallNormal;
		                }
		                else
		                {
		                    if(WasOverlapping)
		                    {
		                        OverlappingEntities[OverlapIndex] = OverlappingEntities[--OverlappingCount];
		                    }
		                    else if(OverlappingCount < ArrayCount(OverlappingEntities))
		                    {
		                        OverlappingEntities[OverlappingCount++] = HitEntity;
		                    }
		                    else
		                    {
		                        InvalidCodePath;
		                    }
		                }
		            }
		            else
		            {
		                break;
		            }
			    }    




42:06
he writes the RectanglesIntersect function.
so there are two ways to check whether two RectanglesIntersect

1.	the Minkowski method

				inline bool32
				RectanglesIntersect(v3 P, v3 Dim, rectangle3 Rect)
				{
				    rectangle3 Grown = AddRadiusTo(Rect, 0.5f*Dim);
				    bool32 Result = IsInRectangle(Grown, P);
				    return(Result);
				}


2.	the AABB way, the more formal way:
you just think about it one axis at a time.
the way to think about is that, if two rectangles intersect, they must have one point in common
and if they have one point in common, then they must overlapp in all three dimensions
if any of the dimensions have a gap, they do not overlap

				handmade_math.h

				inline bool32
				RectanglesIntersect(rectangle3 A, rectangle3 B)
				{
				    bool32 Result = !((B.Max.X < A.Min.X) ||
				                      (B.Min.X > A.Max.X) ||
				                      (B.Max.Y < A.Min.Y) ||
				                      (B.Min.Y > A.Max.Y) ||
				                      (B.Max.Z < A.Min.Z) ||
				                      (B.Min.Z > A.Max.Z));
				    return(Result);
				}

these are only true if they are AABB




1:01:10 
the concept behind the collision rules refactor

inside the movement logic, we put the collision rules transiently
then when we are done, we remove them

that gets rid of the lifetime management problem 

by adding the entry/exit system in the physics, that gives us the trigger
to create and delete the collision rules. 

this will be finished up on day 074






1:05:45
the way to think about "overlap testing" is "Separating Axis Thererom"

for any two shapes if they do not overlap, you will be able to find some axis in which they do not overlap



