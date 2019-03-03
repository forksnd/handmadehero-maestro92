Handmade Hero Day 239 - Rendering the Game Through OpenGL

Summary:

Explained how to do blending in OpenGL

mentioned that we would like to support the option to do the rendering in multiple ways,
software rendering or hardware rendering (such as openGL);, or even different ways of hardware rendering (OpenGL or mental);

Casey explained that there are two ways of doing, one the "common way" which Casey doesnt like,
and the Casey way. 

explained it that the more "common way" doesnt give the platform layer as much control over the rendering.
the 2nd way makes the game code and rendering code more separated and more decoupled. 

Keyword:
OpenGL, rendering


2:50
following up on yesterday_s episode, we wrote a bunch of openGL rendering code in handmade_opengl.h. 
but we arent actually using it. so we will like to somehow use it 

essentially how do we make our renderer use the software renderer as well as OpenGL
there are a couple of different ways that we can do this. 


Casey will first do it in a way that people often would do it.
and then Casey will mention why he doesnt think its a very good idea 
and how we might improve it. 



5:45 
so Casey will first demonstrate the common version:

so here is our situation. So currently we are calling TiledRenderGroupToOutput to do our rendering.


                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    Platform = Memory->PlatformAPI;    

                    ...
                    ...

                    if(AllResourcesPresent(RenderGroup))
                    {
                        TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer, &TranState->TranArena);
                    }

                    ...
                    ...
                }


so instead of calling TiledRenderGroupToOutput(); we will insert a mitigation call, 
a call that will either go to OpenGL or our software renderer depending on the circumstances. 

so we add a function 

                handmade_render_group.cpp 

                inline void RenderToOutput(platform_work_queue *RenderQueue,
                               render_group *RenderGroup, loaded_bitmap *OutputTarget,
                               memory_arena *TempArena)
                {
                    // TODO(casey): Don't do this twice?
                    SortEntries(RenderGroup, TempArena);

                    if(1) // RenderGroup->IsHardware)
                    {
                        Platform.RenderToOpenGL(RenderGroup, OutputTarget);
                    }
                    else
                    {
                        TiledRenderGroupToOutput(RenderQueue, RenderGroup, OutputTarget, TempArena);        
                    }
                }



10:24
so instead of polluting our entire code base with #include "handmade_opengl.h", we will delay that, and just add a 
platform function call 

                typedef void platform_opengl_render(struct render_group *RenderGroup, struct loaded_bitmap *OutputTarget);                              

                typedef struct platform_api
                {
                    ...
                    ...

                    // TODO(casey): Temporary?
                    platform_opengl_render *RenderToOpenGL;
                    
                    ...
                    ...

                } platform_api;



14:02
and now we have 

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    Platform = Memory->PlatformAPI;    

                    ...
                    ...

                    if(AllResourcesPresent(RenderGroup))
                    {
                        RenderToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer, &TranState->TranArena);
                    }

                    ...
                    ...
                }


and we initialize the platform_api function pointers 

                win32_handmade.cpp

                GameMemory.PlatformAPI.RenderToOpenGL = OpenGLRenderGroupToOutput;


the OpenGLRenderGroupToOutput(); function is the one we defined in handmade_opengl.cpp


                handmade_opengl.cpp

                internal void OpenGLRenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {  

                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)
                            (RenderGroup->PushBufferBase + Entry->PushBufferOffset);
                        
                        void *Data = (uint8 *)Header + sizeof(*Header);
                        switch(Header->Type)
                        {
                            ...
                            ...
                        }
                    }
                }

so in short, we defined the OpenGL version of RenderGroupToOutput(); the one we wrote in day238, and we assign it to the 
platformAPI.



19:05
also in the Win32DisplayBufferInWindow(); function, we completely commented out everything.
we only left the SwapBuffers(); calls. 


                internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                                           HDC DeviceContext, int WindowWidth, int WindowHeight)
                {
                #if 0
                    .............................................................
                    ...............windows StretchDIBits(); call ................
                    .............................................................
                #endif

                #if 0
                    ............................................................
                    ................ OpenGL Rendering Buffer ...................
                    ............................................................
                #endif
                    
                    SwapBuffers(DeviceContext);
                }


20:08
so now Casey wants to first add the tech to render textured quads 

so just as a first implementation, we will bind the textures, and submit the texture every frame. 
this will be incredibly slow, but this is just to get something to the screen.


                case RenderGroupEntryType_render_entry_bitmap:
                {
                    render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                    Assert(Entry->Bitmap);

                    v2 XAxis = {1, 0};
                    v2 YAxis = {0, 1};
                    v2 MinP = Entry->P;
                    v2 MaxP = MinP + Entry->Size.x*XAxis + Entry->Size.y*YAxis;

                    if(Entry->Bitmap->Handle)
                    {
                        glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);
                    }
                    else
                    {
                        Entry->Bitmap->Handle = ++TextureBindCount;
                        glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);
        
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    }
                    
                    OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                } break;


23:00
so at this point, when we render to the screen, Casey pointed out that our blending doesnt work,
so Casey will point out how blending works in OpenGL 

so [refer to my glsl piepline google doc]
the pipeline is 

    Vertex Shader ---> Geometry Shader ---> Clipping ---> Rasterization ---> Fragment Shader ---> Blending


in the fixed functin pipeline, has preloaded functions for each step. essentially, there is 

    OpenGL_s code ----> OpenGL_s code ..... [ OpenGL_s code at every stage ]


so the reason why Casey brought this up is that 
why Vertex Shader, Geometry Shader .... Fragment Shader all became programmable,
the Blending stage didnt change much, its still not very programmable on a lot of GPU.

what that means, after the Fragment Shader, (in which the pixel colors are determined);
what happens to them as compared to whats underneath them

[ what Casey means is Destination pixel vs Source pixel
the phrase "whats underneath them" refers to the source pixel]

typically feeding source pixel inside your Fragment is very very difficult.
our software renderer actually has the flexiblity of using source pixels in the fragment shader code. 


so in OpenGL, the "blending" stage is still very much a Fixed-function thing 

recall in our software rendering alpha blending formula we were using is. Recall everything is pre-multiplied

                // TODO(casey): Check this for math errors
                Alpha = Src_Alpha + (1 - Src_Alpha) * Dst_Alpha; 
                Color = Src_Color + (1 - Src_Alpha) * Dst_Color;


if we were doing non-premultiplied, we have 
             
                Color = Src_alpha * Src_Color + (1 - Src_alpha) * Dst_Color



but in the OpenGL BLending stage, we have a more general function, essentially we have 


                d_ = a * Src_Color + b * Dst_Color;

meaning d_ can come from any combination of Src_Color and Dst_Color, and we only have control over a and b 
so we get to pick a or b

so we can recreate the alpha blend we did
hence the following code:


                internal void OpenGLRenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {    
                    glViewport(0, 0, OutputTarget->Width, OutputTarget->Height);

                    glEnable(GL_TEXTURE_2D);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    
                    ...
                    ...
                }


and now we are reproducing our textures again.


34:28
so now our program runs super slowly, so we can do a temporary hack to make our program run faster 
so in our loaded_bitmap, we added a handle in the loaded_bitmap struct. 

                handmade_render_group.h

                struct loaded_bitmap
                {
                    void *Memory;
                    v2 AlignPercentage;
                    r32 WidthOverHeight;    
                    s32 Width;
                    s32 Height;
                    // TODO(casey): Get rid of pitch!
                    s32 Pitch;
    ----------->    u32 Handle;
                };

then we go back to section 20:08, where you see that we would set the bitmap handle if we run it for the first time 
so only for the first time, we can submit to the GPU our texture 

                case RenderGroupEntryType_render_entry_bitmap:
                {
                    render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                    Assert(Entry->Bitmap);

                    v2 XAxis = {1, 0};
                    v2 YAxis = {0, 1};
                    v2 MinP = Entry->P;
                    v2 MaxP = MinP + Entry->Size.x*XAxis + Entry->Size.y*YAxis;

                    if(Entry->Bitmap->Handle)
                    {
                        glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);
                    }
                    else
                    {
                        Entry->Bitmap->Handle = ++TextureBindCount;
                        glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);
        
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    }
                    
                    OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                } break;



37:49
and now you see, with this change our game runs back to original speed 


38:00
however, there is still one more problem with what we are seeing on screen.
Casey mentioned that since we are streaming, the audience probably can see the bug. 
but When Casey is watching it natively, its very very ugly. its a very noticeable bug.
thats cuz we are not doing bilinear filtering 

so we enable bilinear filtering 

                case RenderGroupEntryType_render_entry_bitmap:
                {
                    render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                    Assert(Entry->Bitmap);

                    v2 XAxis = {1, 0};
                    v2 YAxis = {0, 1};
                    v2 MinP = Entry->P;
                    v2 MaxP = MinP + Entry->Size.x*XAxis + Entry->Size.y*YAxis;

                    if(Entry->Bitmap->Handle)
                    {
                        glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);
                    }
                    else
                    {
                        Entry->Bitmap->Handle = ++TextureBindCount;
                        glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);
        
    --------------->    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    --------------->    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    }
                    
                    OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                } break;

The reason why its called GL_LINEAR instead of GL_BILINEAR is becuz this setting is the same no matter you are doing 
1D or 2D or 3D texture. 


46:03
So Casey mentiones another issue in our renderer, and they revolve around textures. 
essentially, this process right here, where we bind the textures, and submit it, that is something
that we have to device a better plan for. 

                case RenderGroupEntryType_render_entry_bitmap:
                {
                    render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                    Assert(Entry->Bitmap);

                    v2 XAxis = {1, 0};
                    v2 YAxis = {0, 1};
                    v2 MinP = Entry->P;
                    v2 MaxP = MinP + Entry->Size.x*XAxis + Entry->Size.y*YAxis;

                    if(Entry->Bitmap->Handle)
                    {
                        glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);
                    }
                    else
                    {
                        Entry->Bitmap->Handle = ++TextureBindCount;
                        glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);
        
    --------------->    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    --------------->    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    }
                    
                    OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                } break;


Cards are bad at submitting things asynchronously, and our games is set up to stream its asset incrementally. 
and that is the part that is gonna be problematic. 


49:01
so back to our original point. Recall at the beginning of this episode, Casey mentioned
that he will first demo how to set up the renderer code so that we can use the software renderer or the hardware renderer

so in section 14:02 and have the following code 

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    Platform = Memory->PlatformAPI;    

                    ...
                    ...

                    if(AllResourcesPresent(RenderGroup))
                    {
                        RenderToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer, &TranState->TranArena);
                    }

                    ...
                    ...
                }

and then inside the handmade_render_group.cpp, we defined the RenderToOutput(); function


                handmade_render_group.cpp 

                inline void RenderToOutput(platform_work_queue *RenderQueue,
                               render_group *RenderGroup, loaded_bitmap *OutputTarget,
                               memory_arena *TempArena)
                {
                    // TODO(casey): Don't do this twice?
                    SortEntries(RenderGroup, TempArena);

                    if(1) // RenderGroup->IsHardware)
                    {
                        Platform.RenderToOpenGL(RenderGroup, OutputTarget);
                    }
                    else
                    {
                        TiledRenderGroupToOutput(RenderQueue, RenderGroup, OutputTarget, TempArena);        
                    }
                }

the big thing with this approach is that this creates the concept that the game is round tripping to the round trip layer


so if you think about it, visually, it looks like this:

     ___________________________                 _______________                 _______________________
    | win32 layer               |               |               |               |   win32 layer         |
    |    GameUpdateAndRender    |-------------->|   Game code   |-------------->|      RenderToOpenGL   |
    |___________________________|               |_______________|               |_______________________|



so the platform layer has to be setup to service OpenGL rendering, in an arbiturary fashion from the game. 
so there is no way in the platform code to dictate when rendering happens through OpenGL. 

meaning our win32 layer doesnt control when does rendering happen anymore. We dont control that anymore. 
like previously in our software renderer, we call GameUpdateAndRender(); which calls into the game code,
then it pretty much never calls us about anything else. and then it returns to us the bitmap that it 
wants to us to be displayed. Essentially, the win32 does the final step to render the pixels onto the screen,
win32 has final say. 
the win32 layer could do anything with that buffer. we could have saved it to disc, we could have tested it. or anything we want. 
we can do a lot of planning about when to display that buffer.

so our previous approach is very clean and well decoupled. our game code doesnt need to know anything about the platform stuff.

but now with this approach, the game code needs to be aware of the platform openGL stuff.

50:28
so what Casey is proposing is to restore some of that process, by considering the render list as the output, rather then the bitmap.

so previously we have 

     ___________________________                 _______________                                 _______________________
    | win32 layer               |  Bitmap       |               |                               |                       |
    |    GameUpdateAndRender    |-------------->|   Game code   |------------------------------>|     win32 layer       |
    |___________________________|               |               |   passes the filled           |_______________________|
                                                |   fills the   |   bitmap back to win32
                                                |    bitmap     |
                                                |_______________|
                                              


now we have 


     ___________________________                 _______________                                 _______________________
    | win32 layer               |  Bitmap       |               |                               |                       |
    |    GameUpdateAndRender    |-------------->|   Game code   |------------------------------>|     win32 layer       |
    |___________________________|               |               |   passes the filled           |_______________________|
                                                |   fills the   |   bitmap back to win32
                                                |    bitmap     |
                                                |_______________|
                                                    |        ^
                                                    |        |
                                                    |        |  do a round trip here
                                                    |        |
                                                 ___v________|____
                                                |                 |
                                                |  asking win32   |  
                                                |    to do openGL |
                                                |    rendering    |
                                                |_________________|


and actually the we have to do the rendering round trip twice, becuz we do it once for the game rendering,
once the debug rendering. 

so the timing of when we want to do the rendering (meaning when do we trigger this function call, and when do openGL rendering gets called);
is completely out of the win32_s control


so if there is special knowledge about the platform, about when certain rendering commands should be issued, the platform code 
will now get much more complicated becuz it now has to do stuff, such as defer rendering that comes through 


52:50
so what Casey prefer is to treat the render group more like the bitmap. 

so previously, we were passing a bitmap, now we want to pass a render buffer 

then the win32 layer will then ask the rendering layer to do what it wants to do with it 

ether to dump the render buffer to a bitmap buffer or something else. 

     ___________________________                       _______________                                 _______________________
    | win32 layer               |  Render buffer      |               |                               |                       |
    |    GameUpdateAndRender    |-------------------->|   Game code   |------------------------------>|     win32 layer       |
    |___________________________|                     |               |   passes the filled           |_______________________|
                                                      |   fills the   |   render buffer back to win32       |
                                                      |    bitmap     |                                     |
                                                      |_______________|                                     |
                                                                                                       _____v___________________
                                                                                                      |                         |
                                                                                                      |   rendering layer       |
                                                                                                      |_________________________|

this way our code is much more decoupled, and a much cleaner separate.
it also allows us to do stuff like, capture the output of the game at the step 
where the "game code passes the render buffer back to win32", in a way that you can reproduce offline very trivially
and very concisely. 

it can lets you render it twice.. you can do anything you want. 

Casey says that its not a big deal, but its a cleaner separation of code. 


Q/A
1:05:49
how is Vsync controlled?

In OpenGL, VSync is controlled through extension calls, we will conver this later. 


1:10:16
is there any good reason for desktop GPUs to not have programmable blend modes? beucz i have seen that most mobile GPUs have that

Yes, there are alot of good reasons. 

so lets take our FrameBuffer, there are two ways to render this.

there is the Tiled Way and the non-tiled way.

                FrameBuffer
                 ___________________________
                |                           |
                |                           |
                |                           |
                |                           |
                |                           |
                |                           |
                |                           |
                |                           |
                |___________________________|

the tiled renderer looks like the one we wrote 

                 __________________________
                |        |        |        |
                |        |        |        |
                |________|________|________|
                |        |        |        |
                |        |        |        |
                |________|________|________|
                |        |        |        |
                |        |        |        |
                |________|________|________|

each core is working on a tile. This is the way how a lot of mobile GPU works.
mobile GPUs are usually tiled. 
Casey says he doesnt really know the reason for that, but he believes is becuz of the memory architectures. 
so what happens is that when you dont have giant memory controls, with special memory, 

it is a lot easier for each core to have one tile_s worth of frame buffer memory on the core, where it can just 
quickly read and write on the framebuffer. 

so on mobile, where there is less wattage, less memory bandwidth.


on regular GPU, they are typically non-tiled, they are more random accessed based.
