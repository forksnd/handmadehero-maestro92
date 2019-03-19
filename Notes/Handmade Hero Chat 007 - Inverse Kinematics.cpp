Handmade Hero Chat 007 - Inverse Kinematics

2:02
Casey first explaining what kinematics is 

there is this concept in compute graphics, called "multi-link assembly"
or you can think of it as an "ARM" or "LEG"

something that has multiple pieces of it, but can move independently, 

so when we talk about kinematics, we are talking about taking one of "multi-link assembly",
figuring it out where it is when we know its joint angles 

here, as an example, imagine we have a character, and its arm has two links, 
link 1 and link 2

and two joints, joint a and joint b


            ####
            ####
             ||
    _________||_______a
             ||        \     /
             ||        1\   /2 
             ||          \ /
             ||           b  
             ||
            /  \


4:46
so if we examine this arm in isolation 

        shoudler
            a                               hand
             \                             /
               \                         /  
                 \                  2  /
                   \ 1               /
                     \             /
                       \         /
                         \     /
                           \ /
                            b  
                          
                          elbow

so what do we need to know to figure out where the hand is?

what think we may need is the angle that the shoulder is coming out of the arm socket, angle d
and similarly, how wide my elboe has opened, angle e

        shoulder
            a                             hand
            |\                             /
            |  \                         /  
            |    \                     /
            | d    \                 /
            |        \             /
                       \  f(d)   /
                         \     /
                           \ /                       
                         elbow

so if we know angle d and angle e, and we have information on link 1 and link 2, then it will be possible
for us to calculate the position of the hand, since link 1 and link 2 doesnt change lengths 


so the process of figuring where something is based on initial position and a series of parameters that describe the "assembly"
that is "kinematics"

so lets say in this problem, we have the concept of a "state vector" (in this case, we can say the state is value of angle d and angle e)

        |         |
        | angle d |
        |         |
        | angle e |
        |         |


 so taking the state vector and produce Position Hand 

    S ---> P_hand 

we call that kinematcis 


7:59
we also have this notion of "Degrees of Freedom". it is used to describe how many variables we can change to effect 
the final value. Here, we can change the should angle and elbow angle, so in this case, DOF is 2.

the final result we care about also has two variables, which is P_handX and P_handY.

so the number of varaibles in any varaible, that is the number of degree of freedom 

so the input here is 2 DOF, the output is also 2 DOF


11:43
so this "arm assembly" and degree of problem could get pretty complicated
consider this example 



        shoulder 
------------a----------------------------- hand--------------
            |\                             /
            |  \                         /  
            |    \                     /
            | d    \                 /
            |        \             /
                       \  f(d)   /
                         \     /
                           \ /                       
                         elbow

so lets say first angle is d, the 2nd angle is a function of d,

then the state vector is only one variable 

        |         |
        |         |
        | angle d |
        |         |
        |         |
        



in this case, no matter what d value i give, f(d) also changes so that my hand automatically slide along the x axis, and never leave the y axis
so the degree of freedom would appear to be 2, P_handX and P_handY. But y in this case is a constant 

        |         |
        | P_handX |
        |         |
        | P_handY |
        |         |
so in truth, this only has one degree of freedom.

so figuring the degree of freedom is somewhat tricky. you really have to think about how many DOF your problem really has. 


13:20
so to give an overview of kinematcis

we are taking a state vector that is describing an "assembly", 



13:48
Casey describes one more term here, which is "End Effector"
the end result that we care about when we do kinematics (in this case, P_handX and P_handY); that is called the "End Effector"

15:04
Casey proceeds to talk about what "Inverse kinematics" is 

so if "Kinematics" is 
taking state vector and producing End Effector State 

            State vector -------> End Effector state 

(the End Effector state is the position of the hand);

Inverse kinematics is 

            End Effector state  ------->  State vector


so why do we care about this? because almost all the time, when you are actually dealing with these sorts of things,
the Kinematics is what the renderer does. 

so any renderer that wants to deal with "articulated bodies" (such as ragdolls); will have some kind of state vector of all the joints, and it 
has to produce the 3D positions of all things, so it can render it 

17:48
but if you want to do modeling work, lets say the animator does rigging and it will control what it is suppose to happen, what the hand positions is 
supposed to be, then you need inverse kinematics. 

For example, if you have a robot arm that the animator animated from state a to state b, where the arm is just extended further 
the animator would want the computers to solve what the state vector would be (what the joint angles would be);


state a                                             state b

    shoulder                                          shoulder                                   hand                      
        a                             hand                a                                   ___/                                   
         \_                           _/                   \___                           ___/        
           \_                       _/                         \___                   ___/                                        
             \_                   _/                               \___           ___/                               
               \_               _/                                     \___   ___/                                                    
                 \_           _/                                           \_/                                                          
                   \_       _/                                             elbow                                                       
                     \_   _/                                                                                                
                       \_/                                                                                                                                                    
                     elbow                     


this is what inverse kinematics is. 


19:37
so how do we do that?
Inverse Kinematics is a very very very very nasty problem 


the reason is because of the following very tricky nasty reasons.
1.  inverse trig is always harder than trig 
2.  its typically underdetermined. and if not, its discontinuous
3.  There are usually joint limits 


24:55
Casey explaing what "underdetermined" and "overdetermined"

overdetermined is when you have a system, that has more requirements for how it has to be setup than it has variables 
to actually satisfy them 
doesnt really come up in Inverse Kinematics problems 


Inverse Kinematics(IK); usually encouters underdetermined problems. 
this reason is because normally for IK, you are dealing with system that has a lot of links.

so for a 2-link arm case (in a 2D game, where we only care about the shoulder angle and the elbow agnel);, 
the output we care about is still gonna be the position of the hand 

                (x, y);

and we also have 2 degrees of freedom that we can use to position it. 
the degrees of freedom in the input space and the output space is the same 

meaning if we are talking about how many varaibles we have in the input state vector vs
the number of variables we have in the output, they are equal. 

so two variables, and two unknowns, very easy to solve

but if we were to include the neck joint, waist joint, leg joint, are all allowed to move, to have handX and handY to 
reach at a certain position,

so our output hand position, if we are 2D, will have 2 DOF 
if we are in 3D, then we have 3 DOF 

but our input, including all of our joints, neck, waist, leg, elbow, shoulder, will have 5 DOF
so that is an underdetermined case, we dont have enough equasions to solve for all these variables 

in math class, we are all learned that to solve for for every unknown, we need one equation 
in our case, the DOF are essentially unknowns. 

32:11
so when we end up in a situation where we have a state vector that has more variables to solve for than our output,
(essentially a underdetermined system);

then we need to have some way of mayking up what the joint angles should be. 

one very typically way to do is to get the solution that makes you move the least from your current location.
which means, now we are solving for a solution that gets my hand position to the desired location, and we are subjecting it to a 
minimization criteria. 

typically, there is only one smallest movement solution, and now I have a shot at solving it. 

sometimes there may be a "rest" state, and we want a solution that is as close to the rest state as possible. 

or another solution may be, try to distribute the motion event among all joints. 


34:45
so even when you dont have the underdetermined problem, you may still have the problme of discontinuouty.
what that means is that, often times when you have direct solutions, they may end up being discontinuous

see image explanation by Casey at 35:35


this is becuz sometimes there is this positive/negative solution to a problem.
a lot of solutions has a positive / negative choice


36:54
so besides all of that, we still have the third problem, which is joint limits 

in our arm example, there limits of the angles where the shoulder cant move past 
that is the same for our human body.


so for math solution, there are boundary conditions. this interacts with our problem 2.
so IK could be very very nasty. 




43:25
luckily, most of the time, the need for an accurate solution for IK problems is not there. 
in an animation context, is not really about getting a precise mathematical answer. 
so if you are just looking for answer that achieves the goal of positioning the leg or hand at 
a location reasonably well. Even though, the problem specification is super nasty, there are some cheesy
algorithms that you can use in a game context and not worry to much, espeically, if you are just using it for fix up. 

for example, if you are taking someone else_s animation and you are trying to tweak it to complete a "pick up" animation. 
that is not so hard, a simple iterative that moves the joint a little bit will work. 



47:55
someone mentioned that he was intending to use IK to solve for foot placement on an uneven terrain. Is that hard?

no, its not hard. There are recursive/Iterative descent, that are super trivial to implement.

