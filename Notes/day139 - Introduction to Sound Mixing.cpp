Handmade Hero Day 139 - Introduction to Sound Mixing
Summary:

briefly estimated the memory usage of a piece of .wav audio music.

explains how our brain understands sound frequencies
high frequencies sound, high pitch voice
low frequncies sound, low deep sound.
frequencies = pitch

also mentioned Amplitude = volume

explained how sound mixer is just a sound adder

mentions potential problems in implementing a mixer 
-   clipping
-   interpolation / Modulation

talked about what clipping is and how clipping is usually not a problem
talked about how Modulation works 

mentioned how to change the volume, pan and pitch.

changing volume and pan are very easy

changing pitch is has two approaches 
-   length preserving
-   not length presrving 

talked about the cheap way of doing not length preserving, which is to just play the audio clip faster.

the length presrving method are very complicated

Keyword:
sound, sound mixer, 


0:50
Casey mentioned that we probably wont want to do multi-part loading of .wav files.  
so he wants to approach the audio, to see how audio asset files look. 
so that we can see whether want to put that in the asset system. 

Normally for sound effects, we dont need to do anything fancy cuz they are small, we load them 
and we playback 

but if you have a 3,4 minute piece of music, that something you may not want to keep the entire thing in memory
so maybe you are pulling it in chunks of it at a time, so you only have to load certain amounts of it. 
lets say 10 seconds of, then you bring in the next 10 seconds. 

Casey again said, nowadays, its probably no reason to bother with that kind of streaming. cuz if you do the back of the
envelope calculation. 

is if you have something that is going 48000 samples per second, and you are talking about a stereo piece of music that is 
16 bits. so that is 4 bytes per sample (2 channels, so 2 x 16 bits, giving you 4 bytes);

48000 * 4 = 192000 is prett much how much memory a second of music is worth.

so if my music is 30 minutes long. (dividing it by 1024 converts it to megabytes)

192000 * (3 * 60) / (1024 * 1024) = 32.95

thats 32 MB of memory.

so if we asks ourselves the question can we spare 64 megabytes of memory as a music streaming buffer, the answer is probably yes. 
so i dont know if it really makes sense to think about this problem very deeply. 


4:22
Casey beginning to talk about Sound (+music);

Casey explaining the basic physics of Sound particles and Sound Waves




12:19
the way your brain understand frequencies is that the higher the frequencies, the faster the pitch of the sound is.

So something that is vibrating very slowly, will be a very low deep sound.
and something that is vibratingv very quickly, will have a very high pitch sound.



17:54
his point is simply that the frequences, when we talk about how many things we will output in a second, 
we often talk about 800 samples of sound for every 1 sample of video.


19:05
Amplitude plates to volume

so frequency and Amplitude pretty much gives us the entire characteristics of the sounds that we hear.




30:33
Casey talking about how sound mixing works

just adding two waves together


36:17
a mixer is really just an adder



37:51
Casey talking about some more concepts:
-   Clipping
-   Interpolation / Modulation

assume we have a "16-bit" sound sample, which goes from [-32k, 32k]

    
so the loudest sound of this wave will be 32k and -32k

               32 k
                ..                                         ..            
             .      .                                   .      .
           .          .                               .          .
         .              .                           .              .
       .                 .                         .                 .
      .                   .                       .                   .
    ------------------------------------------------------------------------------->
                            .                   .
                             .                 .
                              .              .
                                .          .
                                  .      .
                                     . .  
                                    -32 k

in real-life, what that means is whatever loudest sound your speaker can produce, turn to that.

so if we have a sound wave in our code, in software, we will map the wave to the settings of our speakers 

so what happens if we mix two sound waves, where two 32 k peaks have to be added together?


the superposition is a sound that goes on top, and it can not be produced by the speaker

thhat is clipping


                64 k
               ....
             .      . 
            .        .    
           .          .
          .            .
       ---------------------                                32 k
         .      ..      .                                   ..            
        .    .      .    .                               .      .
       .   .          .   .                            .          .
      .  .              .  .                         .              .
      ..                 . .                       .                 .
      .                   .                       .                   .
    ------------------------------------------------------------------------------->
                            .                   .
                             .                 .
                              .              .
                                .          .
                                  .      .
                                     . .  
                                    -32 k


clipping is something you can choose to address or not. 
often times you just dont have to. You will be surprised, but a lot of times, adding two sounds together 
often does not produce any objectionable clipping whatsoever

its surprising just how little problem clipping becomes

but if you really want it, you can do things such as "Compressors"
there are things such as 
    "soft knee" compressors 
    "hard knee" compressors 

which are some mapping techniques that always levels out at 32 k.

it does some logrithmic curve stuff

                                            
            ^                         . . . . .  .  .   32 k
            |                  .
            |             .
            |         .
            |      .
            |    . 
            |  .
            | .
        ------------------------------------------------------------------->



44:48
there are things that we might want to do with our sounds, even if we are not fancy audio guys.
-   adjust volume
-   adjust Pan 
-   vary pitch

Volume and Pan basically boil down to the same thing. 

panning is just a volume change.

what is Volume? Volume is just Modulation 
so if I want to make a sound sound more quiet, I just need to multiply by a value between [0 ~ 1]

any value between 0 ~ 1, it will just create a softer version of the original sound. 

The challenge of this is that you cannot change these parameters discontinuously

lets say i want to fade in some music, it starts out quiet and reaches full volume as it goes,


so over time, volume will go from v = 0 to v = 1. that means volume has to be smoothly incremented over time.
this gets complicated if we are doing SIMD, as we will process these 4 samples at a time.




changing pitch is even more complicated
-   Length preserving 
-   not length preserving

let say i have a wave of an audio file. If I want to make it more high pitch,
one way is just to play the audio faster. 

it it was going 48000 samples / sec when it was sampled, I could play it back at 2 * 48000 samples / sec
and that will be a much high pitch sound.

But the problem is, that will also speed the sound up. Meaning the lenght of your clip will only be half as long.

so playing it faster is considered a cheap way, which is the not length preserving way. 


but if you increase the pitch as well as preserving the length.
Length preserving pitch change are very complicated.
People have only figure it out how to do those in recent 10 years. 


typically you will never see length preserving pitch change in a game, becuz the cost beneft is just not there.
the chances that a game needs to do that much pitch bending is extermely low. Typically only a game 
that is specifically doing something with audio, that you will see it being used.

you wouldnt see it in a first person shooter. You pretty much just record all the sounds that you need, and you dont 
pitch bend them much at all. You only ibtch bend them a little bit to give your sound some variety. Those all ones
that can happen with the not length preserving approach. 



53:50
even if we were to do the non-length-preserving pitch change, it does introduce one more interesting aspect, 
if we want to shift it by a non-mulitple of 2, which we certain will.

Casey proceeds to show us an audio sample at 400 hz an 800 hz, which sounds completely different.
so chances are we will change it by 1.02 or 1.1, just tiny pitch changes. that means we are sampling
points which we wont have. Cuz of that, we have to use interpolation 

see graph at 55:36, something I cant draw using text. and you will understand what Casey meant. 


56:40
Casey discusses plans to write a mixer

-   takes input sounds
-   knows what time they are at 
-   can add the sound in, mulitply by a volume 
-   continuoulsy change the volume
-   vary the pitch



58:23
you said panning is a function of volume.

