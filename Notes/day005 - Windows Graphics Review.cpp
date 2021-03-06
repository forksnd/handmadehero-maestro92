Handmade Hero Day 005 - Windows Graphics Review

4:00
CS_HREDRAW | CS_VREDRAW 

repaint whole window when you resize horizontally, 
repaint whole window when you resize vertically


5:37
typically, one of the things u want to avoid is passing pointers to things that are on the stack (if you can avoid it).
cuz the compiler now no longer knows what's going on

when the compiler sees you pass a copy of your object

				void function(MyObject myObject)
				{

				}

there a bunch of assumptions the compiler can make, it can assume no body else needs to touch that thing

it can assume it is a private copy that the function is gonna use, So there is no possibility for aliasing

aliasing: when I have two pointers pointing to the same place in memory

Example:

				uint8* A = somewhere in memory;
				uint8* B = somewhere in memory;

				int Y = *B;
				*A = 5;
				int x = *B;

obviously, Y and X should equal;
so we should assume the compiler should just load it once

unfortunately, we have not given the compiler enough information for him to know that A and B does not point to the same location

some might written the code 


				uint8* A = somewhere in memory;
				uint8* B = somewhere in memory;

				A = B

				int Y = *B;
				*A = 5;
				int x = *B;

that way Y will equal what was previously in B. that way x and Y do not equal

This is one type of aliasing, called pointer aliasing

So there are a lot of ways we can tell the compiler that there is no aliasing. one of the easiest ways to do it is to just not pass pointers



Then the question is, why is the compiler smart enough to figure out that "we do not have aliasing here"

the code transformations, that the optimizers gets a bunch bytecode or intermediate representation and.... u should defer the question to compiler experts


30:22
by default, we do not have any pages in our virtual memory space. so if we touch some address space that points to an invalid page. 

50:07
small demo on analyzing stack on visual studio

56:23
for security reasons, our stack will be at randomized locations. 

1:08:00
every address space for every process is different. A pointer does not uniquely identify memory in the system. It uniquely identify memory in one process





