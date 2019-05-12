Handmade Hero Day 282 - Z Movement and Camera Motion

Summary:
added Z axis considerations in the motion code

Keyword:
code clean up

16:21
Casey adding checks for z in the camera motion code 

                if(Global_Renderer_Camera_RoomBased)
                {
                    WorldMode->CameraOffset = V3(0, 0, 0);

                    v3 AppliedDelta = {};
                    for(u32 E = 0;
                        E < 3;
                        ++E)
                    {
                        if(Entity->P.E[E] > hRoomDelta.E[E])
                        {
                            AppliedDelta.E[E] = RoomDelta.E[E];
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                        }
                        if(Entity->P.E[E] < -hRoomDelta.E[E])
                        {
                            AppliedDelta.E[E] = -RoomDelta.E[E];
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                        }
                    }
                    
                    v3 EntityP = Entity->P - AppliedDelta;

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
                    if(EntityP.z > hRoomApron.z)
                    {
                        r32 t = Clamp01MapToRange(hRoomApron.z, EntityP.z, hRoomDelta.z);
                        WorldMode->CameraOffset = V3(0, 0, t*hRoomDelta.z);
                    }
                    if(EntityP.z < -hRoomApron.z)
                    {
                        r32 t = Clamp01MapToRange(-hRoomApron.z, EntityP.z, -hRoomDelta.z);
                        WorldMode->CameraOffset = V3(0, 0, -t*hRoomDelta.z);
                    }
                }


33:29
Casey discusses his plans to finalize the definition of z in this game. 

Casey plans to have two concepts of z in this game: "floor Z" and "Actual Z"

the "Floor Z" is something that creates perspective 

the "Actual Z" is the actual Z in the world. This will orthographic. 

previously when we render bit maps, we have "Transform" and "Offset"
the "Transform" part is perspective z, the "Offset" is orthographic z


Q/A
1:04:55
you dont want to add a depth buffer in your rendering for no reason, because that doubles your read/write bandwidth


