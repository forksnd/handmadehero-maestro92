Handmade Hero Day 281 - Animating the Camera Between Rooms

Summary:
wrote logic to animate camera between rooms 


Keyword:
camera animation 

2:14
Casey plans to fix the bug where there is a one glitchy frame when the character switches rooms.
Casey mentions that it seems like the camera update is taking 2 frames to update properly.



5:14
Currently the camera update when room changes is at 
notice we have the if statement, where its either Global_Renderer_Camera_RoomBased, or just a scrolling camera.
Casey mentioned that he will get rid of the scrolling camera logic since we are just gonna do the RoomBased logic

                handmade_sim_region.cpp

                internal void EndSim(sim_region *Region, game_mode_world *WorldMode)
                {
                    
                    entity *Entity = Region->Entities;
                    for(uint32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
                    {
                        ...
                        ...

                        if(Entity->ID.Value == WorldMode->CameraFollowingEntityIndex.Value)
                        {
                            world_position NewCameraP = WorldMode->CameraP;

                            NewCameraP.ChunkZ = EntityP.ChunkZ;

                            if(Global_Renderer_Camera_RoomBased)
                            {
                                if(Entity->P.x > (9.0f))
                                {
    ------------------->            NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(18.0f, 0.0f, 0.0f));
                                }
                                if(Entity->P.x < -(9.0f))
                                {
                                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(-18.0f, 0.0f, 0.0f));
                                }
                                if(Entity->P.y > (5.0f))
                                {
                                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(18.0f, 10.0f, 0.0f));
                                }
                                if(Entity->P.y < -(5.0f))
                                {
                                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(0.0f, -10.0f, 0.0f));
                                }
                            }
                            else
                            {
                                //            real32 CamZOffset = NewCameraP.Offset_.z;
                                NewCameraP = EntityP;
                                //            NewCameraP.Offset_.z = CamZOffset;
                            }

                            WorldMode->CameraP = NewCameraP;
                        }

                        Entity->P += ChunkDelta;
                        Entity->MovementFrom += ChunkDelta;
                        Entity->MovementTo += ChunkDelta;
                        StoreEntityReference(&Entity->Head);
                        PackEntityIntoWorld(World, Entity, EntityP);
                        
                    }
                }

7:05
and very clearly, we see the bug at 

                if(Entity->P.y > (5.0f))
                {
    -------->      NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(18.0f, 10.0f, 0.0f));
                }

which should be 

                if(Entity->P.y > (5.0f))
                {
    -------->      NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(0f, 10.0f, 0.0f));
                }



8:01
Casey wants to implement a smooth transition for the camera when moving between rooms.




11:25
Casey mentioning the few things we want for camera control

so the first Casey wants to mention is "hysteresis". "hysteresis" is an important concept with camera controls, 
especially in 3D, but also important in 2D.

Camera control is sort of the persons perception of where they are in the world.
so in the world of video game, where the player is in the world through a computer screen, you sort of have this 
"perception" problem 

see timestamp at 14:50, Casey refers to an arc in the view of a gamer_s thought process

so camera control is crucial for player not to vomit and have comfortable movement, essentially it affects two things.

-   player_s ability to "project" the world 

-   player_s ability to "control" the character 



20:04
so what "hysteresis" the word means, in programming terms, is "taking into acount previous frames"
another way to say it is, "is not time independent"

so we can make a function, and it can be completely independent of time. and that is what we have right now 

                handmade_sim_region.cpp

                internal void EndSim(sim_region *Region, game_mode_world *WorldMode)
                {
                    
                    entity *Entity = Region->Entities;
                    for(uint32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
                    {
                        ...
                        ...

                        if(Entity->ID.Value == WorldMode->CameraFollowingEntityIndex.Value)
                        {
                            world_position NewCameraP = WorldMode->CameraP;

                            NewCameraP.ChunkZ = EntityP.ChunkZ;

                            if(Global_Renderer_Camera_RoomBased)
                            {
                                if(Entity->P.x > (9.0f))
                                {
                                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(18.0f, 0.0f, 0.0f));
                                }
                                if(Entity->P.x < -(9.0f))
                                {
                                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(-18.0f, 0.0f, 0.0f));
                                }
                                if(Entity->P.y > (5.0f))
                                {
                                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(18.0f, 10.0f, 0.0f));
                                }
                                if(Entity->P.y < -(5.0f))
                                {
                                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(0.0f, -10.0f, 0.0f));
                                }
                            }
                            else
                            {
                                //            real32 CamZOffset = NewCameraP.Offset_.z;
                                NewCameraP = EntityP;
                                //            NewCameraP.Offset_.z = CamZOffset;
                            }

                            WorldMode->CameraP = NewCameraP;
                        }

                        Entity->P += ChunkDelta;
                        Entity->MovementFrom += ChunkDelta;
                        Entity->MovementTo += ChunkDelta;
                        StoreEntityReference(&Entity->Head);
                        PackEntityIntoWorld(World, Entity, EntityP);
                        
                    }
                }

its just dependent on position of the hero and the camera position.
this logic doesnt take into account the path the player has taken. 


so "hysteresis" is about not doing that, its about "lets take into account time", 
"lets take into account the path by which someone has taken to arrive at where hes at, and make our algorithms
aware of that"


26:19
so with our setup in games 

                 _______________________________
                |              #|#              |
                |              #|#              |
                |      A                B       |
                |              #|#              |
                |______________#|#______________|

lets say we have room A and B, the player crosses from room A to room B, we will want the camera to transition from room A to room B
what Casey wants to do is to smoothly vary romo A and room B, and there are a couple of ways to do it. 

so a method without hysteresis, means whereever the player is, it doesnt matter how the player got there, we will be able to say 
where the camera is. 

so what we want to do instead is, as the player transits between the rooms, we take the t value that maps from camera A to B

so imagine like a function that takes A, B and t and produces some kind of interpolation 

                f(A, t, B) ---> interpolation

this can potentially lead to a problem: what if the player just goes back and forth in the corridor. If the player is just hopping 
back and forth, the camera will just be going back and forth like crazy as well. Maybe that is what we want, maybe we dont, but we 
will play with it. 

if we dont want this to happen, that is when we have to think about hysteresis. and what hysteresis will tell us in this situation is 
we have pay attention to what the player have been doing, and then we can figure out whether we want this to happen. 


for example, when the player is going back and forth, maybe we want to bring the camera to a rest, straddling between the two rooms 
until they cross the corridor into room B for more than a second. 

                       ------------------    
                 ______.________________._______
                |      .       #|#      .       |
                |      .       #|#      .       |
                |      A                B       |
                |      .       #|#      .       |
                |______._______#|#______._______|
                       .----------------.



29:23
for now we are just gonna do the direct mapping method, and they way we are gonna do it it is to introduce 
this apron/border region 

                 _______________________
                |   .......D.........   |
                |   .               .   |
                |   .               .   |
                | A .               . B |
                |   .               .   |
                |   .................   |
                |__________C____________|
                
and if you cross into in these areas, (area A, B, C or D); it will start to do the interpolation.


30:20
so the thing we want to do is to make a distinction between where camera is and the simulation region of the camera 
our current code actually does the correct logic for marking the simulation region of the camera 

                if(Entity->ID.Value == WorldMode->CameraFollowingEntityIndex.Value)
                {
                    world_position NewCameraP = WorldMode->CameraP;

                    NewCameraP.ChunkZ = EntityP.ChunkZ;

                    if(Global_Renderer_Camera_RoomBased)
                    {
                        if(Entity->P.x > (9.0f))
                        {
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(18.0f, 0.0f, 0.0f));
                        }
                        if(Entity->P.x < -(9.0f))
                        {
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(-18.0f, 0.0f, 0.0f));
                        }
                        if(Entity->P.y > (5.0f))
                        {
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(18.0f, 10.0f, 0.0f));
                        }
                        if(Entity->P.y < -(5.0f))
                        {
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, V3(0.0f, -10.0f, 0.0f));
                        }
                    }
                    else
                    {
                        //            real32 CamZOffset = NewCameraP.Offset_.z;
                        NewCameraP = EntityP;
                        //            NewCameraP.Offset_.z = CamZOffset;
                    }

                    WorldMode->CameraP = NewCameraP;
                }

                Entity->P += ChunkDelta;
                Entity->MovementFrom += ChunkDelta;
                Entity->MovementTo += ChunkDelta;
                StoreEntityReference(&Entity->Head);
                PackEntityIntoWorld(World, Entity, EntityP);
                

but if we want to animate the camera, [lets say the camera lagging abit] we have to keep track of the camera position

[i think of it as camera destination position, and camera current position]



so we want to implement this in a way that all kinds of camera effects will be easier to implement, such as screen shake 
so Casey added 

                handmade_world_mode.h

                struct game_mode_world
                {
                    ...
                    ...

                    world_position CameraP;
    ----------->    v3 CameraOffset;
                };


                    


37:58
so when we render the world, we render the world at camera_s current position 
                
                handmade_world_mode.h

                internal b32 UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState,
                                     game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
                {
                    ...
                    ...

                    v3 CameraP = Subtract(World, &WorldMode->CameraP, &SimCenterP) + WorldMode->CameraOffset;

                    ...
                    ...
                }



38:34
now we want have to do is to set the new destination camera position when our character walks into the alleyway 
middle ground zone and set the correct CameraOffset

-   as you can see, we first defined a few parameters

                    v3 RoomDelta = {24.0f, 12.5f, 0.0f};
                    v3 hRoomDelta = 0.5f * RoomDelta;
                    r32 ApronSize = 0.7f;
                    v3 hRoomApron = {hRoomDelta.x - ApronSize, hRoomDelta.y - ApronSize, 0.0f};

    RoomDelta is the dimensions of the room 
    hRoomDelta is the half dimensions of the room
    ApronSize is ApronSize, nothing surprizing 
    hRoomApron is half of Apron size 

first thing we do is that, once we walk out of our current room, we change the new destination camera position 
                
                if(Entity->P.x > hRoomDelta.x)      <---- checking if we walked out our current room 
                {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                if(Entity->P.x < -hRoomDelta.x)     <---- checking if we walked out our current room 
                {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                if(Entity->P.y > hRoomDelta.y)      <---- checking if we walked out our current room 
                {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                if(Entity->P.y < -hRoomDelta.y)     <---- checking if we walked out our current room 
                {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }



44:00
so once the player is in the apron region, we want to do some logic 

                                                                
                 ___________________________________ hRoomDelta.y
                |                                           |
                |                EntityP.y                  |
                |        ___________________________  hRoomApron.y
                |       |                           |       |
        

so if the player is in the apron region, then we know that EntityP.y is 

                hRoomApron.y <= EntityP.y <= hRoomDelta.y

we get an interpolation t valule out of it to control the camera offset.
what we will do at first is will go about half way to our final camera destination position, 
then once we enter the next room, the camera will flip the other half 

[so Casey for now only implemented this one case]

                if(Entity->P.y > hRoomApron.y)
                {
                    r32 t = Clamp01MapToRange(hRoomApron.y, EntityP.y, hRoomDelta.y);
                    WorldMode->CameraOffset = V3(0, t * hRoomDelta.y, 0);
                }



as you can see the mapping here is 

                 ___________________________________ hRoomDelta.y
                |                                           |
                |                     A   EntityP.y         |
                |        ___________________________  hRoomApron.y
                |       |                           |       |
        

so when our character is on hRoomApron.y line, that maps to our current room Camera position 

                 _______________
                |               |
                |               |
                |      B        |
                |               |
                |_______________|
                | ............. |
                | .           . |
                | .    A      . |
                | ............. |
                |_______________|
                

then once  EntityP.y equals to hRoomDelta.y maps, then the the camera will be centered on the room border 
                 _______________
                |               |
                |               |
                | .....B....... |
                | .           . |
                |_.___________._|
                | .           . |
                | ............. |
                |      A        |
                |               |
                |_______________|
                
then finally, once the EntityP.y, gets to the other side of the apron position, it will center on room B
hence the mapping. 


so as Casey demonstarted in 47:08, with the code above, what is happening is that our 
when our character is in Room A apron region, the camera pans halfway
once it enter room B apron region, it will snap to the center of room B, since we havent done the mapping 
on the other on the apron area of room B. 
[if you look closely in the video, there is a bit of snapping to room B going on]


                |       |                           |       |
                |       |___________________________|       |
                |                                           |
                |                     B                     |
                |___________________________________ hRoomDelta.y
                |                                           |
                |                     A     EntityP.y       |
                |        ___________________________  hRoomApron.y
                |       |                           |       |
        

so Casey added the other cases


                if(EntityP.y > hRoomApron.y)
                {
                    r32 t = Clamp01MapToRange(hRoomApron.y, EntityP.y, hRoomDelta.y);
                    WorldMode->CameraOffset = V3(0, t * hRoomDelta.y, 0);
                }
                if(EntityP.y < -hRoomApron.y)
                {
                    r32 t = Clamp01MapToRange(-hRoomApron.y, EntityP.y, -hRoomDelta.y);
                    WorldMode->CameraOffset = V3(0, -t * hRoomDelta.y, 0);
                }
                if(EntityP.x > hRoomApron.x)
                {
                    ...
                }
                if(EntityP.x < -hRoomApron.x)
                {
                    ...
                }

                // code from previous parts 
                if(Entity->P.x > hRoomDelta.x)
                {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                if(Entity->P.x < -hRoomDelta.x)
                {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                if(Entity->P.y > hRoomDelta.y)
                {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                if(Entity->P.y < -hRoomDelta.y)
                {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }


so essentially which ever apron area you are in, we animate that camera position. We map the EntityP 
within the apron area to the camera positions inside the room 

so in the video of 48:23, you can see the camera animates nicely. 



49:58
unfortunately, there is still a glitch when we transition between rooms as shown in the video 




52:06
So Casey mentioned that other than the glitch, Case now wants to add some bounce to the camera in the Z dimension

                v3 RoomDelta = {24.0f, 12.5f, 0.0f};
                v3 hRoomDelta = 0.5f * RoomDelta;
                r32 ApronSize = 0.7f;
    -------->   r32 BounceHeight = 0.5f;
                v3 hRoomApron = {hRoomDelta.x - ApronSize, hRoomDelta.y - ApronSize, 0.0f};

the bounce is just gonna be squared t, where we are gonna do a parabolic arc as the camera 
reach the edge of the room 


                if(EntityP.y > hRoomApron.y)
                {
                    r32 t = Clamp01MapToRange(hRoomApron.y, EntityP.y, hRoomDelta.y);
                    WorldMode->CameraOffset = V3(0, t*hRoomDelta.y, (-(t*t)+2.0f*t)*BounceHeight);
                }
                if(EntityP.y < -hRoomApron.y)
                {
                    r32 t = Clamp01MapToRange(-hRoomApron.y, EntityP.y, -hRoomDelta.y);
                    WorldMode->CameraOffset = V3(0, -t*hRoomDelta.y, (-(t*t)+2.0f*t)*BounceHeight);
                }
                if(EntityP.x > hRoomApron.x)
                {
                    r32 t = Clamp01MapToRange(hRoomApron.x, EntityP.x, hRoomDelta.x);
                    WorldMode->CameraOffset = V3(t*hRoomDelta.x, 0, (-(t*t)+2.0f*t)*BounceHeight);
                }
                if(EntityP.x < -hRoomApron.x)
                {
                    r32 t = Clamp01MapToRange(-hRoomApron.x, EntityP.x, -hRoomDelta.x);
                    WorldMode->CameraOffset = V3(-t*hRoomDelta.x, 0, (-(t*t)+2.0f*t)*BounceHeight);
                } 


so now when we move across moves, there is a bounce. 




1:01:44
Casey mentioned that he thinks the glitch is happening because the camera offset is not taking into account 
the NewCameraP. 
[i still dont understand why is there a glitch]

whats happening is because, we set the NewCameraP position, and were still using the offset from the previous frame 
so its way offsetted, then it gets corrected on the next frame

So Casey changed the order of the operation

also we get a new EntityP to figure out what the CameraOffset should be for the next frame.
                  
                v3 EntityP = Entity->P - AppliedDelta; 

so Entity->P is our new position. 

EntityP is the position with respect to our new NewCameraP.


[this may be a bit confusing, but see the following example: 
so assume in one frame, we went from room A to room B, lets say room height is 10
so we went from y = 9 to y = 11 

                 _______________
                |               |
                |               |
                |      B        |
                |               |   
                |_______________|  EntityP = +11 
                |               |  EntityP = +9
                |               |
                |      A        |
                |               |
                |_______________|
                
so when we changed rooms, AppliedDelta = 10 

so the EntityP position with respect to the camera becomes -1 and +9
                 _______________
                |               |
                |               |
                |      B        |
                |               |   
                |_______________|  EntityP = -1 
                |               |  EntityP = +9
                |               |
                |      A        |
                |               |
                |_______________|

recall that when we do the position interpolation, its all in room local coordinates

                r32 t = Clamp01MapToRange(hRoomApron.y, EntityP.y, hRoomDelta.y);

therefore we were seeing the glitch. Essentially we have to do our math in room local coordinates]


-   full code below


                WorldMode->CameraOffset = V3(0, 0, 0);

                v3 AppliedDelta = {};
                if(Entity->P.x > hRoomDelta.x)
                {
                    AppliedDelta = V3(RoomDelta.x, 0.0f, 0.0f);
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                if(Entity->P.x < -hRoomDelta.x)
                {
                    AppliedDelta = V3(-RoomDelta.x, 0.0f, 0.0f);
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                if(Entity->P.y > hRoomDelta.y)
                {
                    AppliedDelta = V3(0.0f, RoomDelta.y, 0.0f);
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                if(Entity->P.y < -hRoomDelta.y)
                {
                    AppliedDelta = V3(0.0f, -RoomDelta.y, 0.0f);
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                }
                
                v3 EntityP = Entity->P - AppliedDelta;

                if(EntityP.y > hRoomApron.y)
                {
                    r32 t = Clamp01MapToRange(hRoomApron.y, EntityP.y, hRoomDelta.y);
                    WorldMode->CameraOffset = V3(0, t * hRoomDelta.y, 0);
                }
                if(EntityP.y < -hRoomApron.y)
                {
                    r32 t = Clamp01MapToRange(-hRoomApron.y, EntityP.y, -hRoomDelta.y);
                    WorldMode->CameraOffset = V3(0, -t * hRoomDelta.y, 0);
                }
                if(EntityP.x > hRoomApron.x)
                {
                    ...
                }
                if(EntityP.x < -hRoomApron.x)
                {
                    ...
                }
