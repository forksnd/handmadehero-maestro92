Handmade Hero Day 015 - Platform-independent Debug File I/O

1:40
File I/O typically breaks down into two distinct categories:
1.	loading read-only assets from something: music, 3D models, models, textures etc

Gettings stuff off of drive, loading to memory. You are never touching those ffiles other than the "Read" function

you are never writing to them, you are putting stuff out to the disk, you are only pulling them from the disk

by disk, this could be: network, optical drive, hardrive or ssd drive


2.	saving the state of the game
where the game reads and writes:

stuff like the "configuration for the game", or "player's progress in the game"


So for Game IO, thats pretty much two classes
one just reading
the other both reading and writing

they also have different criteria for success. 

saved game things are typically tiny. usually returns in one call, does not have to be streamed.
whereas, read only assets, you typically stream that stuff in the background, so the user is not sitting on a loading screen any longer than they have to

they try to overlap that stuff with other stuff to keep the game running

with assets you are reading gigabytes worth of data, and typcially, this is not instantaenous. So this is not something that can be ignored in the performance criteria, becuz its happening all the time, and happening on a large scale

so for the read-only asset thing, theres a lot more hard-core engineering in there


6:49
how file IOs are used to be done vs how file IOs are done nowadays


9:25
					
					if(Read(File, sizeof(Buffer), Buffer))
					{
						// use Buffer
						uint8 Buffer2(256)
						if(Read(File, sizeof(Buffer2), Buffer2))
						{

						}
						else
						{

						}
					}
					else
					{

					}


this old way is bascially a streaming API. 
at anytime, the harddrive could fail, the file could get locked, the file could get deleted, the file could flat out pull out the USB drive
basically you are introducing a bunch of failure cases: every read is a failure case that you need to have some strategy for handling

we also have to worry about the threading saftey.

The file handles are very stateful. They store a location of where you are reading in the file. So if multiple people reading this file, they need multiple locations for this file.

so that means each thread needs a file handle


lastly this streaming API is totally synchronous. The code after the read cant start after the read has completed

Definitely, the hard drive, even if it is a SSD drive is the slowlest piece of memory on your machine. It is much slower than the speed that the CPU is executing.
So pretty much it is stalling the program at where the read is happening. especially if you are doing two reads in a row, then we are really stalling the program

Pretty much there is nothing good about streaming based file I/O




15:20
we are gonna treat it like a memory allocation. It is like we are allocating this file in memory.

in the final version, we will be writing this to a queue, and we will process it asynchronously

					
					uint64 fileSize = GetFileSize(filename);
					void* bitmapMemory = ReserveMemory(memory, fileSize);
					readEntireFileIntoMemory(filename, bitmapMemory);

with this structure, the OS does not have to do more allocation than it needs to. It just uses part of the memory you already allocated from "memory"


we can also use file mapping



22:20
we are making internal, just to help the compiler out, so that it knows there are no external linkage



23:59
Operating system like handles. The way OS tend to work, there are resources that are on the Operating Systems side, sockets, files, streams.... they need a way of talking about these resources to you, and they use a handle for that

They are typically indices to a table, or a poiner into kernel memory that says here is the thing that Im talking about on  your side.




44:19
__FILE__ is a C preprocessor macro that inserts the name of the file that got compiled


46:10
again, this is only for debugging, you would not want to virtual alloc every file you read

virtual alloc is for large allocations. virtual alloc minimum allocation size is 64k (or something like that). it allocates in 4k pages, but u have to allocate atleast 64k. 

long story short, u dont want to be doing lots of small VirtualAllocs



54:37
the file will only say its true size when it closes its handle. You will notice the free bytes actually went down by the amount that we wrote.


1:00:27
when you write to a critical file, you typically just dont want to overwrite the old file. And the reason for that is becuz your overwrite could fail, and it will only partially overwrite the file, leaving them the corrupted saved file.

So what you want to do usually, is to write to a different file, and either have a rolling buffer scheme, meaning you have an A and a B file, and you would write to the A file, then to the B file, then to the A file ... on alternating runs of the game. And you load whichever one that is most recent.

Or you write to a secondary temp file, delete the old file, and rename the file inplace


1:11:50
does the order in a struct amtter?

cuz of how c packing works

struct fake_struct
{
	uint8 x;
	uint32 y;
	uint8 z;
	uint8 u;
	uint8 v;
};


is more bloated then 

struct fake_struct
{
	uint8 x;
	uint8 z;
	uint8 u;
	uint8 v;
	uint32 y;
};

if u dont care about memroy, u should certainly care about performance, cuz
reading all that extra memroy and all that cache polution is gonna cost you



1:23:21
xor swap

