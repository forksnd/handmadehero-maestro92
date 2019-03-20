Handmade Hero Chat 010 - Partial Specialization


1:35
Casey said that someone mentioned "References Prevent Null pointers".

Casey went on a rant explaining how is that not true 


14:53
someone asked about what is the difference between "interpretation" and "translation"

Casey says that his understanding of this question is from the angle of binary command stream

lets say I am going to write an "interpreter" for x64,
that means I am going to write a piece of code, which itself is actually running, 

and its going to look at the individual pieces of the instructions, as they would be executed.

so its essentially software that acts like a intel CPU. 

so its a software version of a CPU.

an interpreter is more or less an emulator 


Translation on the other hand, implies a single process applied once that produces a new piece of code.
(like a compiler?);



20:25
Casey answering the big question.
someone asked about how some games uses offline rendered art for certain elements, while still using real time rendered
art for other elements. 

for example, we pre-rendered some sprites, and we render it in game. 

so games can generate special visual effects using a combination of pre-rendered art and real time rendering.

This calls into a category of computing, called "Partial Specialization" or "Partial Evaluation"
apparently resident evil did a lot of those.

so for example, lets say you have something like:

                int foo(int a, int b)
                {
                    int c = 5 * a + 6 * b;
                    return c;
                }

so lets assume we went back in history, and we are on the most primitive CPU, and this is very expensive to compute.
in modern days, this is laughable to compute. 
lets assyme that the one line 

                int c = 5 * a + 6 * b;

took 80% of our time. and now we want to think about how do we optimize our program. 


lets say we did some analysis, and we found that in 

    20% of the cases, b is 7. 
    30% of the cases, b is 9.
    25% of the cases, b is 10.
    the rest b is random



so in this case, that means for (20 + 30 + 25); 75% of the time, that i dont really need to do the "6 * b" operation.

so what we can do is to create 4 versions of foo(); each one being specialized to a certain paramter space, while still allowing 
some freedom in the rest of the parameter space. 

so now the expression becomes 

    int c = 5 * a + constant term;

so we remove 1/3 of the work. So that is partial specialization in a nutshell. 
so you are taking a general equation, and you are removing some of the parameter space. 
It is called partial because we are not specifying all of the parmeters, we are only specifying some of them. 


28:30
so how does this apply to resident evil or final fantasy?

If you think about the 3D rendering pipeline, as a giant equation, you can think of any given pixel having that equation.




49:07
in a lot of ways parital specialization is like optimizations. you are trying to see where you can get away with not doing work 