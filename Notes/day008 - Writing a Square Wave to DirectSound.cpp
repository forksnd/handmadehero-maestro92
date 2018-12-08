Handmade Hero Day 008 - Writing a Square Wave to DirectSound

Summary:
wrote the function fill up the circular buffer with our audio data 
wrote a lot of details of managing th PlayCursor and WriteCursor of the circular buffer

Keyword:
audio, DirectSound

2:02
C++ linkage, C++ across the dll boundary

                struct foo
                {
                    int32 x;
                    int16 y;
                    void Bar(int C);
                };

                void foo::Bar(int C)
                {

                }

foo:: is the scoping operator 



6:35
Casey talking about what virtual does.

                struct foo
                {
                    // insert vtable pointer 

                    int32 X;
                    int16 Y;

                    virtual void Bar(int C);
                };


it will cause the compiler to insert a vtable pointer
a vtable is essentially a table that contains a bunch of function pointers 







19:08
so again back to our graph from day 007. we have our 2 second buffer.

                                 software 
                                write cursor

                                     |
                                     |
                                     V
            |______________________________________________________________|
            
            0                                                              n


in order for us to write into this buffer, directX actually knows we are doing a circular buffer, 
so it allows you to lock a region by specifying an offset, 

recall this is a system buffer, windows wants us to know when are we writing to it. so we need to issue a lock call to it.


when you do the lock call, windows can potentially give you 2 regions 
for example if you specify a lock start and a size. if it warps around, you will get 2 regions. 

so essentially, we need to handle both possible cases 

case 1

                                  start                  end
                                    |.......region1.......|
                                    |                     |
                                    |                     |
                                    |                     |
                                    v                     v
            |______________________________________________________________|
            
            0                                                              n

case 2 

                                                  start 
                              end                   |.......region1....... 
            .....region2......|                     |
                                                    |
                                                    |
                                                    v
            |______________________________________________________________|
            
            0                                                              n



22:06
Casey proceeds to write that code to lock regions 

-   recall that we have a sound buffer that is interleaved 

                16 bits
                LEFT RIGHT LEFT RIGHT LEFT RIGHT LEFT RIGHT 


-   Casey first writes the loop to fill up the buffer    
    we are just assuming we have WriteCursor and BytesToWrite
    

-   you get the two regions by calling 
{
                VOID *Region1;
                DWORD Region1Size;
                VOID *Region2;
                DWORD Region2Size;
                if(SUCCEEDED(GlobalSecondaryBuffer->Lock(WritePointer, BytesToWrite,
                                                             &Region1, &Region1Size,
                                                             &Region2, &Region2Size,
                                                             0)))
}
    the lock function populates the values for your two regions.
    note that Region1Size and Region2Size are in bytes. 


-   so we are writing to 2 regions in the two for loop. In each of the for loop,
    we do a calculation for the Region1SampleCount.
    
                DWORD Region1SampleCount = Region1Size/BytesPerSample;
   
    BytesPerSample is just both left and right channel 

                [LEFT RIGHT] LEFT RIGHT LEFT RIGHT 

    so we have.
                int BytesPerSample = sizeof(int16)*2;


-   then we loop through the for loop 

                for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
                {
                    int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
                    *SampleOut++ = SampleValue;
                    *SampleOut++ = SampleValue;
                }

    we are calling *SampleOut++ = SampleValue; twice?
    that is cuz we are essentially doing 

                    *SampleOut++ = LEFT;
                    *SampleOut++ = RIGHT;

    so we are writing both the left and right channel


-   then at the end we just unlock. should be pretty straightforward 


                int CALLBACK
                WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {
                    ...
                    ...

                    GlobalRunning = true;
                    while(GlobalRunning)
                    {

                        ...
                        ...
                        DWORD WriteCursor = ;
                        DWORD BytesToWrite = ;
                        ...
                        ...

                        VOID *Region1;
                        DWORD Region1Size;
                        VOID *Region2;
                        DWORD Region2Size;
                        if(SUCCEEDED(GlobalSecondaryBuffer->Lock(WritePointer, BytesToWrite,
                                                                 &Region1, &Region1Size,
                                                                 &Region2, &Region2Size,
                                                                 0)))
                        {
                            // TODO(casey): assert that Region1Size/Region2Size is valid

                            // TODO(casey): Collapse these two loops
                            DWORD Region1SampleCount = Region1Size/BytesPerSample;
                            int16 *SampleOut = (int16 *)Region1;
                            for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
                            {
                                int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
                                *SampleOut++ = SampleValue;
                                *SampleOut++ = SampleValue;
                            }

                            DWORD Region2SampleCount = Region2Size/BytesPerSample;
                            SampleOut = (int16 *)Region2;
                            for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
                            {
                                int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
                                *SampleOut++ = SampleValue;
                                *SampleOut++ = SampleValue;
                            }

                            GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
                        }
                    }
                }   



35:33
we then move on to calculate WritePointer and BytesToWrite

-   notice the windows api call 

                SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

    gives you back the PlayCursor and WriteCursor. 


                    PlayCursor           WriteCursor
                    
                        |                    |
                        |                    |
                        V                    V
            |______________________________________________________________|
            
            0                                                              n

    the Hardware, sound card or whoever is controlling it is actually looking at this buffer at a particular 
    location, which is the PlayCursor.  

    becuz of how the hardware works or kernel mixing work, this is all asynchronous. The sound will be playing 
    whether we like it or not.

    so its not safe for us to start writing at the PlayCursor location. Just like how we mentioned in day007,
    there will be delays and latencies in writing, so we need to be a bit ahead of the PlayCursor. 

    and that is what the WriteCursor for. That pretty much leads the PlayCursor. 

-   the thing is, according to the MSDN spec, When we lock our region, WriteCursor does not get incremented or changed.
    which means this WriteCursor is doest do shit, and we have to keep track the WriteCursor ourselves. 
    We dont actually use WriteCursor at all.


-   If you also notice from the section above, inside the two for loops where we are filling in regions, we call

                
                for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
                {
                    int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
                    *SampleOut++ = SampleValue;
                    *SampleOut++ = SampleValue;
                }

    we are incrementing RunningSampleIndex in there. And that is how we keep track of our real WriteCursor. 

-   Mind you that RunningSampleIndex just goes on infinitely. The way we calculate the WriteCursor position in the buffer
    is just using the mod operator 

                DWORD ByteToLock = RunningSampleIndex*BytesPerSample % SecondaryBufferSize;

-   we also calculate the BytesToWrite;
    when we consider BytesToWrite, is basically however far we are from the PlayCursor. We dont ever want to invade 
    PlayCursor territory. We dont want to write past PlayCursor, or overwrite content in PlayCursor


    for example 
    assuming our audio data is just increasing integers. PlayCursor wants to play the audio data in order.
    
    here the ByteToLock may potentially invades into PlayCursor territory, 
    we will then overwrite what was valid for PlayCursor.
    Therefore we want to cap the bytes to Write at PlayCursors position

                                      
             ByteToLock      PlayCursor 
                 |              |1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17                 
            19 20|              |
                 |              |
                 |21 22 23 24 25 26 27 28               
                 v              v                
            |______________________________________________________________|
            
            0                                                              n



    pretty much we need to know if PlayCursor is 
    in front of or behind the ByteToLock

    if together pretty much means PlayCursor has caught up, meaning there are no more valid data
        for PlayCursor to play, so ByteToLock got the whole buffer to write to.

                    PlayCursor           
                    ByteToLock
                        |..................................................                
            ............|                
                        v                
            |______________________________________________________________|
            
            0                                                              n

                BytesToWrite = SecondaryBufferSize;


    if in front, PlayCursor has all that valid data to play: 1 2 3....
    BytesToLock has all the ..... emtpy space to fill

                    PlayCursor                             ByteToLock              
                                                        
                        |1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 |...........                
            ............|                                       |
                        v                
            |______________________________________________________________|
            
            0                                                              n

                BytesToWrite = (SecondaryBufferSize - ByteToLock);
                BytesToWrite += PlayCursor;


    if behind, same idea.

                    ByteToLock                       PlayCursor             
                                                       
                        |............................|1 2 3 4 5 6 7 8 9 10 11                
            12 13 14 15 |                            |
                        v                            v           
            |______________________________________________________________|
            
            0                                                              n

                BytesToWrite = PlayCursor - ByteToLock;

    should be pretty straightforward.


-   full code below

                uint32 RunningSampleIndex = 0;


                // NOTE(casey): DirectSound output test
                DWORD PlayCursor;
                DWORD WriteCursor;
                if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD ByteToLock = RunningSampleIndex*BytesPerSample % SecondaryBufferSize;
                    DWORD BytesToWrite;
                    if(ByteToLock == PlayCursor)
                    {
                        BytesToWrite = SecondaryBufferSize;
                    }
                    else if(ByteToLock > PlayCursor)
                    {
                        BytesToWrite = (SecondaryBufferSize - ByteToLock);
                        BytesToWrite += PlayCursor;
                    }
                    else
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }



                    ............................................................
                    ............ the two for loops to fill .....................
                    ............ region1 and region2 ...........................
                    ............................................................

                }




56:54
Casey calling the windows API to play the sound 

                int CALLBACK
                WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {

                    ...
                    ...


                    if(!SoundIsPlaying)
                    {
                        GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                        SoundIsPlaying = true;
                    }
                    
                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                               Dimension.Width, Dimension.Height);

                }



1:23:02
what is the acceptable latency for games? How far advance should we write the cursor.
it depends. Its very difficult to figure it out on some platforms. On mac, you can do it exactly. 
cuz they can tell you exactly when your video frame will be displayed out to the hdml port. so you can actually
your audio is hard synced.

on windows, atleast on XP, you cant know when exactly the vsync happens unless you do OpenGL. 

unless you know when exactly the vsync, you dont know how much latency you will have 