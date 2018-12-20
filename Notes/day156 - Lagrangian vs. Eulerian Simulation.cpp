Handmade Hero Day 156 - Lagrangian vs. Eulerian Simulation

Summary:

added fancier trajectory to particles

Added acceleration to particles.

attempted to solve the foundtain problem with the particles system and hacks without success

introduces the Lagrangian and Eulerian simulation methods 

taked about Lagrangian is bad at solving simulation problems about density
mentioned that Eulerian is very good at solving density problems 

implemented a simple mixed method to emulate the foundtain effect in the particle system 

Keyword:
Graphics, Particle System, Lagrangian and Eulerian simulation


3:05
Casey intends to add more fancy things to the particle
first he added acceleration to the particle


                handmade.h

                struct particle
                {
                    bitmap_id BitmapID;
                    v3 P;
                    v3 dP;
                    v3 ddP;         <----------- added acceleration
                    v4 Color;
                    v4 dColor;
                };


Casey added gravity to the particles


                for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 3; ++ParticleSpawnIndex)
                {
                    particle *Particle = GameState->Particles + GameState->NextParticle++;
                    if(GameState->NextParticle >= ArrayCount(GameState->Particles))
                    {
                        GameState->NextParticle = 0;
                    }

                    Particle->P = V3(RandomBetween(&GameState->EffectsEntropy, -0.05f, 0.05f), 0, 0);
                    Particle->dP = V3(RandomBetween(&GameState->EffectsEntropy, -0.01f, 0.01f), 7.0f*RandomBetween(&GameState->EffectsEntropy, 0.7f, 1.0f), 0.0f);
    ------------>   Particle->ddP = V3(0.0f, -9.8f, 0.0f);
                    Particle->Color = V4(RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                         RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                         RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                         1.0f);
                    Particle->dColor = V4(0, 0, 0, -0.25f);
                }
                
also noted that we set our Particle->dP to be 7.0f*RandomBetween(&GameState->EffectsEntropy, 0.7f, 1.0f); in the y direction.
that is to coutereact the gravity




and of course in the simulation code, we also update the velocity 


                for(u32 ParticleIndex = 0;
                    ParticleIndex < ArrayCount(GameState->Particles);
                    ++ParticleIndex)
                {
                    particle *Particle = GameState->Particles + ParticleIndex;                        
                    // NOTE(casey): Simulate the particle forward in time
                    Particle->P += (0.5f*Square(Input->dtForFrame)*Input->dtForFrame*ddP + Input->dtForFrame*Particle->dP);
                    Particle->dP += Input->dtForFrame*ddP;    <------------------------
                    Particle->Color += Input->dtForFrame*Particle->dColor;

                    ...
                    ...
                }





12:22
Casey added logic for collision resolution with the ground for the particles 

first Casey just did the negation if it goes negative

                for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
                {
                    particle *Particle = GameState->Particles + ParticleIndex;                        
                    // NOTE(casey): Simulate the particle forward in time
                    Particle->P += (0.5f*Square(Input->dtForFrame)*Input->dtForFrame*ddP + Input->dtForFrame*Particle->dP);
                    Particle->dP += Input->dtForFrame*ddP;    <------------------------
                    Particle->Color += Input->dtForFrame*Particle->dColor;

                    if(Particle->P.y < 0.0f)
                    {
                        Particle->P.y = -Particle->P.y;
                        Particle->dP.y = -Particle->dP.y;
                    }

                    ...
                    ...
                }


then Casey introduced the concept energy loss and the Coefficient of Friction and restitution


                for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
                {
                    particle *Particle = GameState->Particles + ParticleIndex;                        
                    // NOTE(casey): Simulate the particle forward in time
                    Particle->P += (0.5f*Square(Input->dtForFrame)*Input->dtForFrame*ddP + Input->dtForFrame*Particle->dP);
                    Particle->dP += Input->dtForFrame*ddP;    <------------------------
                    Particle->Color += Input->dtForFrame*Particle->dColor;

                    if(Particle->P.y < 0.0f)
                    {
                        r32 CoefficientOfRestitution = 0.3f;
                        r32 CoefficientOfFriction = 0.7f;
                        Particle->P.y = -Particle->P.y;
                        Particle->dP.y = -CoefficientOfRestitution*Particle->dP.y;
                        Particle->dP.x = CoefficientOfFriction*Particle->dP.x;
                    }

                    ...
                    ...
                }




15:32
Casey describing how to simulate a fountain                        
Casey explains why does fountain have that fountain look 


20 ~ 28:30
Casey trying to hack to make the particle system look like a fountain through hacks,
but without success....


29:12
Casey start talking about the proper way to solve these fluid simulation problems 

However, the proper solution are sometimes expensive, which you cant afford it 
and youll be back to doing hacks. 

the problem that we are seeing these problems is that, while our particle system is a bunch 
of particle being simulated by newtonian motions, our particles are considered as separate individual particles.  

so for our fountain, at the apex, we have a high density of particles.
the proper way is that for every particle, we see how many neighboring particles does it have.


for example, if A has a lot of neighbor particles, we will consider a repelling force away from the crowd 
but that will be incredibly expensive.


                     A       .                    
                 .      .                         
                    .     .    .                                   
                   .  .     .                      
                        ^
                        |
                        |
                        |
                        |
                        |                        
                        |                      
                        |
                        |
                        |
                        |

it turns out in real physics simulatiosn on computers (Not the fake ones in games, but the real ones for 
Engineering, Tesla for Cars, NASA for spaceships. The legit kinds);, there are two ways to simulate physics,
two schools of thoughts.

1.  Eulerian Methods
2.  Lagrangian Methods

what we are doing right now is the Lagrangian methods.
What is distinctive about the Lagrangian methods is that when you have the information about what you are simulating,
is actually the thing that moves through the configuration space. 

so our particles, which as a 3d position in our x y z space, and we are also simulating the particles themselves 

                y
                ^
                |
                |
                |
                |
                |
                |
                |
                --------------------------> x 
               /
              /
             /
            /
           /
          v
          z




the Eulerian Methods think of things as stationary
we can think of it as a discrete grid, where our fountain starts 
at the bottom and it is producing particles 

but instead of considering them as particles, we consider them as a density in our space.
Theres just a density and a velocity a certain location.            

             _______________________________________
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |   ^   |       |       |
            |       |       |   |   |       |       |
            |_______|_______|_______|_______|_______|


if I were to spawn 5 particles, and the average upwards velocity is 2m/s
our bottom center cell will just store 
    5 particles 
    2 m/s 


so instead of storing particles at all, we will just have the storage of the grid.
the storage will be fixed, the size of our grid 

so instead of simulating particles, we will just be simulating grid cell forward in time.
the grid cell wont move during the simulation, but instead, will broadcast their quantities throughout the simulation.

essentially there will equations we will solve that will produce the cell values in the next time step.


so back to our water fountain problem, at the apex, where we have a crowd of particles, that will be indicated 
by our density, and lets say our we have our formula gives a sideways force depending on density.
That way our main cell will give neighboring cells density 

tick 1 
             _______________________________________
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       \   ^   /       |       |
            |_______|_______|\__|__/|_______|_______|
            |       |       | \ | / |       |       |
            |       |   <---|---10--|--->   |       |
            |_______|_______|_______|_______|_______|

tick 2

             _______________________________________
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |       |       |       |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |   1   |   3   |   1   |       |
            |_______|_______|_______|_______|_______|
            |       |       |       |       |       |
            |       |   3   |   7   |   3   |       |
            |_______|_______|_______|_______|_______|

There are many ways of solving this 



36:03
you can see how that the Lagrangian method falls down at certain problems that are about density. 
Lagrangian methods are very bad at solving density related problems, becuz density is about how much stuff exists 
per unit volume, and the Lagrangian doesnt even have anotion fo that.

Volume has to be imposed ontop of the Lagrangian methods, so when you are going to do a Lagrangian method that has to 
take account volumetric forces, you will constantly run into trouble. Doesnt mean its impossible


36:40
Eulerian methods has the opposite problem.
Things that has to do with volume are very easy. 
but fixed volume




37:45
if you can somehow combine the two methods, you get mixed methods 

so what we can do is we simulate our particles using the Lagrangian methods, but 
we use the Eulerian methods to simulate the volumetric forces.


so for example, assume we have our Lagrangian simulation,
before we tick our Lagrangian simulation, we produce a eulerian map from our Lagrangian simulation


                     A       .                                   _______________________________________
                 .      .                                       |       |       |       |       |       |
                    .     .    .                                |       |       |   10  |       |       |     
                   .  .     .                                   |_______|_______|_______|_______|_______|
                        ^                                       |       |       |       |       |       |
                        |                                       |       |       |   5   |       |       |
                        |.                ------------>         |_______|_______|_______|_______|_______|
                      . |                                       |       |       |       |       |       |
                        |                                       |       |       |   2   |       |       |
                        | .                                     |_______|_______|_______|_______|_______|
                       .|                                       |       |       |       |       |       |
                        | .                                     |       |       |   2   |       |       |
                     .  |                                       |_______|_______|_______|_______|_______|
                        |                                       |       |       |       |       |       |
                        |                                       |       |       |       |       |       |
                        |                                       |_______|_______|_______|_______|_______|



then when I tick the Lagrangian simulation, we do a look up in the eulerian map 
if it is in a high density area, I will introduce a sideways force. 

so its like you adding a eulerian term in your Lagrangian update.

so you can see how you can construct a more physically fluid/gas simulation without having to do n^2 loops. 
Cuz thats the real problem in the Lagrangian simulation. 
when you are only considering one particle, but you have to also think about other particles, that leads to n^2 loops.

thats real bad for a particle system cuz our n is gonna be high. 




43:08
Casey attempting to implement this mixed methods

so Casey defining the stuff for the eulerian part

he defines the Cell and the Grid

                handmade.h

                struct particle_cel
                {
                    real32 Density;
                    v3 VelocityTimesDensity;
                };

and we define a 32 x 32 grid 

                struct game_state
                {
                    ...
                    ...

                #define PARTICLE_CEL_DIM 32
                    u32 NextParticle;
                    particle Particles[256];
                    
                    particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];      <--------------- 
                };



45:15
then we have to put the Lagrangian particles in the eulerian map .
what we are doing is we iterate through the Lagrangian particles, we take their locations 
and then we put them in some eulerian cells

if you think about it, we dont pay n^2, but you do pay a 2nd pass.

-   the figure out which eulerian cell a Lagrangian particle falls into
    we just do some linear mapping. And if it out of bounds, we clamp it at the boundaries.
    {

                r32 GridScale = 0.25f;
                r32 InvGridScale = 1.0f / GridScale;
                v3 GridOrigin = {-0.5f*GridScale*PARTICLE_CEL_DIM, 0.0f, 0.0f};

                ...
                ...

                v3 P = InvGridScale*(Particle->P - GridOrigin);
                
                s32 X = TruncateReal32ToInt32(P.x);
                s32 Y = TruncateReal32ToInt32(P.y);

                if(X < 0) {X = 0;}
                if(X > (PARTICLE_CEL_DIM - 1)) {X = (PARTICLE_CEL_DIM - 1);}
                if(Y < 0) {Y = 0;}
                if(Y > (PARTICLE_CEL_DIM - 1)) {Y = (PARTICLE_CEL_DIM - 1);}
    }

-   once we have the location, we add to the density of that cell 

                real32 Density = Particle->Color.a;
                Cel->Density += Density;

-   we also contribute to the VelocityTimesDensity
                
                Cel->VelocityTimesDensity += Density*Particle->dP;

-   notice that we are treating each particle to have the same density here

                Cel->Density += 1.0f;

    we will change that later


-   full code below 

                r32 GridScale = 0.25f;
                r32 InvGridScale = 1.0f / GridScale;
                v3 GridOrigin = {-0.5f*GridScale*PARTICLE_CEL_DIM, 0.0f, 0.0f};
                for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
                {
                    particle *Particle = GameState->Particles + ParticleIndex;

                    v3 P = InvGridScale*(Particle->P - GridOrigin);
                    
                    s32 X = TruncateReal32ToInt32(P.x);
                    s32 Y = TruncateReal32ToInt32(P.y);

                    if(X < 0) {X = 0;}
                    if(X > (PARTICLE_CEL_DIM - 1)) {X = (PARTICLE_CEL_DIM - 1);}
                    if(Y < 0) {Y = 0;}
                    if(Y > (PARTICLE_CEL_DIM - 1)) {Y = (PARTICLE_CEL_DIM - 1);}
                    
                    particle_cel *Cel = &GameState->ParticleCels[Y][X];
                    real32 Density = Particle->Color.a;
                    Cel->Density += 1.0f;
                    Cel->VelocityTimesDensity += Density*Particle->dP;
                }




51:25
Casey added some debugging code to render the grid cells
-   you can see that we are using the alpha value to indicate the density

                for(u32 Y = 0; Y < PARTICLE_CEL_DIM; ++Y)
                {
                    for(u32 X = 0; X < PARTICLE_CEL_DIM; ++X)
                    {
                        particle_cel *Cel = &GameState->ParticleCels[Y][X];
                        real32 Alpha = Clamp01(0.1f*Cel->Density);
                        PushRect(RenderGroup, GridScale*V3((r32)X, (r32)Y, 0) + GridOrigin, GridScale*V2(1.0f, 1.0f),
                                 V4(Alpha, Alpha, Alpha, 1.0f));
                    }
                }


58:53
Casey showing the rendered density map
here, we are seeing a thick bottom  (rendered white grid cells at the bottom); 
this is becuz all the particles spawns near that location, and also falls to the bottom
around that location.
                                                                 _______________________________________
                                                                |       |       |       |       |       |
                         .                                      |       |       |   1   |       |       |     
                     .     .                                    |_______|_______|_______|_______|_______|
                        ^                                       |       |       |       |       |       |
                        |                                       |       |       |   5   |       |       |
                        |.                ------------>         |_______|_______|_______|_______|_______|
                      . |                                       |       |       |       |       |       |
                        |                                       |       |       |   2   |       |       |
                        | .                                     |_______|_______|_______|_______|_______|
                       .|                                       |       |       |       |       |       |
                        | .                                     |       |       |   2   |       |       |
                     .  |                                       |_______|_______|_______|_______|_______|
                        |                                       |       |       |       |       |       |
              .   . . . |  .    .                               |       |       |  10   |       |       |
               . .   .  |  . . .                                |_______|_______|_______|_______|_______|




1:00:19
now in your Lagrangian particle simulation code, we now go on to grab the eulerian grid cell to do whatever we want to do.

so previously when Casey was doing hacks to represents the fountain affect
the way he was doing was adding a hacky if statement

essentially he wanted some sideways force when particles reach the very top.
to creating that puffing factor

            <----    .       .  ---->                  
                 .      .                         
            <----     .     .    . ---->                                  
                   .  .     .                      
                        ^
                        |
                        |
                        |
                        |
                        |                        
                        |                      
                        |
                        |
                        |
                        |

-   so during when Casey was trying the hack methods, he tried with weird if conditions
    with out much success


                if(Particle->dP.y < 0.3 && Particle->dP.y > -0.2   <------------ The Hack
                    && Particle->P.y > 3)
                {
                    Particle->dP += 100.0 * Input->dtForFrame*V3(Particle->P.x, 0, 0); 
                }

    the line 
                Particle->dP += 100.0 * Input->dtForFrame*V3(Particle->P.x, 0, 0);

    is just simply, however you are away from the center, we magnify it and add it to the velocity.


-   full code below 

                for(u32 ParticleIndex = 0;
                    ParticleIndex < ArrayCount(GameState->Particles);
                    ++ParticleIndex)
                {
                    // NOTE(casey): Simulate the particle forward in time
                    Particle->P += (0.5f*Square(Input->dtForFrame)*Input->dtForFrame*ddP +
                                    Input->dtForFrame*Particle->dP);
                    Particle->dP += Input->dtForFrame*ddP;
                    Particle->Color += Input->dtForFrame*Particle->dColor;

                    if(Particle->P.y < 0.0f)
                    {
                        r32 CoefficientOfRestitution = 0.3f;
                        r32 CoefficientOfFriction = 0.7f;
                        Particle->P.y = -Particle->P.y;
                        Particle->dP.y = -CoefficientOfRestitution*Particle->dP.y;
                        Particle->dP.x = CoefficientOfFriction*Particle->dP.x;
                    }
                    

                    if(Particle->dP.y < 0.3 && Particle->dP.y > -0.2   <------------ The Hack
                        && Particle->P.y > 3)
                    {
                        Particle->dP += 100.0 * Input->dtForFrame*V3(Particle->P.x, 0, 0); 
                    }

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
                    PushBitmap(RenderGroup, Particle->BitmapID, 1.0f, Particle->P, Color);
                }


Now with our eulerian map in place, we do the proper solution 
which we just replace the entire condition with the density as a constant term

                Particle->dP += 1.0 * Cel->Density * Input->dtForFrame*V3(Particle->P.x, 0, 0); 

and at this point of the video, this simple change is already doing a bit of what we want.

notice with this logic, the particles are accumulating x direction throughout its lifetime.
as times goes on, the term "V3(Particle->P.x, 0, 0);" keeps on increasing,
so Particle->dP will have a larger and larger x direction as its lifetime increases.

so pretty much this is not the proper eulerian term to add to the particles.
anywhere there is density, it will accelerate away from 0. So this is still a hack....

-   full code below:


                for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
                {
                    particle *Particle = GameState->Particles + ParticleIndex;                        

                    v3 P = InvGridScale*(Particle->P - GridOrigin);
                    
                    s32 X = TruncateReal32ToInt32(P.x);
                    s32 Y = TruncateReal32ToInt32(P.y);

                    if(X < 1) {X = 1;}
                    if(X > (PARTICLE_CEL_DIM - 2)) {X = (PARTICLE_CEL_DIM - 2);}
                    if(Y < 1) {Y = 1;}
                    if(Y > (PARTICLE_CEL_DIM - 2)) {Y = (PARTICLE_CEL_DIM - 2);}

                    particle_cel *Cel = &GameState->ParticleCels[Y][X];


                    // NOTE(casey): Simulate the particle forward in time
                    Particle->P += (0.5f*Square(Input->dtForFrame)*Input->dtForFrame*ddP +
                                    Input->dtForFrame*Particle->dP);
                    Particle->dP += Input->dtForFrame*ddP;
                    Particle->Color += Input->dtForFrame*Particle->dColor;

                    if(Particle->P.y < 0.0f)
                    {
                        r32 CoefficientOfRestitution = 0.3f;
                        r32 CoefficientOfFriction = 0.7f;
                        Particle->P.y = -Particle->P.y;
                        Particle->dP.y = -CoefficientOfRestitution*Particle->dP.y;
                        Particle->dP.x = CoefficientOfFriction*Particle->dP.x;
                    }
                    

                    Particle->dP += 1.0 * Cel->Density * Input->dtForFrame*V3(Particle->P.x, 0, 0); 

                    ...
                    ...
                }



1:01:17
Casey made a change about density contributions from particles in the eulerian map 


compare to section 45:15 where we did 
                real32 Density = 1.0f;
now we do 
                real32 Density = Particle->Color.a;

so that particles that are just fading in and the ones that are fading out dont contribute as much.


                r32 GridScale = 0.25f;
                r32 InvGridScale = 1.0f / GridScale;
                v3 GridOrigin = {-0.5f*GridScale*PARTICLE_CEL_DIM, 0.0f, 0.0f};
                for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
                {
                    particle *Particle = GameState->Particles + ParticleIndex;

                    v3 P = InvGridScale*(Particle->P - GridOrigin);
                    
                    s32 X = TruncateReal32ToInt32(P.x);
                    s32 Y = TruncateReal32ToInt32(P.y);

                    if(X < 0) {X = 0;}
                    if(X > (PARTICLE_CEL_DIM - 1)) {X = (PARTICLE_CEL_DIM - 1);}
                    if(Y < 0) {Y = 0;}
                    if(Y > (PARTICLE_CEL_DIM - 1)) {Y = (PARTICLE_CEL_DIM - 1);}
                    
                    particle_cel *Cel = &GameState->ParticleCels[Y][X];
                    real32 Density = Particle->Color.a;        <-------------- chainging the density contributions
                    Cel->Density += 1.0f;
                    Cel->VelocityTimesDensity += Density*Particle->dP;
                }





1:02:02
Casey once again addressing that 
    
                Particle->dP += 1.0 * Cel->Density * Input->dtForFrame*V3(Particle->P.x, 0, 0); 

is the wrong eulerian term to add to the Lagrangian particles simulation


1:03:32
for the proper eulerian simulation:

-   we need to examine the particle cel velocities. If things are already going to move out of the cell, you would 
    not feel the need to disperse them if they were already moving into a space that has room for them. 

-   for density, you would only move particles from high density to lower density. Right now we are not looking at that at all.



Casey going on to implement a very simple eulerian simulation, where cells will be looking at neighbors 

-   first since we are looking up neighbors, we have to limite to cells that are in the following range:
    
                X = [1, PARTICLE_CEL_DIM - 2] and y = [1, PARTICLE_CEL_DIM - 2]

    which we do here                 

                if(X < 1) {X = 1;}
                if(X > (PARTICLE_CEL_DIM - 2)) {X = (PARTICLE_CEL_DIM - 2);}
                if(Y < 1) {Y = 1;}
                if(Y > (PARTICLE_CEL_DIM - 2)) {Y = (PARTICLE_CEL_DIM - 2);}

-   then we calculate the dispersion force.
    the despersioni force is the sum of all force from the density gradient among its neighboring cells.
    recall cells only flow from high density places to low density places.

                v3 Dispersion = {};
                real32 Dc = 1.0f;
                Dispersion += Dc*(CelCenter->Density - CelLeft->Density)*V3(-1.0f, 0.0f, 0.0f);
                Dispersion += Dc*(CelCenter->Density - CelRight->Density)*V3(1.0f, 0.0f, 0.0f);
                Dispersion += Dc*(CelCenter->Density - CelDown->Density)*V3(0.0f, -1.0f, 0.0f);
                Dispersion += Dc*(CelCenter->Density - CelUp->Density)*V3(0.0f, 1.0f, 0.0f);

    so we add the sum and give it to the acceleration of the particle 

                v3 Dispersion = {};
                real32 Dc = 1.0f;
                Dispersion += Dc*(CelCenter->Density - CelLeft->Density)*V3(-1.0f, 0.0f, 0.0f);
                Dispersion += Dc*(CelCenter->Density - CelRight->Density)*V3(1.0f, 0.0f, 0.0f);
                Dispersion += Dc*(CelCenter->Density - CelDown->Density)*V3(0.0f, -1.0f, 0.0f);
                Dispersion += Dc*(CelCenter->Density - CelUp->Density)*V3(0.0f, 1.0f, 0.0f);

                v3 ddP = Particle->ddP + Dispersion;


-   full code below:

                for(u32 ParticleIndex = 0;
                    ParticleIndex < ArrayCount(GameState->Particles);
                    ++ParticleIndex)
                {
                    particle *Particle = GameState->Particles + ParticleIndex;                        

                    v3 P = InvGridScale*(Particle->P - GridOrigin);
                    
                    s32 X = TruncateReal32ToInt32(P.x);
                    s32 Y = TruncateReal32ToInt32(P.y);

                    if(X < 1) {X = 1;}
                    if(X > (PARTICLE_CEL_DIM - 2)) {X = (PARTICLE_CEL_DIM - 2);}
                    if(Y < 1) {Y = 1;}
                    if(Y > (PARTICLE_CEL_DIM - 2)) {Y = (PARTICLE_CEL_DIM - 2);}

                    particle_cel *CelCenter = &GameState->ParticleCels[Y][X];
                    particle_cel *CelLeft = &GameState->ParticleCels[Y][X - 1];
                    particle_cel *CelRight = &GameState->ParticleCels[Y][X + 1];
                    particle_cel *CelDown = &GameState->ParticleCels[Y - 1][X];
                    particle_cel *CelUp = &GameState->ParticleCels[Y + 1][X];

                    v3 Dispersion = {};
                    real32 Dc = 1.0f;
                    Dispersion += Dc*(CelCenter->Density - CelLeft->Density)*V3(-1.0f, 0.0f, 0.0f);
                    Dispersion += Dc*(CelCenter->Density - CelRight->Density)*V3(1.0f, 0.0f, 0.0f);
                    Dispersion += Dc*(CelCenter->Density - CelDown->Density)*V3(0.0f, -1.0f, 0.0f);
                    Dispersion += Dc*(CelCenter->Density - CelUp->Density)*V3(0.0f, 1.0f, 0.0f);

                    v3 ddP = Particle->ddP + Dispersion;

                    // NOTE(casey): Simulate the particle forward in time
                    Particle->P += (0.5f*Square(Input->dtForFrame)*Input->dtForFrame*ddP +
                                    Input->dtForFrame*Particle->dP);
                    Particle->dP += Input->dtForFrame*ddP;
                    Particle->Color += Input->dtForFrame*Particle->dColor;

                    if(Particle->P.y < 0.0f)
                    {
                        r32 CoefficientOfRestitution = 0.3f;
                        r32 CoefficientOfFriction = 0.7f;
                        Particle->P.y = -Particle->P.y;
                        Particle->dP.y = -CoefficientOfRestitution*Particle->dP.y;
                        Particle->dP.x = CoefficientOfFriction*Particle->dP.x;
                    }
                    
                    ...
                    ...
                }



1:11:52
Casey mentioning the problem with his approach. 
the force from the dispersion is related to the eulerian cell size 

for example:
if you have eulerian cells of different size, you cant apply the same amount of force 

in the first case, it takes more force to move from left cell to the right cell compare to the 2nd case.
                 _______________________________
                |               |               |
                |               |               |
                |               |               |
                |     ----------|-------->      |
                |               |               |
                |               |               |
                |_______________|_______________|


                 _______________
                |       |       |        
                |   ----|--->   |        
                |       |       |
                |_______|_______|        

so in you dispersion force equation, it needs to account for the size of the cell.


 
1:15:18
Casey mentioned that you can also add variety to the particles too.
For example, we have the particle remember its bitmap


                struct particle
                {
                    bitmap_id BitmapID;  <--------
                    v3 P;
                    v3 dP;
                    v3 ddP;
                    v4 Color;
                    v4 dColor;
                };

then when we spawn the particle, we assign it to the Particle->BitmapID

                
                    for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 3; ++ParticleSpawnIndex)
                    {
                        particle *Particle = GameState->Particles + GameState->NextParticle++;
                        if(GameState->NextParticle >= ArrayCount(GameState->Particles))
                        {
                            GameState->NextParticle = 0;
                        }

                        .....................................................
                        ..... Setting Particle Position and Velocity ........
                        .....................................................

                        Particle->BitmapID = GetRandomBitmapFrom(TranState->Assets, Asset_Head, &GameState->EffectsEntropy);
                    }



1:22:58
someone in the Q/A asked if it make sense to split the simulation tick in two frames 
use the first frame to accumulate velocity and momentum changes  
use the 2nd frame to resolve them.

Yes, you can do that by having two sets of cells. Once set of cells from the previous frame, one from the new frame. 
This is not always a good idea becuz now you have more memory and your cache performance may be worst 




1:24:29
someone mentioned that wouldnt be inaccurate if a particle overlaps with 4 eulerian cell,
but we only count him in one?

Yes, there are tricks you can do to mitigate it. For example, when we build our eulerian map, 
assign it to all the cells it overlaps based on its proximity to each of them

for example, you can do the bilinear filtering stuff, compare distance from particle center 
to all the centers of all 4 cells, and split the density and velocity contributions that way.

                 __________________
                |        |         |        
                |   a  ##|###  b   |        
                |      ##|###      |
                |      ##|O##      |
                |______##|###______|        
                |      ##|###      |        
                |   d    |     c   |        
                |        |         |
                |________|_________|        
