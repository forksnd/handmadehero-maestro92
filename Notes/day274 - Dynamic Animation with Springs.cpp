Handmade Hero Day 274 - Dynamic Animation with Springs

Summary:
implemented a spring system for the character torso and characte cape.
mentioned the concept of overdamped, underdamped and critically damped oscillations

Keyword:
movement system, animation


4:51
When debugging motion or physics, Case demonstrated that you can decrease the dt that you pass into the system 
(not decreasing the number of ticks per second, but the dt value the system uses to tick);

to get a better view of what is happening.


4:58
Casey showed that our current jumping animation doesnt look goof we someone is doing a series jump (jumpting two grid cells in a row);
currently there is an extra bounce when the guy lands. 

the reason why this is happening is because our animation system is polished for a single jump. 

so this is actually a very common situation, where an artist gives you this animation of just a single jump. 
so what we are going to do is to how we can take a jump (a part of a jump); that was previously candid under an animation control, 
and move that just one part to physical control or pseudophysical control, so we can simulate it a little bit, to achieve a more continuous, 
natural feel.


7:58
so recall, our current movement system is that we have three phases, the launch, the jumping, the landing phase.
and in the launching phase, where the character_s bod bends, we are simulating with a sine wave 


                case MovementMode_Hopping:
                {
                    r32 tJump = 0.2f;
                    r32 tMid = 0.5f;
                    r32 tLand = 0.8f;
                    if(Entity->tMovement < tMid)
                    {
                        r32 t = Clamp01MapToRange(0.0f, Entity->tMovement, tMid);
                        Entity->tBob = -0.1f*Sin(t*Tau32);
                    }
                    
                    if(Entity->tMovement < tLand)
                    {
                        r32 t = Clamp01MapToRange(tJump, Entity->tMovement, tLand);
                        v3 a = V3(0, -2.0f, 0.0f);
                        v3 b = (Entity->MovementTo - Entity->MovementFrom) - a;
                        Entity->P = a*t*t + b*t + Entity->MovementFrom;
                    }
                    else
                    {
                        r32 t = Clamp01MapToRange(tLand, Entity->tMovement, 1.0f);
                        Entity->tBob = -0.1f*Sin(t*Pi32);
                        Entity->P = Entity->MovementTo;
                    }

                    ...
                    ...
                    
                } break;


So Casey say we are going to take the body springingness, we are gonna pull that out and we are gonna simulate that all the time.


                if(Entity->tMovement < tMid)
                {
                    r32 t = Clamp01MapToRange(0.0f, Entity->tMovement, tMid);
                    Entity->tBob = -0.1f*Sin(t*Tau32);
                }

the first thing we are gonna do is to decouple the usage of Entity->tBob from any particular mode.
so we are gonna move Entity->tBob out and always simulate that 

so if we want to override it in any mode, we can certainly do that, but at a base line, we would want to simulate that. 

    ------->    Entity->tBob = -0.1f*Sin(t*Tau32);

                switch(Entity->MovementMode)
                {
                    ...
                    ...

                    case MovementMode_Hopping:
                    {
                        r32 tJump = 0.2f;
                        r32 tMid = 0.5f;
                        r32 tLand = 0.8f;
                        if(Entity->tMovement < tMid)
                        {
                            r32 t = Clamp01MapToRange(0.0f, Entity->tMovement, tMid);

                        }
                        
                        if(Entity->tMovement < tLand)
                        {
                            r32 t = Clamp01MapToRange(tJump, Entity->tMovement, tLand);
                            v3 a = V3(0, -2.0f, 0.0f);
                            v3 b = (Entity->MovementTo - Entity->MovementFrom) - a;
                            Entity->P = a*t*t + b*t + Entity->MovementFrom;
                        }
                        else
                        {
                            r32 t = Clamp01MapToRange(tLand, Entity->tMovement, 1.0f);
                            Entity->tBob = -0.1f*Sin(t*Pi32);
                            Entity->P = Entity->MovementTo;
                        }

                        Entity->dP = V3(0, 0, 0);
                        
                        if(Entity->tMovement >= 1.0f)
                        {
                            Entity->MovementMode = MovementMode_Planted;
                        }
                        
                        Entity->tMovement += 5.0f*dt;
                        if(Entity->tMovement > 1.0f) 
                        {
                            Entity->tMovement = 1.0f;
                        }
                        
                    } break;
                }



9:18
so what we want to do with Entity->tBob is to better simulate it. 
if you noice in our current code 

                case MovementMode_Hopping:
                {
                    r32 tJump = 0.2f;
                    r32 tMid = 0.5f;
                    r32 tLand = 0.8f;
                    if(Entity->tMovement < tMid)
                    {
                        r32 t = Clamp01MapToRange(0.0f, Entity->tMovement, tMid);
    ---------------->   Entity->tBob = -0.1f*Sin(t*Tau32);
                    }
                    
                    if(Entity->tMovement < tLand)
                    {
                        r32 t = Clamp01MapToRange(tJump, Entity->tMovement, tLand);
                        v3 a = V3(0, -2.0f, 0.0f);
                        v3 b = (Entity->MovementTo - Entity->MovementFrom) - a;
                        Entity->P = a*t*t + b*t + Entity->MovementFrom;
                    }
                    else
                    {
                        r32 t = Clamp01MapToRange(tLand, Entity->tMovement, 1.0f);
    ---------------->   Entity->tBob = -0.1f*Sin(t*Pi32);
                        Entity->P = Entity->MovementTo;
                    }

                    ...
                    ...
                    
                } break;

depending on the time, we are hard setting Entity->tBob, which is not very interesting and not very natural.

now we want experiment with some more interesting animations. 


so if you look at our HeroCape

                 _______
                /       \
               /         \
              /           \
             /_____________\
                  #   # 
                  #   #

and imagine we are controlling the body bouncing up or down at the center point 

                 _______
                /   ^   \
               /    |    \
              /     o     \
             /______|______\
                  # v # 
                  #   #


this is currently represented by Entity->tBob. 

                handmade_sim_region.h

                struct sim_entity
                {
                    ...
                    ...

    ----------->    r32 tBob;
                    ...
                };

Entity->tBob is literally the offset from the hero center position.
the thing we want to do is to start tracking the cape_s velocity, and maybe perhaps acceleration

if we keep track of cape_s velocity and acceleration, then we can simualte it with more interesting formulas,
such as newtons law. 


also in previous places where we are hard setting Entity->tBob, we want to replace that with modifying 
Entity->tBob velocity.

                    if(Entity->tMovement < tMid)
                    {
                        r32 t = Clamp01MapToRange(0.0f, Entity->tMovement, tMid);
    ---------------->   Entity->tBob = -0.1f*Sin(t*Tau32);
                    }
                    
                    if(Entity->tMovement < tLand)
                    {
                        r32 t = Clamp01MapToRange(tJump, Entity->tMovement, tLand);
                        v3 a = V3(0, -2.0f, 0.0f);
                        v3 b = (Entity->MovementTo - Entity->MovementFrom) - a;
                        Entity->P = a*t*t + b*t + Entity->MovementFrom;
                    }
                    else
                    {
                        r32 t = Clamp01MapToRange(tLand, Entity->tMovement, 1.0f);
    ---------------->   Entity->tBob = -0.1f*Sin(t*Pi32);
                        Entity->P = Entity->MovementTo;
                    }




this is a pretty standard technique. 
its just taking a previously prescribed position and turning it into a prescribed velocity (or prescribed acceleration);
depending on the circumstance, so that it can be mixed overtime with other things in a natural physical, continuous and natural way. 



13:09
so first thing first, we add the velocity field to sim_entity, which we add dtBob

                handmade_sim_region.h

                struct sim_entity
                {
                    ...
                    ...

                    r32 tBob;
    ----------->    r32 dtBob;

                    ...
                    ...
                };



22:53
so Casey initally put in a very simple motion to describe the motion of the character 

                case MovementMode_Hopping:
                {
                    Launching 

                    jumping 

                    landing logic
                } break;


                ddtBob += -100.0f * Entity->tBob
                Entity->tBob += ddtBob*dt*dt + Entity->dtBob*dt;
                Entity->dtBob += ddtBob*dt;



[recall that this is just hooks law, to emulate the motion of a spring 

recall in hooks law is F = kx. So whenver the Cape is too far away from the torso, 
we pull it in towards it. 

recall from day 273, Entity->tBob is just the offset from EntityTransform.
so the spring Force that is dragging the Cape towards the torso is just F = kx.

and we also know that F = ma. so we know that this force is directly acting on the entity_s acceleration 
hence the formula:

                ddtBob += -100.0f * Entity->tBob

and as a full step of simulation, we have 

                ddtBob += Cp*(0.0f - Entity->tBob) + Cv*(0.0f - Entity->dtBob);
                Entity->tBob += ddtBob*dt*dt + Entity->dtBob*dt;
                Entity->dtBob += ddtBob*dt;]


then also in the launching anding phase, we just edit the acceleration and velocity instead of just hard setting 
Entity->tBob

-   full code below:

                switch(Entity->MovementMode)
                {
                    ...
                    ...

                    case MovementMode_Hopping:
                    {
                        r32 tJump = 0.1f;
                        r32 tThrust = 0.2f;
                        r32 tLand = 0.9f;
                        
                        if(Entity->tMovement < tThrust)
                        {
    --------------->        ddtBob = 30.0f;
                        }    
                        
                        if(Entity->tMovement < tLand)
                        {
                            r32 t = Clamp01MapToRange(tJump, Entity->tMovement, tLand);
                            v3 a = V3(0, -2.0f, 0.0f);
                            v3 b = (Entity->MovementTo - Entity->MovementFrom) - a;
                            Entity->P = a*t*t + b*t + Entity->MovementFrom;
                        }
                        
                        if(Entity->tMovement >= 1.0f)
                        {
                            Entity->P = Entity->MovementTo;
                            Entity->MovementMode = MovementMode_Planted;
    --------------->        Entity->dtBob = -2.0f;
                        }
                        
                        Entity->tMovement += 4.0f*dt;
                        if(Entity->tMovement > 1.0f) 
                        {
                            Entity->tMovement = 1.0f;
                        }
                    } break;
                }
                
                r32 Cp = 100.0f;
                r32 Cv = 10.0f;
                ddtBob += Cp*(0.0f - Entity->tBob) + Cv*(0.0f - Entity->dtBob);
                Entity->tBob += ddtBob*dt*dt + Entity->dtBob*dt;
                Entity->dtBob += ddtBob*dt;






25:21
so now what we want to do is to initiate the crouching motion when the head distance exceeds a threshold

the head distance is between the torso and the head (recall that the player is controlling the head 
to guide the motion of the body);


                r32 HeadDistance = Length(Head->P - Entity->P);
                r32 MaxHeadDistance = 0.5f;
                r32 tHeadDistance = Clamp01MapToRange(0.0f, HeadDistance, MaxHeadDistance);

                switch(Entity->MovementMode)
                {
                    case MovementMode_Planted:
                    {
                        if(BodyDistance > Square(0.01f))
                        {
                            Entity->tMovement = 0.0f;
                            Entity->MovementFrom = Entity->P;
                            Entity->MovementTo = ClosestP;
                            Entity->MovementMode = MovementMode_Hopping;
                        }
                        
    --------------->    ddtBob = -20.0f*tHeadDistance;
                    } break;


then the further the HeadDistance is, the larger the lunge, so the player is really ready to make a huge jump
so here we just hardset the ddtBob = -20.0f*tHeadDistance;


29:22
so at this point, its starting to look very good, or at least taking shape


33:04
howevery, our current system right now is that our spring like movement system for the Cape will also oscilalte like a spring

right now we have an "undamped spring"



35:08
Casey explaining hooks law.

the more formal way of this formula is 

            F_spring = k * (P_target - P_current); 

P_target in physics, the equilabrium point
k is the spring constant, which is how stiff do you want this spring to be. 



so for a Dampled Harmonics Oscillator, the equation is 

            F = -kx - cv

http://230nsc1.phy-astr.gsu.edu/hbase/oscda.html


again, k is the spring constant, c is the damping constant
more generally it will look like  


            F_spring = -k * (P_target - P_current) - c * (v_target - v_current);  

so velocity target - velocity current.

in our case, the velocity target is 0. velocity current is just dtBob.

what is interesting about this formula is that the velocity 





37:36
Casey now adding this Damped Harmonics Oscillator into code.


                r32 Cp = 100.0f;
                r32 Cv = 10.0f;
                ddtBob += Cp*(0.0f - Entity->tBob) + Cv*(0.0f - Entity->dtBob);
                Entity->tBob += ddtBob*dt*dt + Entity->dtBob*dt;
                Entity->dtBob += ddtBob*dt;



40:07
Casey also mentioned that there are three types of springs:
overdamped, underdamped springs, and critically damped

here in this case, we want an underdamped springs so that gives us a wobble



46:44
Now Casey wants the head to snap to the body when the player isnt pushing it.
So when the player is moving the head around, we want it to be moving it around freely. But when the player lets go,
we want it to snap back to the correct location. 

so we want to apply the same technique of modelling it like a spring with the head. 