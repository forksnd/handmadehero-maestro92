Handmade Hero Day 191 - Implementing a Radial Menu

Summary:
Drew a radial menu for debugging 

added tech for turning it on and off by right clicking 


Keyword:
UI, Debug system 




10:16
added a DraweDebugMainMenu(); function
this is gonna be a menu that pops out when the user right clicks.


-   so we first define a list of menu items 


-   also we want to draw this as a Radial Menu, so something like the following 
    
    essentially all the items revolves around a circle 

                            
                            item 3
         
                 item 4                item 2 
                                    

            item 5                          item 1


                 item 6                 itme 8

                            item 7 

    hence we have the AngleStep variable.


    r32 AngleStep = Tau32 / (r32)ArrayCount(MenuItems);


-   full code below:


                hanmade_debug.cpp

                internal void DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    char *MenuItems[] =
                    {
                        "Toggle Profile Graph",
                        "Toggle Debug Collation",
                        "Toggle Framerate Counter",
                        "Mark Loop Point",
                        "Toggle Entity Bounds",
                        "Toggle World Chunk Bounds",
                    };
                    
                    r32 MenuRadius = 200.0f;
                    r32 AngleStep = Tau32 / (r32)ArrayCount(MenuItems);

                    for(u32 MenuItemIndex = 0; MenuItemIndex < ArrayCount(MenuItems); ++MenuItemIndex)
                    {
                        char *Text = MenuItems[MenuItemIndex];

                        v4 ItemColor = V4(1, 1, 1, 1);
                        if(MenuItemIndex == DebugState->HotMenuIndex)
                        {
                            ItemColor = V4(1, 1, 0, 1);
                        }
                        
                        r32 Angle = (r32)MenuItemIndex*AngleStep;

                        rectangle2 TextBounds = DEBUGGetTextSize(DebugState, Text);        
                        DEBUGTextOutAt(TextP - 0.5f*GetDim(TextBounds), Text, ItemColor);
                    }

                    DebugState->HotMenuIndex = NewHotMenuIndex;
                }







16:10
Casey added the Arm2 function 

                handmade_math.h

                inline v2
                Arm2(r32 Angle)
                {
                    v2 Result = {Cos(Angle), Sin(Angle)};

                    return(Result);
                }

that just gives the coordinate on a unit circle 




25:06
Casey would like a utility function where we can know the dimensions of a rendered bitmap
without us calling PushBitmap();

So Casey added a used_bitmap_dim struct.

                handmade_render_group.h 

                struct used_bitmap_dim
                {
                    entity_basis_p_result Basis;
                    v2 Size;
                    v2 Align;
                    v3 P;
                };


and we have a function 

    
                handmade_render_group.cpp 

                inline used_bitmap_dim GetBitmapDim(render_group *Group, loaded_bitmap *Bitmap, real32 Height, v3 Offset)
                {
                    used_bitmap_dim Dim;
                    
                    Dim.Size = V2(Height*Bitmap->WidthOverHeight, Height);
                    Dim.Align = Hadamard(Bitmap->AlignPercentage, Dim.Size);
                    Dim.P = Offset - V3(Dim.Align, 0);
                    Dim.Basis = GetRenderEntityBasisP(&Group->Transform, Dim.P);

                    return(Dim);
                }


then Casey cleaned up the PushBitmap(); function as well 

which it now uses the GetBitmapDim(); function

                inline void PushBitmap(render_group *Group, loaded_bitmap *Bitmap, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
                {
    ------------>   used_bitmap_dim Dim = GetBitmapDim(Group, Bitmap, Height, Offset);
                    if(Dim.Basis.Valid)
                    {
                        render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap);
                        if(Entry)
                        {
                            Entry->Bitmap = Bitmap;
                            Entry->P = Dim.Basis.P;
                            Entry->Color = Group->GlobalAlpha*Color;
                            Entry->Size = Dim.Basis.Scale*Dim.Size;
                        }
                    }
                }




51:02
added a distance check in the DrawDebugMainMenu(); so we know which menu item we are picking 

we use the BestDistanceSq and NewHotMenuIndex variable to keep track of the menu item we are closest to
and thats the one select

                hanmade_debug.cpp

                internal void DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    char *MenuItems[] =
                    {
                        "Toggle Profile Graph",
                        "Toggle Debug Collation",
                        "Toggle Framerate Counter",
                        "Mark Loop Point",
                        "Toggle Entity Bounds",
                        "Toggle World Chunk Bounds",
                    };
                    
                    u32 NewHotMenuIndex = ArrayCount(MenuItems);
                    r32 BestDistanceSq = Real32Maximum;
                    
                    r32 MenuRadius = 200.0f;
                    r32 AngleStep = Tau32 / (r32)ArrayCount(MenuItems);

                    for(u32 MenuItemIndex = 0; MenuItemIndex < ArrayCount(MenuItems); ++MenuItemIndex)
                    {
                        char *Text = MenuItems[MenuItemIndex];

                        v4 ItemColor = V4(1, 1, 1, 1);
                        if(MenuItemIndex == DebugState->HotMenuIndex)
                        {
                            ItemColor = V4(1, 1, 0, 1);
                        }
                        
                        r32 Angle = (r32)MenuItemIndex*AngleStep;
                        v2 TextP = DebugState->MenuP + MenuRadius*Arm2(Angle);

                        r32 ThisDistanceSq = LengthSq(TextP - MouseP);
                        if(BestDistanceSq > ThisDistanceSq)
                        {
                            NewHotMenuIndex = MenuItemIndex;
                            BestDistanceSq = ThisDistanceSq;
                        }

                        rectangle2 TextBounds = DEBUGGetTextSize(DebugState, Text);        
                        DEBUGTextOutAt(TextP - 0.5f*GetDim(TextBounds), Text, ItemColor);
                    }

                    DebugState->HotMenuIndex = NewHotMenuIndex;
                }



54:38
in the DEBUGEnd(); function, we render this if we have our right mouse button clicked 


                internal void DEBUGEnd(game_input *Input, loaded_bitmap *DrawBuffer)
                {

                    debug_state *DebugState = DEBUGGetState();
                    if(DebugState)
                    {
                        ...
                        ...

                        if(Input->MouseButtons[PlatformMouseButton_Right].EndedDown)
                        {
                            if(Input->MouseButtons[PlatformMouseButton_Right].HalfTransitionCount > 0)
                            {
                                DebugState->MenuP = MouseP;
                            }            
                            DrawDebugMainMenu(DebugState, RenderGroup, MouseP);
                        }
                        else if(Input->MouseButtons[PlatformMouseButton_Right].HalfTransitionCount > 0)
                        {
                            DrawDebugMainMenu(DebugState, RenderGroup, MouseP);
                            switch(DebugState->HotMenuIndex)
                            {
                                case 0:
                                {
                                    DebugState->ProfileOn = !DebugState->ProfileOn;
                                } break;

                                case 1:
                                {
                                    DebugState->Paused = !DebugState->Paused;
                                } break;
                            }
                        }


                        if(DebugState->ProfileOn)
                        {
                            .............................................................
                            ........... Render Our Debugging Stuff ......................
                            .............................................................
                        }
                    }
                }



Q/A
Casey explaining how he would do closures in C 


recall in our code base, DrawRectangleSlowly(); function

    
                handmade_render_group.cpp

                void DrawRectangleSlowly()
                {
                    for(int Y = YMin; Y <= YMax; ++Y)
                    {
                        uint32 *Pixel = (uint32 *)Row;
                        for(int X = XMin; X <= XMax; ++X)
                        {
                            .........................................................
                            ................... Render my Pixel .....................
                            .........................................................
                        }
                    }
                }





so this is a very complicated function, and inside it has essentially some shader code 

for a given pixel, compute what the color should be. 
if we want to make mulitple shaders, the logical way that you would do this is that 

you make the iteration code, (our double for loop); reusable, and we just want to replace the 
"Render My Pixel" portion with different shader code.

so ideally we want something like 


                handmade_render_group.cpp

                void DrawRectangleSlowly()
                {
                    for(int Y = YMin; Y <= YMax; ++Y)
                    {
                        uint32 *Pixel = (uint32 *)Row;
                        for(int X = XMin; X <= XMax; ++X)
                        {
                            DoWorkOnPixel(Pixel);
                        }
                    }
                }



however, often times the DoWorkOnPixel(); wants to have access on a separate stack frame. 
it wants to have access to varaibles that are not necessarily known in the DrawRectangleSlowly(); function.

For example: for our normal mapping shader, and it needs the lighting enviornment, whereas other shaders 
doesnt do any lighting, so it wont need lighting enviornment information. 

so what we want to to pass in is not just the code we want to execute, but also the stack frame, a little set of data.

other programming languages allows you to do this, such as closures which allows you to encapsulate data thats referenced 
by something into itself to be passed through to somewhere. and you can come back to it, after doing the work you want on the data 


This is something C cant do, and C++ recently added lambdas which allows you to do some of that stuff. 



1:07:02
so what Casey did is just a poor mans version of it.

Casey just uses a switch statement, and put all the code in there.
you can put a switch statement or you can pass in an Op code  (essentially a shit ton of if statements);

branches are relatively cheap, so its not too bad.

                handmade_render_group.cpp

                void DrawRectangleSlowly()
                {
                    for(int Y = YMin; Y <= YMax; ++Y)
                    {
                        uint32 *Pixel = (uint32 *)Row;
                        for(int X = XMin; X <= XMax; ++X)
                        {
                            if( mode == NORMAL_MAPPING)
                            {
                                RenderNormalMapping();
                            }
                            else if ( mode == REGULAR_RENDERING)
                            {
                                RegularRendering();
                            }
                            ...
                            ...
                        }
                    }
                }




