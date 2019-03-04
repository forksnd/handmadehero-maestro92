Handmade Hero Chat 001


10:35
someone ask how does emulation work?

1.  CPUs today arent much faster at straight line code 
2.  Emulation requires emulation
    emulation is not the same thing as running code 

    emulation means that a particular CPU that you are talking about, must be recreated, accurately enough
    that the program running on it cannot tell the difference between the emulated enviornment and the actual hardware. 

when Casey means "tell the difference", Casey does not mean that the code is actively looking for differences 

24:33
so what has to happen is that you have to build 
a virtual CPU model, 
a virtual GPU model, 
a virtual motherboard model 

and you have to simulate all of their defects, specified weirdness, all of everything.
to accurately recreate the emulated enviornment, and you have to run all these virtual CPUs. its not simply running the code. 
so who knows how complicated that could be.  


51:17
const optimizations 
most of the time when you are in a function, and you get something passed in as a const, the compiler cant assume that its actually 
const, becuz anyone could actually modify it, becuz in C, the spec allows you to cast the const away. 

see specs for const_cast
https://en.cppreference.com/w/cpp/language/const_cast

so the only time that it actually helps u with optimization is when the compiler can see that you, yourself, have made the thing 
that is const. It has to be made right there. It can be passed in, cant be a pointer, 

you made the thing yourself. its an actual value, and the compiler couldnt have figured it out that it doesnt get modified anywhere else.

so when you pass this address back to somewhere else, and someone else can use it.

that is the only time that the compiler can use const for optimization purposes.
anyother time, it really doesnt help you very much, or help the compiler. 

const as a concept doesnt even exist in the optimizer for LLVMs. The only thing that will happen is that he front end 
will do some transforms if it knows about const in that specific case mentioned above. 

