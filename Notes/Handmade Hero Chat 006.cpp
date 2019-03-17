Handmade Hero Chat 006

1:05
talks about what Bresenham is 

1:32
Casey describes that Bresenham fits under a more general scheme, which is error based rasterization. 
you can use this to draw anything. You dont have to use to just draw lines. you can use to draw any shape
that has an implicit formalization of some kind.


[essentially, this is the basis of the pizza drawing algorithm]



2:38
so the main question is, what is Bresenham algorithm and why do we care?

in the old old old old days, 

imagine you have a FrameBuffer that is being drawn out to the CRT monitor. 
drawing graphics was incredibly expensive. Processors was super slow, and you drew things on pixel at a time, and you hope its fast enough that the user 
doesnt get bored watching it. 

Literaly. If you look at the kings quest, you can see the the pixels getting filled on screen. 

so in those days, the performance characteristics of algorihtms looked a lot different than they look now. 
it wasnt about minimizing cache misses, it wasnt about trying to keep a memory footprint, it wasnt about parallelizing instructions,
it wasnt about multithreading.

You literally have the CPU as the total gaining factor. So you are just trying to minimze the number of operations to draw a line.

[for our battle server, we have a bunch of threads, but each thread is running simulation, so in a sense, we are still dealing with 
    single threadded code, so we are also on the task of minimizing CPU instructions].

so the line drawing algorithm is just about how to draw the line fast. (the line verison of the pizza algorithm);
this algorithm, in todays idea, will look like an laughable idea. 

+so there maybe a lot of restrictions back in the day, maybe there is no floating math, so that we cant even normalize the slope or what not. 
or having to do a divide.
         _________________________________________________
        |    |    |    |    |    |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |  a | a1 |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |    | a2 |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |    |    |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |    |    |    |    |    |    |  b |
        |____|____|____|____|____|____|____|____|____|____|


pretty much anything that you think that is very trivial to do today, you might not get it at that time. 
At that time, its possible that you only have integer units. 

like the pizza algorithm, if we were to draw a line from a to b, for our first move, its about either choosing a1 or a2.
it comes down to choosing a pixel.

this is called an "error based" algorithm cuz you are minimizing the error.

17:05
now in a modern context, the way we would want to do this is to because as we render line, we are advancing the line pixel by pixel towards
destination. We may want to consider this a in a parallelized way. so if we can somehow not have dependencies from pixel 1 with pixel 3.


49:37
Casey mentioned that there are two ways to choose the pixel in the line drawing algorithm depending on whether you want to overfill or not 

one is this 
         _________________________________________________
        |    |    |    |    |    |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |  a | a1 |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |    | a2 |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |    |    |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |    |    |    |    |    |    |  b |
        |____|____|____|____|____|____|____|____|____|____|

the other is 

         _________________________________________________
        |    |    |    |    |    |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |  a | a1 |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    | a2 |    |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |    |    |    |    |    |    |    |
        |____|____|____|____|____|____|____|____|____|____|
        |    |    |    |    |    |    |    |    |    |  b |
        |____|____|____|____|____|____|____|____|____|____|

bot are valid, all depends on how you want to draw it. 
There are Bresenham for both. you just change  your error formula depending on which pixel you are considering