Handmade Hero Day 272 - Explicit Movement Transitions

Summary:

Casey gave his thoughts on Finite State Machine and why he thinks that Finite State Machine is not that useful in programming. 

added the hopping motion to the characters. demonstrated how he broke down how he solved the parablic equation for the character movement 
system

Keyword:
movement system 


7:46
Casey mentions that the goal now is that although we have the body locked on to grid centers,
but when we move the head, we want the body to lean left or lean right, and only when the head has gotten 
to the mid point, then the body actually will "jump" to the next traversable point.


12:52
Casey voicing his concerns about his current movement code.


                case EntityType_HeroBody:
                {
                    // TODO(casey): Make spatial queries easy for things!
                    sim_entity *Head = Entity->Head.Ptr;
                    if(Head)
                    {
                        r32 ClosestDistanceSq = Square(1000.0f);
                        v3 ClosestP = Entity->P;
                        sim_entity *TestEntity = SimRegion->Entities;
                        for(uint32 TestEntityIndex = 0;
                            TestEntityIndex < SimRegion->EntityCount;
                            ++TestEntityIndex, ++TestEntity)
                        {
                            sim_entity_collision_volume_group *VolGroup = TestEntity->Collision;
                            for(u32 PIndex = 0;
                                PIndex < VolGroup->TraversableCount;
                                ++PIndex)
                            {
                                sim_entity_traversable_point P = 
                                    GetSimSpaceTraversable(TestEntity, PIndex);

                                v3 HeadToPoint = P.P - Head->P;

                                real32 TestDSq = LengthSq(HeadToPoint);            
                                if(ClosestDistanceSq > TestDSq)
                                {
                                    ClosestP = P.P;
                                    ClosestDistanceSq = TestDSq;
                                }
                            }
                        }

                        ddP = (ClosestP - Entity->P);
                        Entity->Collision = WorldMode->NullCollision;
                        MoveSpec.UnitMaxAccelVector = true;
                        MoveSpec.Speed = 100.0f;
                        MoveSpec.Drag = 10.0f;
                    }
                } break;

so assume we have the body position and the target position
currently for the body position, when it comes to rest with the target position, the movement is not very smooth
its sloppy and it overshoots (like a spring, as I mentioned in day 271); 

It doesnt have a conception that it is actually on the point.
the only hacky thing we can add right now is to add an episilon, to check how close you are to the point. If you are close enough
then just clamp to it. 

so the idea is to add the concepts of states
for example, we can add the "Stand" state and the "movement" state, two separate modes. 


14:47
on a side note, Casey mentioned that Finite state machine is one of those glorified terms that isnt very useful.
because it makes it sound like, there is a formalism, when often times there really isnt a formalism and often times you dont want a 
formalism

Casey mentions that FSM has a lot of formal concepts that are useful in CS not very useful in programming. 
Casey thinks is because the CS people approaches the problem the reverse direction that you should do.
FSM is like Object Oriented Programming. They are basically a way of trying to pretend that the notion of what "state" your are in 
is the most important thing, and everything else comes after this "state"

in actual coding, Finite State Machine tends to lead people down the OOP trail


a lot of times you may want to start thinking that these are some major stages, but in reality there are really lots of fluidity
between these stages, and often times you dont want to think your program entirely being in one state or another. 
so you kind of want the notion of state come out of the natural coding process, and you dont want clamp this concept of a finite state 
machine down to the code, because it has all of these negative effects that OOP has. 

It doesnt mean you wont get your code to have some finite state machine elements in it, and you may argue that currently in our 
handmade hero code base, we may already have lots of code that looks like finite state machines.

So have states is not a bad thing, but arriving at code that is containing states is just a bad way of approaching the problem.
you want the code to come out the other way around. 

pretty much any program of any complexity, will have somethings in it, that will look like or literally are finite state machine.


So Casey wants to increase the stateness of this motion code. 



19:06
so Casey added the enum sim_movement_mode 


                handmade_sim_region.h 

                enum sim_movement_mode
                {
                    MovementMode_Planted,
                    MovementMode_Hopping,
                };
                struct sim_entity
                {
                    ...
                    ...
                };





20:19
then in EntityType_HeroBody, we have different logics depending on the movement modes 

you can see that in each mode, we have logic to transition to other modes

for now we just have a tolerance value 



                case EntityType_HeroBody:
                {
                    sim_entity *Head = Entity->Head.Ptr;
                    if(Head)
                    {
                        ..........................................
                        ...... logic for chasing the head ........
                        ..........................................
                        
                        switch(Entity->MovementMode)
                        {
                            case MovementMode_Planted:
                            {
                                if(BodyDistance > Square(0.01f))
                                {
                                    Entity->MovementMode = MovementMode_Hopping;
                                }
                            } break;

                            case MovementMode_Hopping:
                            {
                                if(BodyDistance < Square(0.01f))
                                {
                                    Entity->dP = V3(0, 0, 0);
                                    Entity->MovementMode = MovementMode_Planted;
                                }
                                else
                                {
                                    ddP = (ClosestP - Entity->P);
                                    MoveSpec.UnitMaxAccelVector = true;
                                    MoveSpec.Speed = 100.0f;
                                    MoveSpec.Drag = 10.0f;
                                }
      
                            } break;
                        }
                    }
                } break;


23:29
Casey mentioned that nowadays, the performance costs for sines, cosines and sq roots are nothing when compared to memory accesses





29:18
Now Casey mentioned that he would like make the movement a more animation style movement.

Casey no longer likes the chasing vector motion, and he wants it to be more procedural

Casey does have concerns that this approach may be a mistake.


[so you can think of it that we have free control of the head, but we are developing a feel for the body 
following the head.

I feel like you can sort of use this code to write a camera following a character, in a third person POV games]

                case EntityType_HeroBody:
                {
                    sim_entity *Head = Entity->Head.Ptr;
                    if(Head)
                    {
                        ..........................................
                        ...... logic for chasing the head ........
                        ..........................................
                        
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
                            } break;

                            case MovementMode_Hopping:
                            {
                                Entity->tMovement += 6.0f*dt;
                                r32 t = Entity->tMovement;
                                v3 a = V3(0, -2.0f, 0.0f);
                                v3 b = (Entity->MovementTo - Entity->MovementFrom) - a;
                                Entity->P = a*t*t + b*t + Entity->MovementFrom;
                                Entity->dP = V3(0, 0, 0);
                                
                                if(Entity->tMovement >= 1.0f)
                                {
                                    Entity->MovementMode = MovementMode_Planted;
                                }
                            } break;
                        }
                    }
                } break;



notice in the MovementMode_Hopping case, the if condition we use is 
                
                if(Entity->tMovement >= 1.0f)

our previous has set tMovement in the range of 0.0 ~ 1.0f, which makes it easier to work with


47:01
Casey mentioned that he wants to add a hoppping motion
so the idea is that hopping, under the influecne of gravity is a parabolic function 

                h = a * t^2 + b * t + c 


assume at position t0, you are at P_from 
at position t1, you are at P_to 

f(t0) = P_from;
f(t1) = P_to;
so you just gotta solve this equation to get the motion that you want 


Casey mentions that sometimes you don thave to fully solve for equations

so imagine that to t = 0 and t = 1;

so with t = 0, you get 

                P_from = c 

with t = 1, you get 

                P_to = a + b + P_from
                P_to - P_from = a + b;

which means that when we finish jump, the sume of the unknown terms, a and b, equals the velocity,
which is quite interesitng


so  a = delta - b;
    b = delta - a;

which means if we solve for one, and get the other, we will have a proper parabolic arch 


Casey says a lot of times, you dont even have to solve for equations, you just have to partially solve for 
equations. So in this case, you can just freely make up two vectors, as long as they sum up to the delta


therefore as you can see, we have the following code:

                case MovementMode_Hopping:
                {
                    Entity->tMovement += 6.0f*dt;
                    r32 t = Entity->tMovement;
    ---------->     v3 a = V3(0, -2.0f, 0.0f);
                    v3 b = (Entity->MovementTo - Entity->MovementFrom) - a;
                    Entity->P = a*t*t + b*t + Entity->MovementFrom;
                    Entity->dP = V3(0, 0, 0);
                    
                    if(Entity->tMovement >= 1.0f)
                    {
                        Entity->MovementMode = MovementMode_Planted;
                    }
                } break;


and now we are getting the hopping motion


so Casey made the height of the character parabolic during the hopping, but the x and y plane position is still linear
 


58:42
short lesson, if you are trying to solve for an equation, first reduce the equation down first
and then solve for the last few variables then 