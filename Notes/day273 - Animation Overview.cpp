Handmade Hero Day 273 - Animation Overview

Summary:
talked the concept of how animations is about an interval of time. It is accomplished by having the camera 
letting photons in to form the picture during the exposure time. So it depends on the shutter speed.

emphasized that in games, we are gonna do instantaneous animations, which is a hack of real life.

talked about the difference between animation simulation and physics simulation to produce the state of an entity/world

talked about different animation phases in a full jump animation, the launch, the ballistic and the landing phase.

implemented the code for these three phases.


described the difference between b-spline and bezier curves 


Keyword:
Animation, movement system


Casey talking about the topic of animations in game


so if you think about movies, you have the concept of a camera and the exposure time 



             ___________  _
            |           |/
            |           |
            |___________|\_
                 / \
                /   \
               /     \

so during the exposure time, all the photons come in. 
so what that means is that a single frame of animation is not techinically an instant in time.
what it actually is is an interval of time, and that interval is exactly the exposure time, (exactly how long the aperture was open for);

 
so if Casey was using a camera, and he sets the shutter speed to be 1/60 th of a second, so that means we have 16.6 ms worth of photos 
coming in. 

so if we are really talking about animations in its broadest sense is that, we want to capture an interval of time.
so we arent trying to find where the object is in an discrete absolute instance of time. We are actually talking about where the object move
during a particular interval. 

the reason why this concept is important is that, if you have two frames 


         frame 0               frame 1      
     _______________       _______________     
    |               |     |               |     
    |               |     |               |      
    |    ####       |     |          #### |           
    |    ####       |     |          #### |    
    |_______________|     |_______________|     
    
so if I go see a movie, this is only happening 24 frames a second. But it still looks great. 
but if a game runs 24 frames a second, it s not so great. 

part of the reason why is that when you are shooting a film, you get motion blurr. That motion blurr is happening 
because we are capturing an interval.

so in movies, because of motion blurr, the picture may look like, which makes the motion look a lot more rich

         frame 0     
     _______________ 
    |               |
    |               |
    |....####       |
    |....####       |
    |_______________|

again in computer graphics, we are talking about how to draw an interval time, not an instant of time 


unfortunately, in modern day computers, we arent that fortunate. there are ways to approximate motion blurrs, but they arent really correct.
so a lot of times we dont do motion blurr at all. we just try to get out high frame rate, such as 60 frames per second, and hoping things
dont look that ugly at that rate. However, this certainly does lead to certain crispyness in the images you see in CG that 
doesnt have motion blurr. Even at 60 frames per second, there will be motion blurr on fast moving things, especially on things such as bullets
or fast cars. 


9:25
in handmade hero, we are going to instantaneous. if we assume our frame rate is high enough, and we dont care too much about motion blurr, 
or even for things that we want motion blurr, we will hack it. 

you will have to understand that instantaneous frames are hacks, as in real life it doesnt work that way. You cant have instantaneous
instance where photos come in and paint your picture.

12:22
instantaneous animation is about being able to take some sort of a function, sampled at a time, and we want to produce the state of the world

                f(t) ---> state of the world;

so we want to render the world and we want to render the world at a time.

so this is essentially what we are trying to do with animations. 
another thing to note that usually in games, we want the flexibility to sample these function at arbituary time

                t1 = t0 + 1/60s

for example, we dont want to have a discrete step, where you have 

                f(0) ---> state 0;
                f(1) ---> state 1;
                f(2) ---> state 2;

in nowadays, games run on multiple hardware. we dont know that our game runs on 1/60th frame rate
it could be 1/50th of a second, it could be 1/30th of a second

so the idea is that we want to be able to increment our time stamp in any increment we want 

                t1 = t0 + dt;


17:26
so in terms of the state of the world, that usually consists of states of system or state of entity


lets take the example of position of entity, there are usually two types of functions:

1.  if we have function at time t, and output the exact value at the time 
                
                f(t) ---> P;

2.  you have other ones where it requires the previous value. so a function that 

                f(t0) + f'(t0) * dt;

    this is also called the forward euler 

    and this is also a numerical integration method. this is very crappy, but in games, its fine. 


a lot of times, there are to many variables that we cant write the position update in terms of method 1,
so we settle with method 2.

Method 1 is very accurate and very beautifle, but you can only use method 1 if you can get away with it. 
For example, in game 2, we would model the height of the projectile. The projectile height is not affected by anything else,
therefore you can model it with method 1.

but alot of times in games, you cant do it. 



21:28
typically, even though these two methods are all the same unified concept on a high enough level,

we typically call method 1 animation, method 2 physics. 

like, if you put in the t and it outputs the state, we typically call that the animation of a character
when we just put in a dt, and we try to solve for the state after a bunch of stuff happens, twe call that physics. 






23:00
so here when I mention "Animation", I am talking about f(t); kind of thing, what are some of the techniques involved?

b-spline is very common. 
a b-spline is what lots of animation packages use. So if you have a animation curve in space, and you author a bunch of b-spline
handles. 

B-spline, stands for basis-spline, is a spline function. its like basis points that represents a curve. 

b-spline is like the work horce of animations typically. 

a 0th order b-splipe is a step, where it teleports from point a to point b

                            a               b

a 1st order b-spline is a line, where there is a line lerping from a to b

                            a---------------------b

a 2nd order b-spline is a parabola 

                                   b
                                  .  .
                               .        .
                             .            .
                            a               c


a 3rd order b-spline is where you have a wiggle

                                   b              D
                                  .  .           .    
                               .        .      .
                             .            .  .
                            a              c


28:27
so b-spline is a very general, versatile tool that all animators uses.

but the thing to know is that, for the f(t) that is producing our state;


                f(t) ---> state;

could be formulated as a spline function, or it could be formulated as any function that we want. 


31:14
Casey now wants to talk about the "parameters" of a state,
or we call it a "state vector"
in physics terminology, its called "generalized coordiantes"


recall in our example, we have the ground point and the head height 


                o  head height 
                |
                |
                |
                |
                x  ground point 

the head height is f(t) = height;

the ground point(the body) is physically simulated f(t0) + delta * f'(t);



so the state vector for this system is 

        |   ground_x    |
        |   ground_x    |
        |   ground_x    |
        |   head height |
        |               |

which has 4 components



46:01
Casey mentioned that he wants to add a bit of "anticipation" animation to the character movement code,
"anticipation" here means, we want the character to squad down a bit before jumping, like what people do in real life. 

right now our f(t) is just a parabola;

                            ..         
                         .      .      
                       .          .    
                      .             .  
                     .               . 
                    .                 .

                  t = 0              t = 1


so if we want to add in the anticipation animation to our formula, what we want to do is break the range into sections 

    anticipation        ballistic                   follow through
        |                   |                           |
        |                   |                           |
        v                   v                           v
    |------------|-------------------------------|-------------|

    0                                                          1


colloquially, the sections are also called 

launch, ballistic and land

so the idea is that you take your t, and see which section you are in.
then within the section you do all your calculation from 0 - 1 again

we always want to normalize time t within the seciton, cuz thats easier.

50:52
so Casey changed the hopping code, to add the different phases

                case EntityType_HeroBody:
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

Q/A

1:04:51
what is the difference between b-splines and bezier curves?

the short answer in terms of the curves they produce is no difference. 
they are the exact same curve, but the terminology are just phrased differently 


Bezier                              b-spline

                                    "knot" --> t values
                                    "Contact point" ---> position 


so if you have a curve like this 


                       b              d               f
                      .  .           .  .           .  . 
                   .        .      .       .      .       .
                 .            .  .           .  .           .
                a              c               e              g


this is actually two bezier curve, but one b-spline curve.

so if you have a rather long curve, like the one above, if you are representing this with bezier curves,
then it is solved by stringing multiple bezier curves torgether. Usually in games, you have 3rd orders beziler, 
so it can cover four points. so one 3rd order beziler, cover 4 control points, but it will only pass through the first point 
and the 4th control point. 

so in this case, the first bezier curve will cover point a, b, c, d. It will pass through point a and d. 
the 2nd bezier cur, will start from point d and end on g

in b-spline curve, if you feed the exact same control points, its just one b-spline curve, and it will only pass through 
point a and point g.

but in the b-spline curve case, if you put three points on d, then you will pass through point d, producing the exact same 
curve in the bezier curve case, but usually you dont want do that 
(not sure why its three points, i guess you have to do the math to find out);


the curve equation for the two is exactly the same. you can have a bezier curve and fit it with b-spline, or vice versa.
so there isnt any difference in the actual curve. the difference is how are you controlling that curve. 

meaning if you feed a the same control points to both, they wont produce the same curves. But there exists a set of control points 
that will allow me identical ones. 










