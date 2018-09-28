Handmade Hero Day 076 - Entity Heights and Collision Detection

Summary:
started to check physics collision in the z axis (previously only x and y axis)

Architecture design of entity heights

talked about how to debug heap corruption bugs


Keyword: 
physics, debugging memory





11:55
in the entity physics collision code, we start to check z axis. Previously, we only considered whether we are 
colliding on the x and y axis

Lots of Architecture design of entity heights in the game



1:08:42
any tips on tracking down heap corruption bugs?

so we wrote these memory allocation functions such as

				handmade.h

				#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
				#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))

				inline void *
				PushSize_(memory_arena *Arena, memory_index Size)
				{
				    Assert((Arena->Used + Size) <= Arena->Size);
				    void *Result = Arena->Base + Arena->Used;
				    Arena->Used += Size;
				    
				    return(Result);
				}

Casey typically keeps two versions of these functions around: shipped version and debug version
the shipped version is what you see here 

the debug version will actually do a legit allocation thru the OS

it asks for a block of memory, with a special debug allocation routine.
usually when you call virtual alloc, (assume in 4k pages), you use right from the first byte offset from the page
and you use however much you are going to use

What Casey does is he usually allocate a bigger page than the amount that he needs to use 

for example if he needs 6k memory, you get 3 pages instead of 2 pages 



						page1			page2			page3
					|				|				|				|

					0k				4k				8k				12k

							^						^
							start					end


then he page aligns the 6k start at the 2k mark of page1, so that memory 2k ~ 8k is the region for his 6k

then he sets up VirtualProtect on his page3, then he set it to be not accessible, read or write

that way, if anyone ever somehow touches page 3 (which they are not supposed to), you get an immediate page fault


and you can do the same trick to check underflow, that we check if we creep into page1 memory.



						page1			page2			page3
					|				|				|				|

					0k				4k				8k				12k

									^						^
									start					end





this gets a bit more complicated if you have alignment issues. If you are aligning your allocations to 16 byte boundaries, which is common in 
SIMD operations, someone can technically write 15 bytes.

For example 
if you just allocate just one byte. 

and you allocate one byte again, the pointer you will get starts from the 16 byte mark

and if you allocate just one byte, you should only have access to the 0th byte. 
That means from byte1 to byte16, that should be clean.

						
					|				|				|				|

					0byte			16byte			32byte			



