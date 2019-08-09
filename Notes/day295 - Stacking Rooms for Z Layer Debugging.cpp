Handmade Hero Day 295 - Stacking Rooms for Z Layer Debugging

Summary:
did some simple profiling and optimization in various code


Keyword:
profiling


Casey doing a lot of profiling



46:54
Casey added a cumulative percentage in the profiler view for each function


1:00:57
Casey discussing the things we can optimize 

1:01:17
Casey mentioned that brifely estimated the things we can do to optimize

"SimulateEntities"
definitely we can otpimize

"GetClosesTraversable" 
is a spatial query and we can optimize that

"OpenGLRenderCommands"
not so sure



1:03:40
Casey analyzing what is taking so long in "beginSim"

espeically, Casey is investigating what we are doing in BeginSim(); especially these few lines

                handmade_sim_region.cpp

                internal sim_region* BeginSim()
                {

                    sim_region *SimRegion = PushStruct(SimArena, sim_region);

                    ...
                    ...

                    SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, entity);

                }

in which we are pushing a sim_region or entity array onto our memory arena during our initalization code



1:04:01
so initially, the sim_region struct had this array

                

                struct sim_region
                {
                    // TODO(casey): Need a hash table here to map stored entity indices
                    // to sim entities!
                    
                    world *World;
                    r32 MaxEntityRadius;
                    r32 MaxEntityVelocity;

                    world_position Origin;
                    rectangle3 Bounds;
                    rectangle3 UpdatableBounds;
                    
                    u32 MaxEntityCount;
                    u32 EntityCount;
                    entity *Entities;
                    
                    u32 MaxBrainCount;
                    u32 BrainCount;
                    brain *Brains;
                    
                    // TODO(casey): Do I really want a hash for this??
                    // NOTE(casey): Must be a power of two!
    ------------>   entity_hash EntityHash[4096];
                    brain_hash BrainHash[256];
                };

casey changed it to 


                internal sim_region* BeginSim()
                {
                    SimRegion->MaxEntityCount = 4096;
                    SimRegion->EntityCount = 0;
    ----------->    SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, entity, NoClear());

                }



1:06:45
Casey now looking at "SimulateEntities", which is still expensive


1:08:32
Casey showed that "EntityRendering" is taking a long time, the rest of the operations are evenly scattered


1:12:26
Casey found out that "CollisionRendering" takes all the time


1:14:31 
Casey then looks at PushRenderElement_ function
he does mention that this is a very straight forward code which contains a lot of operation that will benefit a lot from
running the optimizer on it


Q/A
1:25:22
will LLVM work on windows handmade hero?

No 