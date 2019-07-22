Handmade Hero Day 293 - Moving Familiars

Summary:
finished logic for Familiars following the main hero

Keyword:
entity system, game logic







27:06
Casey mentioned the concept of Proportional Derivative Controller and Proportional Integral Controller

Proportional Derivative Controller


	accerleration (p 2nd derivative) = c0 (p_target - pos) + c1(p_velocity_target - vel); 

so what essentially this formula is trying to drive towards the target position and target velocity
the acceration will always be in the direction where pos approaches p_target and vel approaches p_velocity_target

50:25
Casey wants to profile the functions

53:20
after profiling, Casey mentioned says that it bodes well for this entity system, cuz Casey is running on a slow machine
and still getting a solid 60 hertz. The game has tons of entities and Casey has done zero optimizations efforts.


Q/A

1:02:46
any thoughts on multithreading the entity system

it depends on what you want to multi-thread. there are two things you might think about multithreading wise.
one is multi-thread the current entity sim to make it faster. Casey think that at the current state, its not a reasonable idea 
we havent done any optimizations yet, and we are hundres of entities and we are still fine on release mode.

so making it run faster for multithreading not worht it at this point (maybe later);


2nd, if we want to multithread update other entities in the world, that is mostly done already. we are very multithreaded compatible


the other thing that we might want to multithread in the future is some heavy weight ai thing, such as path finding. 



1:06:44
do you have a max entity count on the sim region?

Casey showed that we have 740 ~ 1121 entities. 
