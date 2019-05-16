Handmade Hero Day 285 - Transactional Occupation of Traversables

Summary:
added more logic on whether an entity can occpy a traversable point or not, or now will multiple 
entities contest for a traversable point

[didnt study the code very deep cuz this is just very game specific code, not architectural dicussions,
which is what I want to learn the most] 

Keyword:
movement system


8:11
Casey changing the entity_traversable_point to store Pointer 

				handmade_entity.h

				struct entity_traversable_point
				{
				    v3 P;
				    entity *Occupier;
				};


26:39
Casey wrote the TransactionalOccupy function that checks whether an entity can occupy a traversable_reference or not


				handmade_sim_region.cpp

				internal b32 TransactionalOccupy(entity *Entity, traversable_reference *DestRef, traversable_reference DesiredRef)
				{
				    b32 Result = false;
				    
				    entity_traversable_point *Desired = GetTraversable(DesiredRef);
				    if(!Desired->Occupier)
				    {
				        entity_traversable_point *Dest = GetTraversable(*DestRef);
				        if(Dest)
				        {
				            Dest->Occupier = 0;
				        }
				        *DestRef = DesiredRef;
				        Desired->Occupier = Entity;
				        Result = true;
				    }
				    
				    return(Result);
				}




54:13
see video at this timestamp. With the new movement system, people cant occupy the same traversable point 



1:04:47
Casey solving the problem of dropping multiple chracters people on the same traversable point