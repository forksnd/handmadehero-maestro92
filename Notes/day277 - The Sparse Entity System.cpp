Handmade Hero Day 277 - The Sparse Entity System

Summary:
Casey talked about what entity system he will do for handmade hero
talked about the common types of entity system
talked about what inheritance is

Keyword:
entity system

7:43
Casey address that he would like to take a look at the entities system and the structure of the world 

10:32
so there are lots of ways you can do entity systems. 
And there isnt a holy grail of entity system that someone figured out at some point, and if you just do it this way 
it will work out fine. Casey says he has never seen that to be the case. 

there is a lot of terminologies out there, there is OOP, there is entity component system. But alot of it is underbaked.
This isnt compiler theory, its not a very strict dicipline. its still pretty maverick. Its a real black art kind of thing.




12:18
there has been a lot of different ways that people write entity system. obviously, there are ones that look like the traditional
object oriented program. and the ways these look are (the traditional inheritance model);

                 ____________
                |  entity    |
                |____________|

                /            \
          _____/____        __\________
         |   Orcs   |      |  Humans   |
         |__________|      |___________|


in general this style, very old school OOP is no longer a good idea. Pretty much no one thinks is a good idea anymore. 
the reason is that, there are not rigid categorizations for things. This method is just not very flexible





14:43
what people tend to do nowadays is they use OOP "component" approach


                 ____________
                |  entity    |
                |____________|

                /            \
          _____/____        __\________
         |transform |      |  health   |
         |__________|      |___________|



so an example will be 

                struct Orc : public entity 
                {
                    Health health;
                    Transform transform;
                    ...
                    ...

                };



17:08
another way is the "Act/React" model, which is someething from Looking Glass
(essentially the ECS system in game2);



the component model vs the "Act/React" model kind of models the "AOS" vs "SOA" apporach 



    Component                               Act/React 
    "AOS"                                   "SOA"

------------------------------------------------------
                            |
                            |       
(what we do in dominations) |   Entity Component system (game2);
                            |
                            |
                        

20:07
as you can see, an entity system is a very tricky problem. You want game play to be very flexible.
You typically want everything to happen everywhere except when you dont.


21:28
Casey will now address some of the problems you will see 
lets say you are doing the Orc struct the OOP component way

it will be very fast to process an Orc struct for any code that is doing stuff on Orc.
It will be very easy to write that code. The compiler knows exactly where each member fields are, it doesnt have to 
get them from other places. 

but compare to the ECS system, everytime you process an orc, you have to look up from each component array.


24:56
what we are gonna do on Handmade hero is a complete brand new entity system, one that no one has ever done this style before.


Casey will do the "Sparse Entity System". This was Casey_s way of looking at entities, that preserved all of the speed of how 
you will write code in C to deal with stuff, but still have all of the benefits of entity to do anything and become anything 
in anyway. 

so this is something Casey came up with.

the first thing Casey would like to address is inheritance.
inheritance is sort of like saying, 

so lets take an example, where we have the foo struct, ad we have the a defatify function.

                struct foo
                {
                    int x;
                    bool isFat;                    
                };

                void defatify1(foo* Foo)
                {
                    if(Foo-> < 10)
                    {
                        Foo->fat = false;
                    }
                }

so between between defatify like the way above and writing defatify in the equivalent way below:

                void defatify2(int* x, bool* fat);
                {
                    if(*x < 10)
                    {
                        *fat = false;
                    }
                }

so when you convert from defatify1 to defatify2 is that you have made the funciton more flexibile, since now you can operate 
on any pair of x and fat, 
instead of having to operate on ones that are next to each other (in side foo, int x and bool isFat is right next to each other);

so in a sense, struct foo and defatify1 is an optimization of defatify2

first its an optimization cuz we can type less

2nd, this also helps the compiler to optimize because now the compiler knows there is a fixed offset between x and isFat,
so the processor can do less work to accomplish the same machine code.

So defatify2 is more cumbersome and slower potentially 




33:08
so inheritance is essentially the same sort of understanding, if I say I would like to inherit from struct foo, lets say 
we have struct bar. Obviously what we could do, with out lost of generallity is to to put a foo in bar. 

                struct bar
                {
                    foo Foo;
                };

so sometimes we definitly want to have more information to it 

                struct bar
                {
                    foo Foo;
                    float damage;
                };

so right now lets say we want to call the defatify function, we can just pass the address of bar.foo to it 

but immeidately, you can see that this becomes cumbersome, cuz this results in so much more typing. 


so inheritance is just a way off saying 

                struct bar : public foo
                {
                    float damage;
                };

so defatify2 is more powerful, but more typing, more semantic work.


36:25
So Casey says lets see exactly when does the C++ inheritance way, defatify1(); starts to fall apart 

so when we create one of these structs
                
                struct necro : public health, public burnable
                {
                    ...
                    ...
                };

so you can this leads to the "multiple inheritance" case. This is very problematic 

now we cant even do the optimization that in defatify1, where the compiler just expects the Foo pointer in bar 
to be at the beginning of the the bar struct.

previously a pointer to bar is essentailly a pointer to Foo, since Foo is the frst thing in bar. 


39:17
also in c++ multiple inheritance, this will further break down if you want things to be dynamic. 

meaning lets say an entity gets casted by a spell, and he gets a shield buff.

you cant add an inheritance to a C++ struct at runtime. 


40:35
Now Casey wants to point out things about inheritance that people usually dont talk about 

inheritance is compression. its an compression algorithm. that is all what inheritance is.

lets say I would take every possible thing that can be ever contained in an entity. lets say there are 200 properites.
and I put all of these in one big struct. 

anything that everyone every wanted, we put it in here.

for example, lets say the health struct has a int x;

                struct health
                {
                    int max;
                };

and in transform, we got x y z 
                
                struct transform
                {
                    int x; 
                    int y; 
                    int z;
                };

soo all of these properties, we put it under entity 

                struct entity
                {
                    int healthMax;
                    int x;
                    int y;
                    int z;

                    ...
                    ...

                };

imagine a 64k struct.
if we have this entity struct, that that could be the thing that I pass to any function. 

so what you are doing when you creat an inheritance hierachy, is that you are just compressing things.
yo uare doing compression on the structs that you have by saying, instead of putting everything in one giant struct,
im just gonna the ones he will use. out of the inhertiance hiearchy.

inhertiance is just a compression algorithm that lets you store data that you dont need. thats it.


this is actually a very important, storing less data. but it comes at a price, which is you have a very hard time 
mixing things in very arbituary ways. essentailly, not very flexible. We have to predeclare all the different 
kinds of mixes that we want. 


46:47
so the entity system architecture that Casey wants to try is exactly this 

                struct entity
                {
                    int healthMax;
                    int x;
                    int y;
                    int z;

                    ...
                    ...

                };
[this is just dominations game1 architecture. We have tons of memory for references pointing at null];

so our struct is gonna be epiclly huge. it could be a 64k entity struct.
what we are gonna do is that we know that we cant have every entity in the game taking up 64k all the time,
furthermore, we dont want to process 64k of entity all the time, but we already know that the structure of our game 
invovles sim regions, we simulate a region, where we load everything up, we simulate all of them, and we put them back. 


so our Casey_s idea is that when we go start simulating, we call a decompress and we do a compress into our storage 

_____________________________________________________________
                            |
    struct entity           |          storage (world Chunk);
                            |   
                       <----|------
                         decompress
                            |
                            |
                            |
                        ----|--->
                         compress
                            |


so we got entities stored in world chunk.

49:25
what Casey mean by decompress and compress is that he doenst mean LZ compression, 


so imagine our struct entity has 100 properties 

                struct entity
                {
                    int healthMax;
                    int x;
                    int y;
                    int z;

                };

What Casey wants to do is to mark off the ones we decompress

                struct entity
                {
                -   int healthMax;
                    int x;
                -   int y;
                    int z;

                };


and when we simulate the entity, we mark the ones we added, and de-mark the ones we removed.
and when we compress the only ones that are there.


50:18
So Casey says, why do this? 

the reason is because, all of the simulation code that is gonna access struct entity gets written 
in straight C style, where it knows the offsets of everything, it can be written at full speed,
it doesnt have to do any look ups, and it has free rein to do anything it wants.

any peice of simulation code can pull. it doesnt have to go ask "does it have l proeprty" or "does it have z property"
and so on. 

it can just ask directly and grinding out what it is 


this way we have no limitations what an entity can be. 


53:42
Casey does say that he doest know if this will work because hes never tried it before. 


Q/A
55:58
Casey talking about the dynamic dispatch

what dispach is like saying: I want a way to associate a function with a memory location, that is what dispatch is.

normally what happens is the other way around, I call a function and i pass it some data.

this is the case, I have some data and I want you to tell me some function. The typical case is 

lets say I have struct foo and struct bar 

                struct foo                  struct bar 
                {                           {

                };                          };

                                
                DoFoo(foo* F)               DoBar(bar* B)
                {                           {

                }                           }



so maybe sometimes I just want to call a family of function, lets say the DoFoo(); family of function
and that is determined by the memory location 

so I can just store a function poitner in side foo 

                struct foo
                {
                    function* Do;
                };

and when I call it it just call the function and pass himself 

                F->Do(F);


what this allow me to do is to associate a function and a particular block of memory. 
Casey does mention that this is not very common in real code. 


goes on a discussion about how bad C++ dynamic dispatch is 

Casey mentioned that he doesnt know anyone who uses the C++ dynamic dispatch. He recommends it 
doing it manully.


1:07:28
Casey mentioned that in our system, we wont use dynamic system at all. 
the idea in our game is that we dont want to define what an entity is in our game. 


1:23:54
would the Entity system include particle effects?

No, particle effects are a graphical effect that typically get launched. An entity will spawn particle effects,
but wont spawn particles as entities. Because particles are very specialized and typically designed for high throughput, 
they dont have mutable properties. Like fire particles typically dont change to water particles. 