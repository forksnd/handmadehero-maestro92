
Keyword: transient and permanent memory

13:50
you pretty just want the game to be a service to the platform, where the platform just says "i need the video frame, I need the audio".


15:10
if the game wants memory, then the game gets memory from the platform layer and a big chunk at start up, and the game is responsible for doing everything else

So we are gonna call virtual-alloc at start up, and the game is gonna be responsible for partitioning it for each of its sub system


17:55 

two space in my game_memory
space that is transient
space that is permanent, things that needs to persist



31:33
normally in windows, it does its address randomization stuff for security purposes. If we allocate off of heap, it will be whereever it was


32:35
4 gigs is good becuz it gets us up to full 64 bit address range



35:00
becuz the way C does things is that it can promote numbers up to higher bit depth if something in the expression required it
this is called "integral promotion"


44:32
compiling away asserts



49:00
in window x64, the first 8 terrabytes in address space are reserved for the application


1:01:42
the OS just comes in and takes care of things for you


1:02:56
I want to know if the game has hard bounds on its memory or memory usage 

1:04:30
permenant store is non-negotiable. 
transient store is not

The permenant store is what the game needs to run: entity states, where the player is, 

I want to differentiate between the render stack, the cached version of of everything, the sounds 
which is all stuff that can be recreated off disk again.

we can flush the transient store and leave the permenant store. this happens if we need to minimize our memory footprint



1:22:00
we will have a few different configurations where your game can run in, 1G, 2G, 4G, and that determines what art assets you wll get


1:25:17
modding support

You cant tell if you have allocated enough memory for someone elses mod?

The game needs to work in a certain footprint
If you want to support modding, you still have a footprint requirement

You can allow mods to change what the footprint is if you want: this mod requires 8 gigs instead of 4, and all we would 
respond to that

My prefererence:
Not allowing mods to change the memory footprint. Should still work in the same memory footprint


1:30:40
we are not shipping the internal build. We let windows gives us its randomized address in the external build


1:31:20
virtual memory vs physical memory
virtual address space is not fragmented. If we allocate contiguous virtual memory, it does not have to contiguous in physical memory. In fact, most likely, it will not be


VirtualAlloc will almost never fail, when we ask for the 4 gigabytes, it will give us virtual memory even if our physical memory does not have 4 gigabytes
