Handmade Hero Day 247 - Simplifying Debug Values

Summary:
cleaning up the debug system

Keyword:
Debug System


0:23
Casey mentioned that we havent gotten a response from nvidia, so we will take a break from GPU texture downloads.
we dont know the proper way to implement multi-threaded texture downloads. the only people who knows are from nvidia
so until NVIDA gets back to Casey, we will take a break. 

Casye mentions that this is very typical of graphics programming. You can even go check out the latest published thing 
on NVIDIA and see how they recommend on implementing this, but when you got actually implement it, you find out it doesnt work 
in some places, and its not true in certain parts. 

its possible that its no longer the right way anymore, and nobody bothered to publish what the actual right way was.



2:48
Casey also mentiones the concept of "direct GPU mapping"

https://developer.nvidia.com/gpudirect


4:30
so the way we did it now is the last way Casey saw NVIDIA published. 
so until NVIDA gets back to Casey, we cant do much


5:52
that said, there is one thing that we havent implemented that was recommended in the last publication.
which is to fence the download on either side. 

the reason why we havent done it is because we dont care if it gets fensed 


19:30
Casey mentioned that he didnt like the idea of having the config file for all the debug values



36:40
So Casey is in the process of debugging the debug system.
we had a problem where rendered debug text (in white color); overlaps with the white background,
so we can see anything.

What Casey did is to just render some drop shadow, there are a number of different ways to do that 
one way is to just render the debug text twice. 

notice in the second render call, we offset the BitmapOffset by a bit. so we render a little bit on the right,
and a little bit lower:
                
                BitmapOffset + V3(2.0f, -2.0f, 0.0f)

also we render it in black, passing the 

                V4(0, 0, 0, 1.0f);

-   full code below:

                handmade_debug.cpp 

                internal rectangle2 DEBUGTextOp(debug_state *DebugState, debug_text_op Op, v2 P, char *String, v4 Color = V4(1, 1, 1, 1))
                {
                    rectangle2 Result = InvertedInfinityRectangle2();
                    if(DebugState && DebugState->DebugFont)
                    {
                        ...
                        ...
                        for(char *At = String; *At; )
                        {
                           
                           ...
                           ...
                                
                            if(CodePoint != ' ')
                            {
                                ...
                                ...

                                if(Op == DEBUGTextOp_DrawText)
                                {
                                    PushBitmap(RenderGroup, DefaultFlatTransform(), BitmapID, BitmapScale,
                                        BitmapOffset, Color, 1.0f, 200000.0f);
        ----------------------->    PushBitmap(RenderGroup, DefaultFlatTransform(), BitmapID, BitmapScale,
                                        BitmapOffset + V3(2.0f, -2.0f, 0.0f), V4(0, 0, 0, 1.0f), 1.0f, 100000.0f);
                                }
                                else                    
                                {
                                    ...
                                    ...
                                }
                            }

                        }
                    }
                    
                    return(Result);
                }


41:53
recall previously we had the 
the DEBUG_IF__ and DEBUG_VARIABLE__ in our handmade_debug_interface.h

                #define DEBUG_IF__(Path)  \
                    local_persist debug_event DebugValue##Path = DEBUGInitializeValue((DebugValue##Path.Value_b32 = GlobalConstants_##Path, DebugType_b32), &DebugValue##Path, UniqueFileCounterString(), #Path); \
                    if(DebugValue##Path.Value_b32)

                #define DEBUG_VARIABLE__(type, Path, Variable)                          \
                    local_persist debug_event DebugValue##Variable = DEBUGInitializeValue((DebugValue##Variable.Value_##type = GlobalConstants_##Path##_##Variable, DebugType_##type), &DebugValue##Variable, UniqueFileCounterString(), #Path "_" #Variable); \
                    type Variable = DebugValue##Variable.Value_##type;


Casey thinks it gives too much complexity, and tries to get rid of it.


