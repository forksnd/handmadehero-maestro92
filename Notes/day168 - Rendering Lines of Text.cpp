Handmade Hero Day 168 - Rendering Lines of Text

Summary:
created a global debug render group for rendering debug text. 

made it so that the debug text are rendered at the top left corner of the screen

previously only loaded font glyphs for capital letters.
added glyphs for most of ASCII characters

demonstrated that multi dimensional arrays are sequential in memory

Keyword:
graphics, font

7:29
Todays goal is to display debug counters on the screen

8:00
so in HandleDebugCycleCounters(); we used call_snprintf_s to print out the string in the console.
now we want to display this as text in our game

                win32_handmade.cpp

                internal void HandleDebugCycleCounters(game_memory *Memory)
                {
                #if HANDMADE_INTERNAL
                    OutputDebugStringA("DEBUG CYCLE COUNTS:\n");
                    for(int CounterIndex = 0;
                        CounterIndex < ArrayCount(Memory->Counters);
                        ++CounterIndex)
                    {
                        debug_cycle_counter *Counter = Memory->Counters + CounterIndex;

                        if(Counter->HitCount)
                        {
                            char TextBuffer[256];
                            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                        "  %d: %I64ucy %uh %I64ucy/h\n",
                                        CounterIndex,
                                        Counter->CycleCount,
                                        Counter->HitCount,
                                        Counter->CycleCount / Counter->HitCount);
                            OutputDebugStringA(TextBuffer);
                            Counter->HitCount = 0;
                            Counter->CycleCount = 0;
                        }
                    }
                #endif
                }

recall we were printing DebugCycleCounters for 

                handmade_platform.h

                enum
                {
                    /* 0 */ DebugCycleCounter_GameUpdateAndRender,
                    /* 1 */ DebugCycleCounter_RenderGroupToOutput,
                    /* 2 */ DebugCycleCounter_DrawRectangleSlowly,
                    /* 3 */ DebugCycleCounter_ProcessPixel,
                    /* 4 */ DebugCycleCounter_DrawRectangleQuickly,
                    DebugCycleCounter_Count,
                };
                typedef struct debug_cycle_counter
                {    
                    uint64 CycleCount;
                    uint32 HitCount;
                } debug_cycle_counter;



there are two parts of displaying this that we have to write.
1.  we need to put fronts on the screen 

2.  and then there is the part that we want to convert numerical numbers into text.
    this part we dont care right now.

right now we are just working on the font part of stuff.


10:17
Casey begins to develop the text rendering code by usage usage code.
He first writes OverlayCycleCounters(); he assumes that we will be passed in a RenderGroup

so the first draft he has something like the following

                handmade.cpp

                internal void OverlayCycleCounters(render_group* RenderGroup, game_memory *Memory)
                {
                    PushText(RenderGroup, "DEBUG CYCLE COUNTS:");

                    for(int CounterIndex = 0; CounterIndex < ArrayCount(Memory->Counters); ++CounterIndex)
                    {
                        debug_cycle_counter *Counter = Memory->Counters + CounterIndex;

                        if(Counter->HitCount)
                        {
                            PushText(RenderGroup, NameTable[CounterIndex]); 
                        }
                    }
                }


12:32
Casey gave names to each of the debuc counter enums 

each of these string will correspond to the enums 

                handmade_platform.h

                enum
                {
                    /* 0 */ DebugCycleCounter_GameUpdateAndRender,
                    /* 1 */ DebugCycleCounter_RenderGroupToOutput,
                    /* 2 */ DebugCycleCounter_DrawRectangleSlowly,
                    /* 3 */ DebugCycleCounter_ProcessPixel,
                    /* 4 */ DebugCycleCounter_DrawRectangleQuickly,
                    DebugCycleCounter_Count,
                };

-   full code below:

                handmade.cpp

                internal void OverlayCycleCounters(game_memory *Memory)
                {
                    char *NameTable[] =
                    {
                        "GameUpdateAndRender",
                        "RenderGroupToOutput",
                        "DrawRectangleSlowly",
                        "ProcessPixel",
                        "DrawRectangleQuickly",
                    };

                    ...
                    ...
                }



14:41
so when Casey is considering the PushText(); function, normally you would have to specify your coordinates within the 
RenderGroup in your PushText(); function. But we dont really want to do that here.
If we are talking about debug code we dont want to spend a lot of mental energies, in order to do debug print outs. 

so what we rather do the things that draw fonts in our engine for debug stuff, is that 
the RenderGroup of it just prints them line by line

we are making this convention only for debug texts, so we are explicitly callin the function DEBUGTextLine();

                handmade.cpp

                internal void OverlayCycleCounters(render_group* RenderGroup, game_memory *Memory)
                {
                    char *NameTable[] =
                    {
                        "GameUpdateAndRender",
                        "RenderGroupToOutput",
                        "DrawRectangleSlowly",
                        "ProcessPixel",
                        "DrawRectangleQuickly",
                    };

                    DEBUGTextLine("\\#900DEBUG \\#090CYCLE \\#990\\^5COUNTS:");
                    for(int CounterIndex = 0; CounterIndex < ArrayCount(Memory->Counters); ++CounterIndex)
                    {
                        debug_cycle_counter *Counter = Memory->Counters + CounterIndex;

                        if(Counter->HitCount)
                        {
                            DEBUGTextLine(RenderGroup, NameTable[CounterIndex]);
                        }
                    }
                }

15:56
in the future in our game, we actually might be rendering text for graphics, (not for debugging), then we will be specifiying coordinates.


17:02
Casey also mentioned that, he wonders if he should just make displaying debugging text implicit. Its a global variable that you dont have to 
pass around, and it is available everywhere.

This is only for debugging, and we can compile it in a ship build, making this as a globle variable is probably simpler.

So Casey got rid of the RenderGroup.

                handmade.cpp

                internal void OverlayCycleCounters(game_memory *Memory)
                {
                    char *NameTable[] =
                    {
                        "GameUpdateAndRender",
                        "RenderGroupToOutput",
                        "DrawRectangleSlowly",
                        "ProcessPixel",
                        "DrawRectangleQuickly",
                    };

                    DEBUGTextLine("\\#900DEBUG \\#090CYCLE \\#990\\^5COUNTS:");
                    for(int CounterIndex = 0; CounterIndex < ArrayCount(Memory->Counters); ++CounterIndex)
                    {
                        debug_cycle_counter *Counter = Memory->Counters + CounterIndex;

                        if(Counter->HitCount)
                        {
                            DEBUGTextLine(NameTable[CounterIndex]);
                        }
                    }
                }

18:14 25:58
So Casey also defined the Debug render group as a global variable.
                
                global_variable render_group *DEBUGRenderGroup;

20:25
Casey writing the DEBUGTextLine(); function
the first draft we have is:

                internal void
                DEBUGTextLine(char *String)
                {    
                    if(DEBUGRenderGroup)
                    {
                        asset_vector MatchVector = {};
                        asset_vector WeightVector = {};
                        WeightVector.E[Tag_UnicodeCodepoint] = 1.0f;

                        for(char *At = String; *At;)
                        {
                            MatchVector.E[Tag_UnicodeCodepoint] = *At;
                            // TODO(casey): This is too slow for text, at the moment!
                            bitmap_id BitmapID = GetBestMatchBitmapFrom(RenderGroup->Assets, Asset_Font, &MatchVector, &WeightVector);
                            PushBitmap(RenderGroup, BitmapID, CharScale, V3(AtX, AtY, 0), Color);
                        }
                    }
                }




24:23
in the Tick function(); we call OverlayCycleCounters(); and the TiledRenderGroupToOutput();
Recall that nothing gets rendered until you call RenderGroupToOutput();

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    ...
                    ...

                    OverlayCycleCounters(Memory);
                    if(DEBUGRenderGroup)
                    {
                        TiledRenderGroupToOutput(TranState->HighPriorityQueue, DEBUGRenderGroup, DrawBuffer);
                    }
                }


24:56
then we have to set up that DEBUGRenderGroup.
we intialize it in the initalization stage
notice that we pass the false in the last argument, indicating that its not going to render in the background

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    if(!TranState->IsInitialized)
                    {
                        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));
                            
                        ...
                        ...

                        DEBUGRenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(16), false);


                    }

                }




29:04
we want to have a render group that can be reused in our engine. 

Previously we had the FinishRenderGroup(); function

                internal void
                FinishRenderGroup(render_group *Group)
                {
                    if(Group)
                    {
                        EndGeneration(Group->Assets, Group->GenerationID);
                    }
                }

which is just calling EndGeneration.


But now we want the RenderGroup to be able to reused 
So Casey refactored the API and wrote the BeginRender(); and EndRender(); function

                handmade_render.cpp

                internal void
                BeginRender(render_group *Group)
                {
                    if(Group)
                    {
                        Assert(!Group->InsideRender);
                        Group->InsideRender = true;

                        Group->GenerationID = BeginGeneration(Group->Assets);
                    }
                }

                internal void
                EndRender(render_group *Group)
                {
                    if(Group)
                    {
                        Assert(Group->InsideRender);
                        Group->InsideRender = false;
                        
                        EndGeneration(Group->Assets, Group->GenerationID);
                        Group->GenerationID = 0;
                        Group->PushBufferSize = 0;
                    }
                }

specifically in the EndRender(); function, we "Reset" some of the fields of the render_group:
-   we set the InsideRender flag to be false.
    thats just a flag indicating whether this RenderGroup is being used or not.

    for a render_group that we will reuse constantly, we will be toggling this flag

-   we reset the GenerationID to be 0

-   we reset the PushBufferSize to be 0


so now we add BeginRender(); and EndRender(); functions everywhere

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    ...
                    ...

                    if(DEBUGRenderGroup)
                    {
                        BeginRender(DEBUGRenderGroup);
                    }

                    ...
                    ...

                    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), false);
    ------------>   BeginRender(RenderGroup);
                  
                    ...
                    ...


                    TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer);    
    ------------>   EndRender(RenderGroup);

                    ...
                    ...

                    EndSim(SimRegion, GameState);
                    EndTemporaryMemory(SimMemory);
                    EndTemporaryMemory(RenderMemory);
                    
                    ...
                    ...

                    OverlayCycleCounters(Memory);

                    if(DEBUGRenderGroup)
                    {
                        TiledRenderGroupToOutput(TranState->HighPriorityQueue, DEBUGRenderGroup, DrawBuffer);
    ------------>       EndRender(DEBUGRenderGroup);
                    }
                }



32:19
Casey added Assert Guards in all the rendering functions

                handmade.cpp 

                internal void RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {
                    Assert(RenderGroup->InsideRender);      <-------------------------

                    Assert(((uintptr)OutputTarget->Memory & 15) == 0);    

                    ...
                    ...

                    DoTiledRenderWork(0, &Work);
                }


34:44
now we want to setup our DEBUGRenderGroup. for our debug output we just want stuff to be orthographic.

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    ...
                    ...

                    if(DEBUGRenderGroup)
                    {
                        BeginRender(DEBUGRenderGroup);
                        Orthographic(DEBUGRenderGroup, Width, Height, 100.0f);
                    }

                    ...
                    ...

                    if(DEBUGRenderGroup)
                    {
                        TiledRenderGroupToOutput(TranState->HighPriorityQueue, DEBUGRenderGroup, DrawBuffer);
                        EndRender(DEBUGRenderGroup);
                    }

                }

26:04
at this timestamp, you can see the text being rendered all at 0,0 location






37:00
Casey coming back to the DEBUGTextLine(); function 
So Casey will be writing this function to render glyph bitmaps lined up horizontally

-   we loop through the string, for every iteration, we update AtX. 
    We advance AtX by CharScale.

                handmade.cpp

                internal void DEBUGTextLine(char *String)
                {    
                    if(DEBUGRenderGroup)
                    {
                        render_group *RenderGroup = DEBUGRenderGroup;
                    
                        asset_vector MatchVector = {};
                        asset_vector WeightVector = {};
                        WeightVector.E[Tag_UnicodeCodepoint] = 1.0f;

                        r32 CharScale = FontScale;
                        v4 Color = V4(1, 1, 1, 1);
                        r32 AtX = 0.0;
                        for(char *At = String; *At; ++At)
                        {
                            MatchVector.E[Tag_UnicodeCodepoint] = *At;
                            bitmap_id BitmapID = GetBestMatchBitmapFrom(RenderGroup->Assets, Asset_Font,
                                                                        &MatchVector, &WeightVector);
                            PushBitmap(RenderGroup, BitmapID, CharScale, V3(AtX, 0, 0), Color);                        
                            AtX += CharScale;                     
                        }
                    }
                }


37:55
at this timestamp, we can see that we are actually rendering text.
but we are rendering at the center of the screen.
and rendering at the center of the screen is not what we want. 

we want display it in the top left orner 

also we want the AtY to advance line by line whenever we have new DEBUGTextLine request

so we added a global AtY variable.
the idea is that at the beginning of the frame, we reset AtY,
then during the frame, everytime DEBUGTextLine(); gets called, we render starting at a new line 
by advancing AtY.


so now we have 
                // TODO(casey): Fix this for looped live code editing
                global_variable render_group *DEBUGRenderGroup;
                global_variable r32 LeftEdge;
                global_variable r32 AtY;
                global_variable r32 FontScale;


39:33 
we also add the DEBUGReset(); function to Reset the variables for the DEBUGRenderGroup;

                handmade.cpp

                internal void DEBUGReset(u32 Width, u32 Height)
                {
                    FontScale = 20.0f;
                    Orthographic(DEBUGRenderGroup, Width, Height, 1.0f);
                    AtY = 0.5f*Height - 0.5f*FontScale;
                    LeftEdge = -0.5f*Width + 0.5f*FontScale;
                }



and in the DEBUGTextLine(); function, we have 


                handmade.cpp

                internal void DEBUGTextLine(char *String)
                {    
                    if(DEBUGRenderGroup)
                    {
                        render_group *RenderGroup = DEBUGRenderGroup;
                    
                        asset_vector MatchVector = {};
                        asset_vector WeightVector = {};
                        WeightVector.E[Tag_UnicodeCodepoint] = 1.0f;

                        v4 Color = V4(1, 1, 1, 1);
                        r32 AtX = 0.0;

                        for(char *At = String; *At;)
                        {
                            MatchVector.E[Tag_UnicodeCodepoint] = *At;
                            bitmap_id BitmapID = GetBestMatchBitmapFrom(RenderGroup->Assets, Asset_Font,
                                                                        &MatchVector, &WeightVector);
                            PushBitmap(RenderGroup, BitmapID, CharScale, V3(AtX, 0, 0), Color);                        
                            AtX += CharScale;

                            ++At;
                        }

                        AtY -= 1.2f*FontScale;
                    }
                }

40:28 
at this point we are rendering text, but you may notice that there are bunch of 'Z'
that is becuz our asset system dont have lower case letters. So when we call GetBestMatchBitmapFrom();
it is choosing 'Z', which is the closest match.


41:29
Casey went in the Asset system to fix the text rendering for lower case letters 

so previously we had, where we are just going from 'A' to 'Z' 
                
                test_asset_builder.cpp

                internal void
                WriteNonHero(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    ...
                    ...

                    BeginAssetType(Assets, Asset_Font);
                    for(u32 Character = 'A';
                        Character <= 'Z';
                        ++Character)
                    {
                        AddCharacterAsset(Assets, "c:/Windows/Fonts/cour.ttf", "Courier New", Character);
                        AddTag(Assets, Tag_UnicodeCodepoint, (r32)Character);
                    }
                    EndAssetType(Assets);
                    
                    WriteHHA(Assets, "test2.hha");
                }


now we are doing '!' to '~', 
'!' is 33 on the ASCII table and 
'~' is 126

-   also notice that we are skipping ' '
                
                test_asset_builder.cpp

                internal void
                WriteNonHero(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    ...
                    ...

                    BeginAssetType(Assets, Asset_Font);
                    for(u32 Character = '!';
                        Character <= '~';
                        ++Character)
                    {
                        if(*At != ' ')
                        {
                            AddCharacterAsset(Assets, "c:/Windows/Fonts/cour.ttf", "Courier New", Character);
                            AddTag(Assets, Tag_UnicodeCodepoint, (r32)Character);
                        }
                    }
                    EndAssetType(Assets);
                    
                    WriteHHA(Assets, "test2.hha");
                }



1:00:20
Casey talking about how arrays are laid out in memory in C
                
                u8 array[7][6][5][4][3][2];

the way they work is that they are completely sequential

and if you initalize it to be 

                array[0][0][0][0][0][1] = 0xA;
                array[0][0][0][0][1][0] = 0xB;
                array[0][0][0][1][0][1] = 0xC;
                array[0][0][1][0][0][1] = 0xD;
                array[0][1][0][0][0][1] = 0xE;
                array[1][0][0][0][0][1] = 0xF;

the array in memory will be that 

00 0A 0b 00 00 00 0c .....
.... 0d ........0e ...
..... 0f

so multi dimensional arrays are actually sequential in memory 
