Handmade Hero Day 007 - Initializing DirectSound

Summary:
talked about how to output sound in direct sound, which is using a circular buffer.

explained the different betwen sound card hardware curso and our software write cursor.
the software write cursor is where we want to write to in the circular buffer.

briefly mentioned how the game loop works, essentailly the current frame calculates the frame data
and audio data for the next frame, and in terms of audio output, we want the line up the software write cursor
with the page flip. 

went to windows api to set up DirectSound

Keyword:
sound, audio, game loop, windows api.

for the first 16 minutes, fixed a bunch of windows api, windows function calls and stuff. Nothing interesting.


18:10
the way that direct sound output works is that you allocate a circular buffer of sound. 



            |______________________________________________________________|
            
            0                                                              n

                                    2 seconds


so what we will do, is just to allocate a sound buffer, that is lets say 2 seconds long.
audio nowadays on most things are 48000 hertz, or 48 khz

so 2 seconds is just 96 khz 

so if we are doing 60 frames per scond, then we are doing 48000 hertz / 60 = 800 samples per frame




20:10
so the hardware is gonna have a cursor of where it is playing 

                    hardware        software 
                     cursor         write cursor

                        |             |
                        |             |
                        V             V
            |______________________________________________________________|
            
            0                                                              n

it is important to know that we cant start the writing at the hardware cursor 
writing to (the xxxxxxxxxxx location);

if we do so, the hardware cursor will get pass before we finish writing, becuz there is a bunch of 
latency in the writing.

so our software write cursor always have to be ahead of the hardware curosr




21:14
it is also important to consider how the sound works with video data 

essentially, we want our software write cursor to sync with the video flip

                        
                    hardware        software 
                     cursor         write cursor

                        |             |
                        |             |
                        V             V
            |______________________________________________________________|
            
            0           ^             ^             ^                       n
                        |---frame1----|---frame2----|
                        |             |             |


so during frame1 game logic, we will be calculating the video data for frame2, as well as the audio data for frame2

which means that we will want our software write cursor to line up where we expect the page flip to happen

(page flip means frame1 video data flipping to frame2 video data)


the challenge is that it is very hard to do that cuz the page flipping happens usually on the vblank in the old days,
in the old days, but could happen anytime now with NVIDA crazy stuff.


so essentially thats the way to think about the game loop.
each frame you calculate video data as well as writing the audio data for next frame.



24:45
Casey writing out the main steps

-   apparently you used to be able to write to the sound card,
but they dont let you do that anymore.
"the primary buffer" needs to be created cuz windows wants you to set the mode on the sound card 

the primary buffer is something we will never touch, we only using it to set the mode. 
kind of the result of some windows legacy code stuff.

-   "the secondary buffer" is our circular buffer


                internal void
                Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
                {
                    // Load the Library

                    // Get a DirectSound object!

                    // "create" a primary buffer

                    // "create" a seconadary buffer 

                    // Start it playing
                }



Casey spending most of the time googling and figuring out how to do windows api calls 





45:16
Casey defining the format for sound card buffer

-   WAVE_FORMAT_PCM is the only thing that msdn said they kind of support

-   for channels, we want stereo and sound effects, so we have 2 

-   wBitsPerSample, we can decide wither we want 8 bit audio or 16 bit audio.
we want 16 bit audio. CD is 16 bit, everything modern audio related is 16 bit or above.
might as well get the extra resolution to make it sound nicer.


-   we will be writing our sound buffer with interleaved channel data. 

                LEFT RIGHT LEFT RIGHT LEFT 

    (left channel data, then right channel data, then left channel data ...)

-   nBlockAlign, pretty much means they want us to calculate how many bytes per sampler there are 
    bytes per sample is gonna be the sum of both channels bytes

                [LEFT RIGHT] LEFT RIGHT LEFT 

    so they want us to compute the size of [LEFT RIGHT]




                internal void
                Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
                {
                    ...
                    ...

                    WAVEFORMATEX WaveFormat = {};
                    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
                    WaveFormat.nChannels = 2;
                    WaveFormat.nSamplesPerSec = SamplesPerSecond;
                    WaveFormat.wBitsPerSample = 16;
                    WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
                    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
                    WaveFormat.cbSize = 0;

                    ...
                    ...

                    LPDIRECTSOUNDBUFFER PrimaryBuffer;
                    if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                    {
                        HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);

                        ...
                    }

                    ...
                }





52:03
Casey proceeds to create the secondary buffer 


                internal void
                Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
                {

                    ...
                    ...

                    // TODO(casey): DSBCAPS_GETCURRENTPOSITION2
                    DSBUFFERDESC BufferDescription = {};
                    BufferDescription.dwSize = sizeof(BufferDescription);
                    BufferDescription.dwFlags = 0;
                    BufferDescription.dwBufferBytes = BufferSize;
                    BufferDescription.lpwfxFormat = &WaveFormat;
                    LPDIRECTSOUNDBUFFER SecondaryBuffer;
                    HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0);
                    if(SUCCEEDED(Error))
                    {
                        OutputDebugStringA("Secondary buffer created successfully.\n");
                    }

                }


1:09:23
are there any reasons why we want more than 2 channels?

for this game, no, cuz Casey doesnt have a full time sound guy, and we are a 2D game. 
so we will mostly have stereo sound. but you can imagine if you have a immersive 3D game, then you may want 
output more channels. 


1:11:06
if the frame rate would ever drop, would there be audio drop outs?
the reason why i said we are going to make a big buffer, (2 seconds); is becuz audio drop outs there are 2 ways 
you can approach them. 

one way you can approach them is you can write way ahead of where the game is. but then you get a lot of latency,
or you get a lot of overhead. 

the other way is that you can just write one additional frame ahead. so if you miss one frame you are fine. 

[didnt quite understand this part]

In short, Casey says he finds that having a large buffer gives you a lot of leniency when you have frame drops.


