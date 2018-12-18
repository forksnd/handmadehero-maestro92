Handmade Hero Day 155 - Introduction to Particle Systems

Summary:
implemented a simple particle system

implemented simple fade to existence, full alpha, then fade out of existence rendering for particles 

Keyword:
Graphics Particle System 


6:38
if you think about how game graphics in general, we have a few different buckets that we put things into 
depending on what it is that we are trying to draw. in our 2D game engine, we have already built 2 buckets: ground tiles and sprites. 

they both flow through the same renderer, but we did them completely differently. 

for ground chunks, we break the world into tiles, and we did extra work to ensure they are seamless

for our sprites, we had simple sprites or groups of sprites: torso + head + cape, with alpha channel etc.

so we render these two differently.


9:31
sometimes we have things that dont have a specific shape. the word we use will be "phenomenom" such as liquids, Gasses 
mist.

artists cant really draw these. And with our current rendering system, we cant really tackle it. 

this is where a particle system comes in.


so if we want to render a fire, or smoke, we will use a particle system to try to capture the feel of a fire or smoke to render it 



11:54
typically in a particle system, you have an Emitter. its job is to emit particles
typically the emitter has parmeters such as what shape it is, or where do particles start, etc 

after the particles are spawned, it will have some simple simulations to follow. For example, the particles
will have some random direction in the upwards facing cone. 




17:07
Casey starting to implement code 

first Casey defines a particle struct 

P is for position
dP is for velocity
Color is color 

                struct particle
                {
                    v3 P;
                    v3 dP;
                    v4 Color;
                };


then we store a list of particles 
the "NextParticle" is the the next available index in the Particles array. 

                struct game_state
                {
                    ...
                    ...

                    u32 NextParticle;
                    particle Particles[256];
                };


19:17
then in our main game simulation code 
we start to write the simulation code for the particles

-   Casey mentioned that he has to two choices
    he can either "simulate then render" the particles, or 
    "render the particles then simulate"

    the pros of the render, simulate method is that you will be rendering the particles at their spawn location
    the the donwside is, you get a frame lag relative to user interaction

-   Casey opted the "simulate then render" approach

                handmade.cpp

                #if HANDMADE_INTERNAL
                game_memory *DebugGlobalMemory;
                #endif
                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    
                    // NOTE(casey): Particle system test
                    for(u32 ParticleIndex = 0;
                        ParticleIndex < ArrayCount(GameState->Particles);
                        ++ParticleIndex)
                    {
                        particle *Particle = GameState->Particles + ParticleIndex;

                        // NOTE(casey): Simulate the particle forward in time
                        Particle->P += Input->dtForFrame*Particle->dP;

                        ...
                        ...

                        // NOTE(casey): Render the particle
                        PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Head), 1.0f, Particle->P, Color);
                    }
                }



34:13
Casey adding the code that spawns particles 

what Casey did is that every frame, we will spawn 2 particles. 
notice the usage of GameState->NextParticle. Its like a circular buffer. So the old particles gets at the top
overwritten the newer particles, which will spawn at the starting location again.
    

                handmade.cpp


                for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 2; ++ParticleSpawnIndex)
                {
                    particle *Particle = GameState->Particles + GameState->NextParticle++;
                    if(GameState->NextParticle >= ArrayCount(GameState->Particles))
                    {
                        GameState->NextParticle = 0;
                    }

                    Particle->P = V3(RandomBetween(&GameState->EffectsEntropy, -0.25f, 0.25f), 0, 0);
                    Particle->dP = V3(RandomBetween(&GameState->EffectsEntropy, -0.5f, 0.5f), RandomBetween(&GameState->EffectsEntropy, 0.7f, 1.0f), 0.0f);
                    Particle->Color = V4(RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                         RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                         RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                         1.0f);
                }




38:16
Casey now making the particles fade out over time. 
the way he did that is to edit the color value. 

first Casey added a dColor field in particle 

                    struct particle
                    {
                        v3 P;
                        v3 dP;
                        v4 Color;
                        v4 dColor;
                    };


this way we are able to simulate the Color 
we initialize the dColor to be a negative value  
                
                Particle->dColor = V4(0, 0, 0, -0.5f); 

this way when we simulate it, Color will decrease overtime 

                handmade.cpp

                for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 2; ++ParticleSpawnIndex)
                {
                    particle *Particle = GameState->Particles + GameState->NextParticle++;
                    if(GameState->NextParticle >= ArrayCount(GameState->Particles))
                    {
                        GameState->NextParticle = 0;
                    }

                    Particle->P = V3(0, 0, 0);
                    Particle->dP = V3(0, 1.0f, 0.0f);
                    Particle->Color = V4(1, 1, 1, 2.0f);                    
                    Particle->dColor = V4(0, 0, 0, -0.5f);           <-------------------------------
                }

then during the Particle System simulation code, we adjust Particle->Color


                // NOTE(casey): Particle system test
                for(u32 ParticleIndex = 0;
                    ParticleIndex < ArrayCount(GameState->Particles);
                    ++ParticleIndex)
                {
                    particle *Particle = GameState->Particles + ParticleIndex;

                    // NOTE(casey): Simulate the particle forward in time
                    Particle->P += Input->dtForFrame*Particle->dP;
                    Particle->Color += Input->dtForFrame*Particle->dColor;      <---------------------

                    // NOTE(casey): Render the particle
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Head), 1.0f, Particle->P, Color);
                }




40:07
Casey mentioned that in the Particle Simulation code, we didnt clamp colors,
and indeed we see artifacts. So Casey was suggesting either we clamp the colors in the renderer, or we just clamp the colors 
in the particle system tick code 

so Casey proceeds to add the clamping code 


-   42:58
    notice that we make the distinction of the local variable "Color" and "Particle->Color".
    the "Color" variable is the output color. "Particle->Color" is the simulation color.
    we can set Paticle->Color to start with values beyond 1. but we are clamping the output color that goes to the renderer. 

    so for example, Casey initalize the Particle->Color = v4(1, 1, 1, 2.0f);
    that way particles can stay black longer  

-   43:20
    by tweaking some numbers, Casey actually showed that the particles dont linearly fade out. 

    what happens is that although the simulation color is above one 
    we clamp the output color 

        Particle->Color.a                                 Color.a

        2   |___                                            |
            |   \____                                       |
            |        \____                                  |
            |             \____                             |
        1   |------------------\____                        |----------------------\____ 
            |                       \____                   |                           \_____ 
            |                            \_____             |                                 \_____
            |__________________________________\__          |_______________________________________\_
            t0                  t1

    between time t0 to time t1, the particles are at full brightness, then it fades out 

    hence the non linear fade out 


                // NOTE(casey): Particle system test
                for(u32 ParticleIndex = 0;
                    ParticleIndex < ArrayCount(GameState->Particles);
                    ++ParticleIndex)
                {
                    particle *Particle = GameState->Particles + ParticleIndex;

                    // NOTE(casey): Simulate the particle forward in time
                    Particle->P += Input->dtForFrame*Particle->dP;
                    Particle->Color += Input->dtForFrame*Particle->dColor;

                    // TODO(casey): Shouldn't we just clamp colors in the renderer??
                    v4 Color;
                    Color.r = Clamp01(Particle->Color.r);
                    Color.g = Clamp01(Particle->Color.g);
                    Color.b = Clamp01(Particle->Color.b);
                    Color.a = Clamp01(Particle->Color.a);

                    // NOTE(casey): Render the particle
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Head), 1.0f, Particle->P, Color);
                }


43:41
Now Casey would like to add in some randomness.

                for(u32 ParticleSpawnIndex = 0;
                    ParticleSpawnIndex < 2;
                    ++ParticleSpawnIndex)
                {
                    particle *Particle = GameState->Particles + GameState->NextParticle++;
                    if(GameState->NextParticle >= ArrayCount(GameState->Particles))
                    {
                        GameState->NextParticle = 0;
                    }

                    Particle->P = V3(RandomBetween(&GameState->EffectsEntropy, -0.25f, 0.25f), 0, 0);
                    Particle->dP = V3(RandomBetween(&GameState->EffectsEntropy, -0.5f, 0.5f), RandomBetween(&GameState->EffectsEntropy, 0.7f, 1.0f), 0.0f);
                    Particle->Color = V4(RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                         RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                         RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                         1.0f);
                    Particle->dColor = V4(0, 0, 0, -0.5f);
                }
                




53:33
the other thing you can do is to give particles an acceleration as well, thats if you want to give particles arch or non linear trajectory


53:36
another thing is that Casey commented that right now we are doing 
                Particle->Color += Particle->dColor.

often times is dColor is not expressive enough for the color variation. 
usually what you for a particle system is a very explicit way of doing a color ramp through a series of colors

for example, you want to start at 0 alpha, then quickly ramp to full alpha, then slowly ramp out again.
this we can make the particle fade into existence as oppose to appearing at full brightness at start.


there are ways you can hack that in, for example putting it on a curve. 



55:27
As a hack, Casey put it some code to have the particles fade into existence

-   the particles start off at color v4(x, y, z, 1.0f);


                    for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 2; ++ParticleSpawnIndex)
                    {
                        particle *Particle = GameState->Particles + GameState->NextParticle++;
                     
                        ...
                        ...

                        Particle->Color = V4(RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                             RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                             RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                             1.0f);
                        Particle->dColor = V4(0, 0, 0, -0.5f);
                    }


-   then if Particle->Color is decreasing from 1.0 to 0.9f range, pretty much when the color is near at birth, 

    we map Particle->Color.a to 0.0 ~ 1.0

    so its just a linear mapping 

                Color.a = 0.9f*Clamp01MapToRange(1.0f, Color.a, 0.9f); 

        Particle->Color.a                                  

        1   |___                                                                                   
            |   \____                                                                          
            |        \____                                                                 
            |             \____                                                       
            |                  \____                                             
            |                       \____                                          
            |                            \____                                      
            |                                 \____                  
            |                                      \____
        0.9 |-------------------------------------------\
            |                                        
            |                                                
            |______________________________________        
            t0                                     t1                                                                 


        Color.a

        1   |                                         
            |                                    ____/
            |                               ____/
            |                          ____/
            |                     ____/  
            |                ____/            
            |           ____/                      
            |      ____/                             
            | ____/
            |/_________________________________________
          0                                        

          

    we also apply a constant value, so when it gets to 1.0, we cap it 0.9. Just to make it look smoother
    when it fades out towards the end.



-   full code below:

                    // NOTE(casey): Particle system test
                    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
                    {
                        particle *Particle = GameState->Particles + ParticleIndex;

                        // NOTE(casey): Simulate the particle forward in time
                        Particle->P += Input->dtForFrame*Particle->dP;
                        Particle->Color += Input->dtForFrame*Particle->dColor;

                        // TODO(casey): Shouldn't we just clamp colors in the renderer??
                        v4 Color;
                        Color.r = Clamp01(Particle->Color.r);
                        Color.g = Clamp01(Particle->Color.g);
                        Color.b = Clamp01(Particle->Color.b);
                        Color.a = Clamp01(Particle->Color.a);

                        if(Color.a > 0.9f)
                        {
                            Color.a = 0.9f*Clamp01MapToRange(1.0f, Color.a, 0.9f); 
                        }

                        // NOTE(casey): Render the particle
                        PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Head), 1.0f, Particle->P, Color);
                    }






1:11:03
will these particles be able to respond to impact?

Yes. This is one of the advantages of simulating the particles as entities with physics as oppose to stateless 
functions. It gives you a lot more flexibilities
