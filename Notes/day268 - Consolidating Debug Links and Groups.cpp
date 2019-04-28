Handmade Hero Day 268 - Consolidating Debug Links and Groups

Summary:
this episode is focused on cleaning up as we are wrapping on the debugger system 

cleaned up the debug element cloning functionality

discussed:
linkedlist vs array

Keyword:
code clean up


2:29
for future features to add, Casey would like to add a memory profiler 



16:30
Casey describing one of his programming habits

Casey refers it as "three part method of function calling"

first, there are the global contextual state, that is only relevant to a particular section or thread 
2nd, you have the local variable
thirdly, you have the actualy variable that you are operating on.

                handmade_debug.cpp

                internal void DrawTreeLink(debug_state *DebugState, layout *Layout, debug_variable_link *link);
                {

                }

in this case, debug_state *DebugState is like the global variable
layout *layout is like the local variable 
 debug_variable_link *link, is the variable you are operating on.

there tends to 3 general layers of scopes of variables that you are operating on. 

the global state that is relevant to this portion of the program  
the context of the operation, 
the immediate variable you are operating on 




cleanup and the cloning functionality


Q/A
1:05:13
Casey mentioned that he likes to make his linked list circular, that way he never has to check for null 

1:15:40
thoughts on linked list and its cache friendliness?


Casey says he doesnt like arrays that much cuz its not very orderable.


Casey says that if he is actually performance evaluating some code, and he wants to know this particular code,
then he might try array vs linkedlist.

Casey claims that he rarely find arrays is much faster than linkedlist. (maybe he does pushstruct style linked list);

infact, Casey says that when he was doing performance optimizations on the witness, there was actually notice 
between using arrays vs using linked lists. 

Casey says that alot of people say that to him, but he suspects if people are actually comparing apples to apples 
he thinks that most of the cache friendliness of a linkedlist vs array will have no difference, if your container size 
is bigger than the cache. 


the reason for that is because, lets say 

we have below, we have a linkedlist node with a next pointer 

                link 
                    next 

vs an array 
                 _______________
                |               |
                |     link      |
                |     link      |
                |     link      |
                |     link      |
                |     link      |
                |               |
                |_______________|
            
however much linkedlist nodes fits in the cache vs how many array elements u have in the cache 
depends on how large the next pointer puts you over the cache limit. 
then you are pay for the cost.

[didnt understand this part? Casey did an example with math, which I didnt quite follow]



Casey says since he does pushstruct style linkedlist, all his linked list nodes are all in the same memory page anyway. 

whatother people do is that they call new, and who knows where would new(); would put you in memory. they would be all over memory 



so when people mention that arrays are faster than linkedlist, maybe its in a very performance critical part, and at that point 
you obviously dont want to have anything extra, so at that point, if you know how big the array should be, and you are not gonna 
do alot of shuffling, that gonna be better. 

Casey claims that he will never tell people to use linkedlist for performance, unless there is a lot of shuffling.

so regarding array vs linkedlist performance 

are linkedlist faster than array?
no

are they slower?
sometimes, but not by much.


so for many cases, Casey chooses linkedlist because its more flexible, and there is no weird memory bound weirdness 

Casey does note that he tends to not use them in performance critical sections of code, so it doesnt matter. 



1:21:14
Casey also mentioned that Concurrency mattes, linkedlist can be better if you have multiple threads working
at the same time 

Arrays are real bad in Concurrency situation. for example, you have to lock down the array anytime you want to add something to it. 
[didnt understand this part]


general take away:
if it is performance critical code,
you gotta time it.

if it is not performance critical code,
Casey hasnt found linkedlist to be awfully slow, like what the mas majority of people think 

so Casey usually writes the one that is easier to use instead of worrying about it. 