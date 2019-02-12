Handmade Hero Day 205 - Picking Entities with the Mouse

Summary:
cleaned up the project and Unproject function
discussing how z works in its project and Unproject function

discussed further of what is the problem of our z in Q/A

Keyword:
project and Unproject




10:25
Casey finalizing the new Unproject function

for questions on how GetRenderEntityBasisP(); work, refer to day 108 


The Unproject function here is just the reverse of GetRenderEntityBasisP();

                handmade_render.cpp

                inline v3 Unproject(render_group *Group, v2 PixelsXY)
                {
                    render_transform *Transform = &Group->Transform;
                    
                    v2 UnprojectedXY;
                    if(Transform->Orthographic)
                    {
                        UnprojectedXY = (1.0f / Transform->MetersToPixels)*(PixelsXY - Transform->ScreenCenter);
                    }
                    else
                    {
                        v2 A = (PixelsXY - Transform->ScreenCenter) * (1.0f / Transform->MetersToPixels);
                        UnprojectedXY = ((Transform->DistanceAboveTarget - Transform->OffsetP.z)/Transform->FocalLength) * A; 
                    }

                    v3 Result = V3(UnprojectedXY, Transform->OffsetP.z);
                    Result -= Transform->OffsetP;

                    return(Result);
                }


so just to recap 

so for the Orthographic 


                 P                 |
                    o--------------o P_
                                   |        
                                   |                c (camera)
                -------------------|-------------------o---------------> Z
                                   |
                                   |      d (monitor to camera)       
                                   |
                                   |
                                   |

                               monitor


we have 

                UnprojectedXY = (1.0f / Transform->MetersToPixels)*(PixelsXY - Transform->ScreenCenter);


which is 
                                  1
                P  =    _________________  x (PixelsXY - ScreenCenter);

                         MetersToPixels 


                              1
                P  =    _________________  x P_

                         MetersToPixels 

Makes sense.






for the perspective case 




                 P                 |
                    o              | P_
                                   o
                                   |                c (camera)
                -------------------|-------------------o---------------> Z
                                   |
                                   |      d (monitor to camera)       
                                   |
                                   |
                                   |

                               monitor


                v2 A = (PixelsXY - Transform->ScreenCenter) * (1.0f / Transform->MetersToPixels);
                UnprojectedXY = ((Transform->DistanceAboveTarget - Transform->OffsetP.z)/Transform->FocalLength) * A; 

                v3 Result = V3(UnprojectedXY, Transform->OffsetP.z);
                Result -= Transform->OffsetP;

                return(Result);

as you can see, this is exactly the reverse of GetRenderEntityBasisP();



15:15
Casey changing the input passed into the the game to be in pixels


                win32_handmade.cpp

                WinMain()
                {
                    while(GlobalRunning)
                    {
                        ...
                        ...


                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        NewInput->MouseX = (r32)MouseP.x;
                        NewInput->MouseY = (r32)((GlobalBackbuffer.Height - 1) - MouseP.y);
                        NewInput->MouseZ = 0; // TODO(casey): Support mousewheel?
                    
                        ...
                    }
                }



48:58
recall that since we have changed the mouse output in the platform code,
we also have to change how our previous debug system reads in mouse inputs 

previously, we had 

                internal void DEBUGEnd(debug_state *DebugState, game_input *Input, loaded_bitmap *DrawBuffer)
                {
                    ...

                    v2 MouseP = V2(Input->MouseX, Input->MouseY);

                }

which is actually incorrect. Previously the game input is adjusted based on our screen 
however, our debug ui space actually has its own Orthographic projection 


                internal void DEBUGStart(debug_state *DebugState, game_assets *Assets, u32 Width, u32 Height)
                {
                    ...
                    ...

                    Orthographic(DebugState->RenderGroup, Width, Height, 1.0f);
                }

if we somehow changed the Orthographic projection of the debug system, our mouse picking system would have 
been incorrect in our original implementation.

so for the proper solution, we unproject by the current transform.

                internal void DEBUGEnd(debug_state *DebugState, game_input *Input, loaded_bitmap *DrawBuffer)
                {
                    ...

                    v2 MouseP = Unproject(DebugState->RenderGroup, V2(Input->MouseX, Input->MouseY)).xy;
                    
                    ...
                    ...               
                }








42:24
Casey adding tech to fully examine the moused picked entity for the debug system


                for(uint32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; ++VolumeIndex)
                {
                    sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        

                    v3 LocalMouseP = Unproject(RenderGroup, MouseP);


                    if((LocalMouseP.x > -0.5f*Volume->Dim.x) && (LocalMouseP.x < 0.5f*Volume->Dim.x) &&
                       (LocalMouseP.y > -0.5f*Volume->Dim.y) && (LocalMouseP.y < 0.5f*Volume->Dim.y))
                    {
                        v4 OutlineColor = V4(1, 1, 0, 1);
                        PushRectOutline(RenderGroup, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, OutlineColor, 0.05f);

                        DEBUG_BEGIN_HOT_ELEMENT(Entity);
                        DEBUG_VALUE(Entity->StorageIndex);
                        DEBUG_VALUE(Entity->Updatable);
                        DEBUG_VALUE(Entity->Type);
                        DEBUG_VALUE(Entity->P);
                        DEBUG_VALUE(Entity->dP);
                        DEBUG_VALUE(Entity->DistanceLimit);
                        DEBUG_VALUE(Entity->FacingDirection);
                        DEBUG_VALUE(Entity->tBob);
                        DEBUG_VALUE(Entity->dAbsTileZ);
                        DEBUG_VALUE(Entity->HitPointMax);
                        DEBUG_BEGIN_ARRAY(Entity->HitPoint);
                        for(u32 HitPointIndex = 0;
                            HitPointIndex < Entity->HitPointMax;
                            ++HitPointIndex)
                        {
                            DEBUG_VALUE(Entity->HitPoint[HitPointIndex]);
                        }
                        DEBUG_END_ARRAY();
                        DEBUG_VALUE(Entity->Sword);
                        DEBUG_VALUE(Entity->WalkableDim);
                        DEBUG_VALUE(Entity->WalkableHeight);
                        DEBUG_END_HOT_ELEMENT();
                    }
                }


46:44
Casey adding the 
        DEBUG_BEGIN_HOT_ELEMENT(); 
        DEBUG_VALUE(); 
        DEBUG_BEGIN_ARRAY();

#defines in the platform player


                handmade_platform.h              

                #define DEBUG_BEGIN_HOT_ELEMENT(...)
                #define DEBUG_VALUE(...)
                #define DEBUG_BEGIN_ARRAY(...)
                #define DEBUG_END_ARRAY(...)
                #define DEBUG_END_HOT_ELEMENT(...)


which he will finish in the next episode.


Q/A
Casey explaining the difference of z in our game vs z in 3D

in 3D, the way you handle z is very clean, cuz its just a straight transform. 



                                   |
                    o              |  
                        o          o
                 o                 o                c (camera)
                -------------------|-------------------o---------------> Z
                        o          o
                    o              |        
                                   |
                                   |
                                   |

                               monitor


in our case we have 

                                        Camera
                                     O
                                    /
                                   /
                                  /
                                 /
                                /
                               /
                              ___
                            _|___|_
                           |       |
                           |_______|
    _________________________|___|_________________


so if we were to do it fully 3D, we would see a top down view where  
        
     _________
    |         |
    |  #####  |   
    |_________|

where ##### is the head and the brick is the body. which is totaly wrong

so what that means is that we need to consider the z values a part of the y value in our game 


so assume our screen, initially we have 

        ^
        |
        |
    y   |
        |
    a   |        _________
    x   |       |         |
    i   |       |  #####  |
    s   |       |_________|
        |          |   |  
        ----------------------------->
                x axis 

with proper z and y handling, we will have 
    

        ^
        |
        |
    y   |          ##### 
        |          ##### 
    a   |        __#####__
    x   |       |         |
    i   |       |         |
    s   |       |_________|
        |          |   |
        ----------------------------->
                x axis 


what that means is that we have to conceptualization of what z means:
it is a z value off the ground for purpose of collision detection, but 
it is also an offset in y if you are a part of a model 

Recall for the heros, its three bitmaps put together, the head, the body and the cape 

so we have a two part transform: 

the basis point of our guy gets transformed regularly,
                
                  ##### 
                  ##### 
                __#####__
               |         |
               |         |
               |_________|
                  |   |
                    \   basis point
                     \
                      \  x,y,z 


but the "alignments" for attachment bitmaps, are faked.





