Handmade Hero Day 230 - Refining Renderer Sort Keys

Summary:
implemented a new formula for the sort key 

refactored the render_transform struct. Split it into object_transform struct and camera_transform struct
defined two handy function for transforms, DefaultUprightTransform and DefaultFlatTransform for upright sprites 
and topdown sprites 

briefly mentioned the difference between switch statements vs OOP virtual functions
switch statements is operation centric.
virtual functions are type centric

Keyword:
sorting, rendering, 


1:45
so Casey mentioned that we have a problem where we have to sort 2 our render elements in different ways 
so the "Sort Key" for rendering needs to do 2 things for us. 


essentially we need to come up with a better formula for using 'z' and 'y'

                 _______________________
                /                       \
               /                         \
              /                           \
             /                             \
            /_______________________________\
               /                         \
              /                           \ 
             /                             \
            /_______________________________\
                

6:06
so Casey said that we need to use 'z' pure semantic, rather than treating it as a continuous value
Casey claims that using if you are on a higher plane, that will always render before people in lower planes 
than you, and we want to use 'z' to indicate the plane you are on


so what Casey is thinking about is to have 


z = 0   ground plane 
z = 1   people standing on the ground plane 
z = 2   the plane above the ground plane 
z = 3   people standing on plane level 2

so for sorting rendering elemnts purposes, we have a semantic "z" value, that is set very specifically,
that doesnt have anything to do with any continuous value. 








21:03
cleaned some of the PushBuffer Reset and initialization code 

                Handmade_render_group.cpp

                internal void ClearRenderValues(render_group *RenderGroup)
                {
                    RenderGroup->SortEntryAt = RenderGroup->MaxPushBufferSize;
                    RenderGroup->PushBufferSize = 0;
                    RenderGroup->PushBufferElementCount = 0;
                    RenderGroup->GenerationID = 0;
                    RenderGroup->GlobalAlpha = 1.0f;
                    RenderGroup->MissingResourceCount = 0;
                }

                internal render_group * AllocateRenderGroup(game_assets *Assets, memory_arena *Arena, uint32 MaxPushBufferSize,
                                    b32 RendersInBackground)
                {
                    render_group *Result = PushStruct(Arena, render_group);

                    if(MaxPushBufferSize == 0)
                    {
                        // TODO(casey): Safe cast from memory_uint to uint32?
                        MaxPushBufferSize = (uint32)GetArenaSizeRemaining(Arena);
                    }
                    Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize, NoClear());
                    Result->MaxPushBufferSize = MaxPushBufferSize;
                    
                    ...
                    ...

                    ClearRenderValues(Result);
                    
                    ...
                    ...
                }


27:45
So Casey made some changes in the render_transform struct 
first it added an "Upright" flag, meaning if this gonna be something like a "Upright" sprite or a "top down" sprite

also Casey noted that render_transform has tons of information 

                            handmade_render_group.cpp

                            struct render_transform
                            {
                    ---------   bool32 Orthographic;
                    |            
                    |           // NOTE(casey): Camera parameters
                    |           real32 MetersToPixels; // NOTE(casey): This translates meters _on the monitor_ into pixels _on the monitor_
    camera related  |           v2 ScreenCenter;
                    |
                    |           real32 FocalLength;
                    ---------   real32 DistanceAboveTarget;


    entity related  ---------   v3 OffsetP;
                    ---------   real32 Scale;
                            };

Casey noted that we have stuff that is for camera, and information pertaining to the entity.

44:03
so Casey split it up into 

                handmade_render_group.cpp

                struct object_transform
                {
                    // TODO(casey): Move this out to its own thang
                    b32 Upright;
                    v3 OffsetP;
                    real32 Scale;
                };

                struct camera_transform
                {
                    b32 Orthographic;
                    
                    // NOTE(casey): Camera parameters
                    r32 MetersToPixels; // NOTE(casey): This translates meters _on the monitor_ into pixels _on the monitor_
                    v2 ScreenCenter;

                    r32 FocalLength;
                    r32 DistanceAboveTarget;
                };




29:31
so now we will modify our sort key, which we will add the concept that we have certain sprites that are upwright

if we have topdown sprites and upright sprites on the same plane 
we want upwrit stuff to always render ontop of topdown sprites


                handmade_render_group.cpp

                inline entity_basis_p_result GetRenderEntityBasisP(camera_transform CameraTransform,
                                                                   object_transform ObjectTransform,
                                                                   v3 OriginalP)
                {
                    ...

                    if(CameraTransform.Orthographic)
                    {
                        ...
                    }
                    else
                    {
                        ...
                        ...
                    }

                    Result.SortKey = 4096.0f*(2.0f*P.z + 1.0f*(r32)ObjectTransform.Upright) - P.y;
                    
                    return(Result);
                }




45:32
For conveinience, Casey defined two simple functions that we can use 

                inline object_transform DefaultUprightTransform(void)
                {
                    object_transform Result = {};

                    Result.Upright = true;
                    Result.Scale = 1.0f;

                    return(Result);
                }

                inline object_transform DefaultFlatTransform(void)
                {
                    object_transform Result = {};

                    Result.Scale = 1.0f;

                    return(Result);
                }




54:14
so now we have all the ground chunks as topdown sprites 
so renderering wise, things would either have the DefaultFlatTransform or DefaultUprightTransform.


for example, for most of our entities they have the DefaultUprightTransform 

                for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
                {
                    sim_entity *Entity = SimRegion->Entities + EntityIndex;
                    
                    switch(Entity->Type)
                    {
                        case EntityType_Hero:
                        {
                            ...
                        } break;

                        case EntityType_Sword:
                        {
                            ...
                        } break;

                        case EntityType_Familiar:
                        {
                            ...
                        } break;
                    }
                    
                    ...
                    ...

                    object_transform EntityTransform = DefaultUprightTransform();
                    EntityTransform.OffsetP = GetEntityGroundPoint(Entity);
                }


Q/A
1:06:33
so Casey briefly mentioned the difference between using a giant swtich statement vs OOP virtual functions 

for example, for the switch statement method you would have 

                void foo()
                {
                    Switch(type)
                    {
                        case type1;
                            ...
                            break;

                        case type2:
                            ...
                            break;

                        case type3:
                            ...
                            break;
                    }
                }


for the oop method, you have 

                type1::foo()
                {
                    ...
                }

                type2::foo()
                {
                    ...
                }
                
                type3::foo()
                {
                    ...
                }
                

code wise, they are the same. the only difference is 
the switch statement method is more operation centric,

the oop method is more type centric. all the things that is related to that type is together in one file

Casey much prefers operation centric. it has the advantage of putting everything the programmer needs to know 
about an operation in one place.         

in the end, you have to write exactly the same code in both cases. you dont save typing with the oop method


also in the case of OOP virtual function method, there is no way to share variables. if you do, you have 
to pass it in. 

or if you have logic that you want to share before and after the foo(); function, you will have to make base classes do that
and stuff 


                void foo()
                {
    ----------->    int sharedVariable = ...

                    Switch(type)
                    {
                        case type1;
                            ...
                            break;

                        case type2:
                            ...
                            break;

                        case type3:
                            ...
                            break;
                    }
                }


1:18:24
how would front to back render actually work?

there is 2 different ways:
1.  they would work with blocks of the screen, and they do it semantically. 

    so you can take all your bitmaps, and you would assign rectangles to your bitmaps. 
    Then you union your rectangles that forms a mask.

                 _________
             ___|___      |
            |       |     |
            |       |     |
            |_______|     |_____
                |_________|     |
                       |        |
                       |________|

    then you would just clip things to that mask. So if it gets clipped by that mask, you wont render it 


2.  or you can do it in the rasterizer by having a flag, to say whether a pixel is already filled
    essentially an early out with the z stuff. pretty much what a graphics cards do 







