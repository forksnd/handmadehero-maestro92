Handmade Hero Day 243 - Asynchronous Texture Downloads

Summary:
talked about how we want to address the problem of sending the GPU the textures we want to use. 

mentioned the concept of texture swizzling. 

mention the idea that we need to somehow evict textures from the GPU if we do it on the CPU,
otherwise we ran out of memory.

mentioned the notion that we might want to change our asset storage into just storage for GPU handles,
and just have the software renderer use the memory allocation calls from the OS level if we dont really 
care about memory restrictions/optimizations for the software renderer.

mentioned how do we move the texture download work to a background thread in our code base.
planning the implement the code for next episode
demonstrated a naive solution, mentioned the problems with the naive solution

mentioned that to do openGL work on other threads, we need to create context for other threads 

created OpenGL context in our background threads. 

wrote the function to allocate(download); and deallocate textures to the GPU in our background threads


Keyword:
OpenGL, Texture, Graphics 

13:16
Casey resuming where he left off from, which is finishing Texture Downloads
what was happening before hand is that we have texutres on CPU, but we need somehow give it to the GPU memory 


             CPU Memory               GPU Memory
             ___________              ___________
            |           |            |           |
            | _________ |            | _________ |
            || Texture ||            || Texture ||
            ||_________||            ||_________||
            |           |            |           |
            |           |            |           |
            |           |            |           |
            |           |            |           |
            |___________|            |___________|


i think that is important when it comes to GPU programming is that we cannot send the whole texture downad to the 
GPU every time because it could be huge  

A single one cutscene with all the layers could be 6 megabytes, when uncompressed.

so other than having GPU memory limitation, so how much bandwidth that can go from the CPU to the GPU becomes important. 

we dont want to send down the textures every frame.

You can think about GPU memory as cache. Its a lot like how we think about the cache on the CPU.
you are trying to put the textures that are relevant to the current time in the game, (the current 20,30 frames);
on the GPU for that time. 

so in order to that, we need to have some opinion on what textures need to be in GPU memory,
and then have some concept on how to get them down there, and at some point remove them from GPU memory, to make room for other textures


16:40
like a regular cache, GPU memory can be sort of virtualized. in the sense that you can over subscribe it.
so the GPU memory is 1 GB, I could create 4 GB of textures and tell it to the card.

if I do that, what happens is that the driver would sit there, and it would basically try to do a best guess of which 
1 GB out of the 4 GB it will put on the card. 

and when we inevitiably reference a texture that wasnt in there, it would pick some memory to evict, and put someone else in. 
so with the driver sitting there, it will make the GPU memory act like the cache. 

This approach would be fine if this were an application where we dont care about hitting a hard frame limit. 
but here, we are talking about want to render 60 frames a second, we want to make sure that the driver is not doing this, not over
subscribe the GPU memory, and then we are knowledgeable about when our textures are going to be used, and then we get them down there
before the driver does the evicting


18:12
unfortunately, graphics card dont make this easy for us, so its kind of tricky. The swapping of that the GPU does (where 
    if we reference a texture that wasnt in the GPU memory, the driver evicts some memory and puts the new demanded texture in);
is often more efficient than you trying to do it yourself. 

the reason is for that is because you dont actually know the format in which the driver needs to store the textures, in order for the GPUs
to actually use them.


18:45
so there is this concept of "swizziling".
swizziling really just means re-arranging things. 

swizziling happens for textures too. this is because, graphics cards are not set up to be optimized for the case that we are 
using in handmade hero. 

In handmade hero, we are talking about abunch of 2D sprites stacked ontop, and standing right next to each other 


                 ___________________
                |                   |
         _______|_______            |
        |               |       ____|_______
        |               |      |            |
        |               |______|            |
        |               |      |            |
        |               |      |____________|
        |_______________|


and we are always talking about using the entire texture, in the order that it was drawn in exactly as an rectangle on the screen.

so the degree of swizziling you need to do to support this kind of operation is probably not huge. 

however, the point Casey wants to make is that, what cards normally try to do that, they dont use the whole texture 

remember those texture papers where they jam the entire texture of an object onto one giant sprite, and then when we render the object, 
different bits of the giant sprite will get mapped to different triangles of your mesh in your 3D model?

the usage is quite different from our usage in handmade hero where we are just rendering the entire sprite on our upright rectangele.


21:23
we think of our textures in memory as a line by line thing
but that is very bad for cache. 
     _______________ 
    |---------------|
    |---------------|
    |---------------|
    |---------------|
    |---------------|
    |_______________|

so for etter cache coherency, textures are stored in a swizzle pattern (the famous z patter, like JT_s interview question);

essentially we are storing our texture more "squareish"


24:38
Casey does mention that he doesnt know the exact format how textures swizzling is stored, 
and he mentioned that it probably differs from card to card. There are probably different swizzle formats depending on the card. 



24:45
what that means is that when you submit a texture to the OpenGL, what actually happens is that the OpenGL
will swizzle that texture. maybe the swizzing happens on the card, maybe it happens on the OpenGL runtime API (on the CPU side);.

so what that means is that if you give it to OpenGL, even if the driver hasnt give that texture to the card, it is possible that 
it has gone through the swizzling and remain in memory already swizzled. This wills save time on the transfer.


25:44
unfortunately, how to swizzle the texutre is not exposed to us. 



26:15
so for us, we may get away with it because we are using rectangular textures, which tends to get treated differntly in graphics cards.
Power of 2 are the more common one.

most textures you will use on a graphics cards that you will use for mapping onto a 3D model are power of 2.  
what that means is that the size is power of 2 in length. For example: 512 x 512, 256 x 256, 512 x 256

it can be rectangle, but the actual dimension is 2 to some power, and never anything else.
originally graphics card only support power of 2. it could literally not do any other shape. 

nowadays, the rectangular texture path might not go through the same kind of swizzling, and may not have the swizzling cost 
associated with them. That is because they are expecting it to be some kind of video donwload or something that you are trying to send 
over a thing that you are going to do rect linear blit, and you want to save time on the submission (skipping the swizzling); 
and you are not going to wrapping on a 3D model.

this is all stuff that only hardcore hardware guys or a serious GPU programmer would know. 


28:32
if you are a generalist, and you are not trying to squeeze every bit of performance out of a GPU, then you dont have to know this stuff.
but it is good to be aware.




29:21
all what we are dealing in handmade hero is that when it comes time to draw something, we look to see if we ever submitted
a bitmap. if we have never submitted it, we submit it. If we have, we just use the handle we previously used. 

there are two problems with this approach. 

1.  it is just too late. We are submitting the texture when we want to draw something. 
     
    if we have a giant texture that we want to send down that is like 1920 x 1080 for the cut scene.
    what this means is that it creates this bubble where the graphics card cant start rendering the current frame that it supposed to be 
    rendering until it finishes downloading the giant 8 MB textures. (1920 x 1080 * 4 - 8294400);


    what we rather do, is we rather have the graphics card be downloading the 1920 x 1080 bitmap, while we are rendering textures 
    that we do have the textures for. Then when it comes time to use that 1920 x 1080 texture, hopefully it would have finished 
    downloading. 

    essentially preloading.


    previously our asset preloading looks like 


                Disk ------------> Memory ------------> Screen 
                       (Thread);            (Instant)



    the process of loading from disk to memory is not instant, so we put it on a thread 

    now there is just an extra stage. the process of putting it from Thread to GPU is also not instant,
    so we want to put that on a thread. 


                                     -----> GPU --------
                                    |                   |
                           (Thread);|                   |
                                    |                   v

                Disk ------------> Memory ------------> Screen 
                       (Thread);            (Instant);


    apparently, there was a time where this was more or less impossible. Like if you try to thread a texture download, 
    heaven help you. 

    nowadays, most people try to stream the textures down to the GPU dynamically because essentially, it really reduces load time.
    also it drastically increases the amount of detail that you can have. 

    lets say you are in a open world RPG, when you are walking around, you need to be actively updating the textures around you. 


2.  the second thing we need to solve is that right now we are not removing anything from GPU texture memory.  
    depending on how many texture memory you have, we are just gonna keep on filling it up. 
    and eventually we might overflow it. 

    right now we have an eviction scheme for our CPU memory textures (as an asset);, when we evict an asset from the
    asset system memory storage, we never delete that corresponding texture on the GPU side. 


    so assume we have the situation where we have texture A and B in both in CPU, and we send them down to the GPU

             CPU Memory               GPU Memory
             ___________              ___________
            |           |            |           |
            | _________ |            | _________ |
            ||    A    ||  ------->  ||    A    ||
            ||_________||            ||_________||
            |           |            |           |
            | _________ |            | _________ |
            ||    B    ||  ------->  ||    B    ||
            ||_________||            ||_________||
            |___________|            |           |
                                     |           |
                                     |           |
                                     |           |
                                     |           |
                                     |___________|


now then we want to render with texture C, and we dont have any memory on the CPU side.
lets say we want to evict texture B to make space for texture D. However, we never tell the GPU that
texture B is no longer in use. So when we start rendering texture D, it gets added to the GPU memory 

             CPU Memory               GPU Memory
             ___________              ___________
            |           |            |           |
            | _________ |            | _________ |
            ||    A    ||  ------->  ||    A    ||
            ||_________||            ||_________||
            |           |            |           |
            | _________ |            | _________ |
            ||    C    ||  ------->  ||    B    ||
            ||_________||            ||_________||
            |___________|            |           |
                                     | _________ |
                                     ||    C    ||
                                     ||_________||
                                     |           |
                                     |___________|


so new textures that we send from CPU to the GPU will continue filling GPU memory until it runs out of memory 

and the driver has to keep backing store, which further fills up main memory. Then this continues until
you run out of mina memory, and you will just get gray images at that point. 


36:55
so what we need to do is that whenever we evict an asset on the CPU side, we need to evict it from the GPU side as well.
which brings us to another point that is worth mentioning


right now the way our CPU asset system work is that we essentially have a general purpose allocator. 

recall this is what we have, in which we can allocate variable size blocks of memory for anyone who wants to use it. 

                CPU
             ___________
            |     A     |
            |           | 
            |-----------| 
            |           | 
            |     D     |
            |           |
            |-----------| <------- unsuable parts cuz its too small
            |-----------|
            |           |
            |     C     |
            |           |
            |           |
            |           |
            |___________|

back then we just did a very simple, basic allocator. we didnt do any optimization, or performance tweaks.
Casey mentioned that we left it the way it is until we have more insight on our needs cuz you dont want to 
over engineer something like this. 

essentially we didnt do any premature optimizations.





38:54
the GPU asset storage is also a general purpose allocator. The GPU driver micromanages this. 
it manages regions, and the similar stuff that we did. 


                GPU 
             ___________
            |     A     |
            |           | 
            |-----------| 
            |           | 
            |     D     |
            |           |
            |-----------| <------- unsuable parts cuz its too small
            |-----------|
            |           |
            |     C     |
            |           |
            |           |
            |           |
            |___________|


if we did not do any software rendering, we could turn our CPU asset store into a fixed size, free list based allocator,
and it would be 100% efficient. This is because all what we would have to store is the handle that the GPU gave us back,
for where we put the actual texture. 

what that means is that if in the future, we dont care about how our asset store performs under the software renderer,
we could change the CPU asset store to a fixed-size allocator, that just holds tha handle from the GPU side, 

then we would just use virtual alloc and virtual free on the OS layer, to allocate and free the things we need on the 
software renderer. 

[If you think about the asset system for the software renderer is just an optimization for the memory storage.
if we dont care about the software renderer memory restriction, we can just use the OS virtual alloc and free.]


40:28
in some sense this might be the correct approach because, there is no point in stacking a CPU side general purpose allocator 
ontop of a GPU driver side general purpose allocator. If the driver is already gonna do it, and we cant stop it, we might as well



41:06
so now what we want to do is to somehow download these textures on a separate thread, and then use them in the main thread.


42:36
previously we had something like, where we just bind the texture on demand, and hope that it wont be too late.
Essentially we werent preloading anything on a separate thread. 

                handmade_opengl.cpp 

                global_variable u32 TextureBindCount = 0;
                internal void OpenGLRenderCommands(game_render_commands *Commands, s32 WindowWidth, s32 WindowHeight)
                {    
                    ...
                    ...
                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        ...
                        switch(Header->Type)
                        {
                            ...
                            case RenderGroupEntryType_render_entry_bitmap:
                            {
                                ...
                                ...

                                if(Entry->Bitmap->Handle)
                                {
                                    glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);
                                }
                                else
                                {
                                    Entry->Bitmap->Handle = ++TextureBindCount;
                                    glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);

                                    glTexImage2D(GL_TEXTURE_2D, 0, OpenGLDefaultInternalTextureFormat, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);
                    
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                                }
                                
                                OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                            } break;

                            ...
                            ...
                        }
                    }
                    ...
                }


in particular, the part that Casey is most worried about is

                glTexImage2D(GL_TEXTURE_2D, 0, OpenGLDefaultInternalTextureFormat, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

this line is taking our texture, Entry->Bitmap->Memory, subtmit that down to the graphics card, and we dont know how long is that 
gonna take.  

we also know that, in a lot of the modern graphics card, everything probably from GE-Force 500 model and up, they
have this thing call copy engines. Copy engines are components on the graphics card that does nothing other than 
transfering stuff like textures over to the graphics card, Asynchronousl, while the graphics card is doing other things. 


we know that there is plent of resource for us to take advantage of on lots of different modern hardwhere.
also these are resources on the graphics card that allows us to do the texture transfer while not interrupting the rendering of our frame.
these resources is setup to do exactly this. 

so what we want to do is to move this glTexImage2D to somwhere, where it can happen in an Asynchronous fashion. 

this may turn out a lot more difficult than you think, mainly because the OpenGL API is not designed with tasks like this in mind. 


44:51
so recall the way we were loading assets Asynchronously on the CPU side is below:
in which we create this load_asset_work.

                handmade_asset.cpp

                internal void ,LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Immediate)
                {
                    TIMED_FUNCTION();

                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value)
                    {
                        if(AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
                           AssetState_Unloaded)
                        {
                            task_with_memory *Task = 0;

                            if(!Immediate)
                            {
                                Task = BeginTaskWithMemory(Assets->TranState, false);
                            }

                            if(Immediate || Task)        
                            {
                                asset *Asset = Assets->Assets + ID.Value;
                                ...

                                loaded_bitmap *Bitmap = &Asset->Header->Bitmap;            
                                ...

                                load_asset_work Work;
                                ...
                                if(Task)
                                {
                                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work, NoClear());
                                    *TaskWork = Work;
                                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                                }
                                else
                                {
                                    LoadAssetWorkDirectly(&Work);
                                }
                            }
                            else
                            {
                                Asset->State = AssetState_Unloaded;
                            }
                        }
                        else if(Immediate)
                        {
                            // TODO(casey): Do we want to have a more coherent story here
                            // for what happens when two force-load people hit the load
                            // at the same time?
                            asset_state volatile *State = (asset_state volatile *)&Asset->State;
                            while(*State == AssetState_Queued) {}
                        }
                    }    
                }



inside the load_asset_work(), we would be calling LoadAssetWorkDirectly();

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
                {
                    load_asset_work *Work = (load_asset_work *)Data;

                    LoadAssetWorkDirectly(Work);

                    EndTaskWithMemory(Work->Task);
                }

and inside the LoadAssetWorkDirectly(); we can see all it does it reads the chunk our of the file 

                Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
then it will finish any operation to make the asset usable


-   full code below:

                handmade_asset.cpp                

                internal void LoadAssetWorkDirectly(load_asset_work *Work)
                {
                    TIMED_FUNCTION();

                    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
                    if(PlatformNoFileErrors(Work->Handle))
                    {
                        switch(Work->FinalizeOperation)
                        {
                            case FinalizeAsset_None:
                            {
                                // NOTE(casey): Nothing to do.
                            } break;

                            case FinalizeAsset_Font:
                            {
                                loaded_font *Font = &Work->Asset->Header->Font;
                                hha_font *HHA = &Work->Asset->HHA.Font;
                                for(u32 GlyphIndex = 1; GlyphIndex < HHA->GlyphCount; ++GlyphIndex)
                                {
                                    hha_font_glyph *Glyph = Font->Glyphs + GlyphIndex;

                                    ...
                                    Font->UnicodeMap[Glyph->UnicodeCodePoint] = (u16)GlyphIndex;
                                }
                            } break;
                        }
                    }

                    CompletePreviousWritesBeforeFutureWrites;

                    if(!PlatformNoFileErrors(Work->Handle))
                    {
                        ZeroSize(Work->Size, Work->Destination);
                    }

                    Work->Asset->State = Work->FinalState;
                }




46:14
so you can see that we set our selves for success, all we have to do is to add the asset type 

                handmade_asset.cpp

                enum finalize_asset_operation
                {
                    FinalizeAsset_None,
                    FinalizeAsset_Font,
    ----------->    FinalizeAsset_Bitmap,
                };

and in the LoadAssetWorkDirectly(); function, we add the case for the FinalizeAsset_Bitmap


                handmade_asset.cpp                

                internal void LoadAssetWorkDirectly(load_asset_work *Work)
                {
                    TIMED_FUNCTION();

                    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
                    if(PlatformNoFileErrors(Work->Handle))
                    {
                        switch(Work->FinalizeOperation)
                        {
                            case FinalizeAsset_None:
                            {
                                ...
                            } break;

                            case FinalizeAsset_Font:
                            {
                                ...
                            } break;

                            case FinalizeAsset_Bitmap:
                            {
                                ...
                                ...
                            } break;
                        }
                    }

                    CompletePreviousWritesBeforeFutureWrites;

                    if(!PlatformNoFileErrors(Work->Handle))
                    {
                        ZeroSize(Work->Size, Work->Destination);
                    }

                    Work->Asset->State = Work->FinalState;
                }



47:46
so on first glance, you might think that we can just do 


                case FinalizeAsset_Bitmap:
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, OpenGLDefaultInternalTextureFormat, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                } break;


however, the problem with this approach is that this glTexImage2D function in LoadAssetWorkDirectly(); function
has no context that why it is being called. 
because if you remember, when we talked about how OpenGL works, there is the concept of the OpenGL context
associated with every thread. When you issue one of these calls like glTexImage2D(); what it is doing is that it is looking up 
in its thread local storage about what context that is, and then executing things on that context. 

OpenGL does not allow the same context, to be active on two thread at once. 


so the main thread who is calling all these GL drawing operations, cant be using same context as glTexImage2D();
in a background asset loading thread. 

so we cant just call glTexImage2D(); right here and expect it totally to work. it will totally fail. 


49:20
Casey demonstarting how this will fail. 

so when we come through the OpenGLRenderCommands(); function, we will just always Bind the texture              
that just means that if the texture isnt there, we just wont draw anything. 

                handmade_opengl.cpp 

                global_variable u32 TextureBindCount = 0;
                internal void OpenGLRenderCommands(game_render_commands *Commands, s32 WindowWidth, s32 WindowHeight)
                {    
                    ...
                    ...
                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        ...
                        switch(Header->Type)
                        {
                            ...
                            case RenderGroupEntryType_render_entry_bitmap:
                            {
                                ...
                                ...
                                glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);     
                                
                                OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                            } break;

                            ...
                            ...
                        }
                    }
                    ...
                }


and the LoadAssetWorkDirectly(); looks like below:

                handmade_asset.cpp                

                internal void LoadAssetWorkDirectly(load_asset_work *Work)
                {
                    TIMED_FUNCTION();

                    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
                    if(PlatformNoFileErrors(Work->Handle))
                    {
                        switch(Work->FinalizeOperation)
                        {
                            case FinalizeAsset_None:
                            {
                                ...
                            } break;

                            case FinalizeAsset_Font:
                            {
                                ...
                            } break;

                            case FinalizeAsset_Bitmap:
                            {
                            case FinalizeAsset_Bitmap:
                            {
                                glTexImage2D(GL_TEXTURE_2D, 0, OpenGLDefaultInternalTextureFormat, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);

                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                            } break;
                        }
                    }

                    CompletePreviousWritesBeforeFutureWrites;

                    if(!PlatformNoFileErrors(Work->Handle))
                    {
                        ZeroSize(Work->Size, Work->Destination);
                    }

                    Work->Asset->State = Work->FinalState;
                }

50:11
other than the problems mentioned above, there are 2 more problems with the code in LoadAssetWorkDirectly();

1.  we dont want be calling gl code in the middle of our LoadAssetWorkDirectly() code path.
    for all we know, we may not even be running on OpenGL. So we need this to be a little more platform separted

2.   also we want to make an OpenGL Context that glTexImage2D(); can use to download the texture.


52:02
so recall when we created the context for our main thread, we had this:

                win32_handmade.cpp

                internal void Win32InitOpenGL(HWND Window)
                {

                    HGLRC OpenGLRC = wglCreateContext(WindowDC);
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        b32 ModernContext = false;
                        
                        wgl_create_context_attribts_arb *wglCreateContextAttribsARB = (wgl_create_context_attribts_arb *)wglGetProcAddress("wglCreateContextAttribsARB");
                        if(wglCreateContextAttribsARB)
                        {
                            // NOTE(casey): This is a modern version of OpenGL
                            int Attribs[] =
                            {
                                ...
                                ...
                            };
                                
                            HGLRC ShareContext = 0;
                            HGLRC ModernGLRC = wglCreateContextAttribsARB(WindowDC, ShareContext, Attribs);
                            if(ModernGLRC)
                            {
                                if(wglMakeCurrent(WindowDC, ModernGLRC))
                                {
                                    ModernContext = true;
                                    wglDeleteContext(OpenGLRC);
                                    OpenGLRC = ModernGLRC;
                                }
                            }
                        }
                        else
                        {
                            // NOTE(casey): This is an antiquated version of OpenGL
                        }

                        OpenGLInit(ModernContext);
                        
                        wglSwapInterval = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
                        if(wglSwapInterval)
                        {
                            wglSwapInterval(1);
                        }
                    }
                }

one important line in here is the "ShareContext" variable, 

                HGLRC ShareContext = 0;
                HGLRC ModernGLRC = wglCreateContextAttribsARB(WindowDC, ShareContext, Attribs);
                if(ModernGLRC)
                {
                    if(wglMakeCurrent(WindowDC, ModernGLRC))
                    {
                        ModernContext = true;
                        wglDeleteContext(OpenGLRC);
                        OpenGLRC = ModernGLRC;
                    }
                }

so when we call the wglCreateContextAttribsARB(); function, you can see that we pass in the "ShareContext".
the "ShareContext" is saying that when I create a context, I may create another context that shared memory with this context.

so if I download a texture to one of them, it is accessible to both. And that is exactly what we want to do.


52:28
so all what we want to do is to start setting ourselves up to calling glTexImage2D(); on another thread, is to make sure that 
anythread that could call glTexImage2D(); has a context that is shared with the main context, and all of them could be downloading
textures, and we wouldnt necessary have a problem. 

whether the driver freaks out if we have 4 threads trying to download different textures at the same time , we dont know. But there is no 
technical reason why we shouldnt be able to do it. If the driver is written properly, then that would work. 




53:18
so if we look at the CreateThread(); function.

                win32_handmade.cpp

                internal void Win32MakeQueue(platform_work_queue *Queue, uint32 ThreadCount)
                {
                    Queue->CompletionGoal = 0;
                    Queue->CompletionCount = 0;
                    
                    Queue->NextEntryToWrite = 0;
                    Queue->NextEntryToRead = 0;

                    uint32 InitialCount = 0;
                    Queue->SemaphoreHandle = CreateSemaphoreEx(0,
                                                               InitialCount,
                                                               ThreadCount,
                                                               0, 0, SEMAPHORE_ALL_ACCESS);
                    for(uint32 ThreadIndex = 0;
                        ThreadIndex < ThreadCount;
                        ++ThreadIndex)
                    {
                        DWORD ThreadID;
                        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Queue, 0, &ThreadID);
                        CloseHandle(ThreadHandle);
                    }
                }

the CreateThread(); function takes the ThreadProc(); as the main routine.
so essentially each thread is waiting for work to do. 

                win32_handmade.cpp

                DWORD WINAPI ThreadProc(LPVOID lpParameter)
                {
                    platform_work_queue *Queue = (platform_work_queue *)lpParameter;

                    u32 TestThreadID = GetThreadID();
                    Assert(TestThreadID == GetCurrentThreadId());
                    
                    for(;;)
                    {
                        if(Win32DoNextWorkQueueEntry(Queue))
                        {
                            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
                        }
                    }

                //    return(0);
                }

if it finds work to do on the Queue, it will do it. 
so one of the work that it will do is to call FinalizeAsset_Bitmap. which means anythread that is created 
on our work system, needs to have a OpenGL context, asscociated with it, otherwise the glTexImage2D(); call will fail.

54:03
so before any thread goes into service, we will have to create an OpenGL context for the thread. 

notice that we pass in a flag of "NeedsOpenGL". This is because we are only doing the asset downloading 
in the low priority threads. the high priority threads dont need it. 

                win32_handmade.cpp

                DWORD WINAPI ThreadProc(LPVOID lpParameter)
                {
                    platform_work_queue *Queue = (platform_work_queue *)lpParameter;

                    u32 TestThreadID = GetThreadID();
                    Assert(TestThreadID == GetCurrentThreadId());

                    if(Queue->NeedsOpenGL)
                    {
                        Win32CreateOpenGLContextForWorkerThread();
                    }

                    for(;;)
                    {
                        if(Win32DoNextWorkQueueEntry(Queue))
                        {
                            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
                        }
                    }

                //    return(0);
                }



so once we created OpenGL context for these threads, they will then be able to download the textures to the GPU. 
however, as mentioned previously, we dont actually want the threads to call glTexImage2D() in the LoadAssetWorkDirectly(); in 
the handmade_asset.cpp file because handmade_asset is not a platform specifc file. So we will solve this later



55:47
so now we will write the Win32CreateOpenGLContextForWorkerThread(); function. 

                internal void Win32CreateOpenGLContextForWorkerThread(void)
                {
                    if(wglCreateContextAttribsARB)
                    {
                        HDC WindowDC = GlobalDC;
                        HGLRC ShareContext = GlobalOpenGLRC;
                        HGLRC ModernGLRC = wglCreateContextAttribsARB(WindowDC, ShareContext, Win32OpenGLAttribs);
                        if(ModernGLRC)
                        {
                            if(wglMakeCurrent(WindowDC, ModernGLRC))
                            {
                                // TODO(casey): Fatal error?
                            }
                        }
                    }
                }

Casey was initially unsure about what to pass in as the WindowDC argument. Since we are creating these context 
for threads that will be doing mostly asset loading in the back, it will never be drawing to the window. 
so Casey was unsure whether it is more appropriate to just use 0 for it. 

so Casey went online and checked for a particular presentation that he had in mind 

http://on-demand.gputechconf.com/gtc/2012/presentations/S0356-GTC2012-Texture-Transfers.pdf

this presentation is on texture transfers. 
Basically anytime you program a GPU, you want to see what the graphics vendor people say, cuz that is usually the best option. 

so on slide page 13, you can see that in their code snippet, they are passing winDC. so we will do the same.




1:16:42
So Casey going back to the LoadAssetWorkDirectly(); function to move the actual call of 
glTexImage2D(); out of handmade_asset.cpp by introducing a callback 




first Casey defined the callback in the handmade_platform.h file 

so we created two function pointers, AllocateTexture and DeallocateTexture

                typedef struct platform_api
                {
                    platform_add_entry *AddEntry;
                    platform_complete_all_work *CompleteAllWork;
                    
    ----------->    platform_allocate_texture *AllocateTexture;
    ----------->    platform_deallocate_texture *DeallocateTexture;

                    ...
                    ...

                } platform_api;


then in the handmade_platform.h function, we add the function pointers. 

                #define PLATFORM_ALLOCATE_TEXTURE(name) void *name(u32 Width, u32 Height, void *Data)
                typedef PLATFORM_ALLOCATE_TEXTURE(platform_allocate_texture);

                #define PLATFORM_DEALLOCATE_TEXTURE(name) void name(void *Texture)
                typedef PLATFORM_DEALLOCATE_TEXTURE(platform_deallocate_texture);




1:20:06
then in LoadAssetWorkDirectly(); we call the Platform.AllocateTexture(); function.


                internal void LoadAssetWorkDirectly(load_asset_work *Work)
                {
                    TIMED_FUNCTION();

                    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
                    if(PlatformNoFileErrors(Work->Handle))
                    {
                        switch(Work->FinalizeOperation)
                        {
                            case FinalizeAsset_None:
                            {
                                // NOTE(casey): Nothing to do.
                            } break;

                            case FinalizeAsset_Font:
                            {
                                ...
                            } break;

                            case FinalizeAsset_Bitmap:
                            {
                                loaded_bitmap *Bitmap = &Work->Asset->Header->Bitmap;
                                Bitmap->TextureHandle = 
                                    Platform.AllocateTexture(Bitmap->Width, Bitmap->Height, Bitmap->Memory);
                            } break;
                        }
                    }
                    ...
                }




1:21:19
finally in win32_handmade.cpp, we defined the two functions 

notice that once we finished submitting the download command to OpenGL Driver, we unbind our texture 
by calling glBindTexture(GL_TEXTURE_2D, 0);

                win32_handmade.cpp

                PLATFORM_ALLOCATE_TEXTURE(Win32AllocateTexture)
                {
                    GLuint Handle;
                    glGenTextures(1, &Handle);
                    glBindTexture(GL_TEXTURE_2D, Handle);
                    glTexImage2D(GL_TEXTURE_2D, 0, 
                        OpenGLDefaultInternalTextureFormat, 
                        Width, Height, 0,
                        GL_BGRA_EXT, GL_UNSIGNED_BYTE, Data);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    
                    glBindTexture(GL_TEXTURE_2D, 0);
                    
                    Assert(sizeof(Handle) <= sizeof(void *));
                    return((void *)Handle);
                }

then we also have the DeallocateTexture function.

                PLATFORM_DEALLOCATE_TEXTURE(Win32DeallocateTexture) 
                {
                    GLuint Handle = (GLuint)Texture;
                    glDeleteTextures(1, &Handle);
                }



1:29:39
of course, we want to call PLATFORM_DEALLOCATE_TEXTURE when we evict our texture 

so when we are in AcquireAssetMemory(); function, we would call Platform.DeallocateTexture();
to tell the GPU to evict that texture memory. as you can see, we are only doing this in the case of a texture

                internal asset_memory_header * AcquireAssetMemory(game_assets *Assets, u32 Size, u32 AssetIndex, asset_header_type AssetType)
                {
                    TIMED_FUNCTION();

                    asset_memory_header *Result = 0;

                    BeginAssetLock(Assets);

                    asset_memory_block *Block = FindBlockForSize(Assets, Size);
                    for(;;)
                    {
                        if(Block && (Size <= Block->Size))
                        {
                            ...
                            ...
                            break;
                        }
                        else
                        {
                            for(asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;
                                Header != &Assets->LoadedAssetSentinel;
                                Header = Header->Prev)
                            {
                                asset *Asset = Assets->Assets + Header->AssetIndex;
                                if((Asset->State >= AssetState_Loaded) &&
                                   (GenerationHasCompleted(Assets, Asset->Header->GenerationID)))
                                {
                                    ...
                                    ...
                                    
                                    if(Asset->Header->AssetType == AssetType_Bitmap)
                                    {
                ---------------->      Platform.DeallocateTexture(Asset->Header->Bitmap.TextureHandle);
                                    }
                                    ...
                                    ...
                                }
                            }

Q/A 

1:42:43
is it possible to tell openGL which graphics card to use if you have more than one?

the short answer is no.
the long naswer is depend if they are fron the same manufacturer. if they are, you may be able to.

for example, if you have an ati card and an nvidia card, only one of the drivers will be loaded/mapped to you 
when you are talking to OpenGL, and there is no way to say :"give me the OpenGL context corresponding to that card"

so whoever is setup as the primary display driver is the one that you will be talking to. 

if you have two graphics card in a machine, and they are from the same vendor, then its the same driver
anyway. In that case, openGL will by default, just do whatever it is going to do. 
if the user set it up as SLI (Scalable Link Interface);, it will do SLI



1:45:28
is it really a good idea to keep the DC around forever? what if the user changes its monitor config while handmade hero is running?
 
if the DC has to get rebuilt, the openGL context will no longer be valid. So that is the least of our worries. 



1:57:02
at the end of the day, all the complexitiy for programming graphics on windows is in the driver. Even if you use vulcan,
its still just about babysitting the driver. Graphics programming is mostly about baby sitting the driver. 

