Handmade Hero Day 036 - Loading BMPs

26:50
1920 x 1080 is currently a standard HD resolution for games nowadays. typically if you want to run a game in HD
1080P is what you want to go for

1920 x 1080 is usually achieve with the GPU though. without GPU, like this game, we will reduce it by quarter 
(width by half an height by half)

do note though this video was made in 2015


28:20
the reason I made a bitmap that size, atleast in GPU land, a lot of times you want the GPU to be power of 2 in size

he goes on to describe how to fit 1920 x 1080 into powers of twos


it seems like you would usually render a larger background for screen shake. You would want extra margins



34:50
do note though this is not stuff you deal with in 3D grpahics. 3D graphics works differently


35:20
TL:DR, the texture size is a bit larger than screen size is cuz we are trying to fit powers of 2 for the screen size



39:25
reading file formats

bmp file format



44:14
actually exmaining the bytes of a bmp file


he does a pretty cool command in the visaul studio debugger window

turns out it is a format-specifier
https://docs.microsoft.com/en-us/visualstudio/debugger/format-specifiers-in-cpp


the command he used was
				(uint8 *)(ReadResult).Contents, 64	

if you scroll to the middle part(size specifiers for pointers as arrays) of the link above, 64 indicates the number of array elements
that you want to view

so here it means, we want to view 64 elements in this array 



50:43
beaware that when you read in bytes information, C++ is under no obligation to pack bytes tightly
it will line bytes in 4 bytes boundaries

so when we say there is uint16 and uint32

				struct bitmap_header
				{
				    uint16 FileType;
				    uint32 FileSize;
				    uint16 Reserved1;
				    uint16 Reserved2;
				    uint32 BitmapOffset;
				    uint32 Size;
				    int32 Width;
				    int32 Height;
				    uint16 Planes;
				    uint16 BitsPerPixel;
				};

when it finishes with uint16 FileType, it will actually skip two bytes, to write the uint32 FileSize
so the solution is to actually tell C to pack the structure tightly


pragma pack and push
https://msdn.microsoft.com/en-us/library/2e70t5y1.aspx


so if you call #pragma pack(1), it will pack your struct one byte aligned



52:37
so once we are done, we have to set the packing mode back to the default pack mode or what it was before. and a lot of times we do not 
know what it is. Is it pack(1)? or pack(2)? or pack(4)? there's no way to find out

therefore what the design they have is just to do a pack and pop 
				

				#pragma pack(push, 1)
				struct bitmap_header
				{
				    uint16 FileType;
				    uint32 FileSize;
				    uint16 Reserved1;
				    uint16 Reserved2;
				    uint32 BitmapOffset;
				    uint32 Size;
				    int32 Width;
				    int32 Height;
				    uint16 Planes;
				    uint16 BitsPerPixel;
				};
				#pragma pack(pop)



1:01:01
do you have to handle endianness when you read the bit maps?

i dont know if they can be a different endianness if they are saved on a mac
but we know that it will always be in its native endianness. nowadays mac also use little endianness


1:01:58
someone mentions "pack push pop vs GCC annotation"





1:11:15
why not write parsers for all primitive types when reading the file?

Example

				uint16 Read_uint16(parser* Parser)
				{
					// advances the Parser as it reads 16 bit
				}

				uint32 Read_uint32(parser* Parser)
				{
					// advances the Parser as it reads 32 bit					
				}

				uint64 Read_uint64(parser* Parser)
				{
					// advances the Parser as it reads 64 bit
				}

				void ReadBMP()
				{

					parser Parser = {beginning of file};

					uint16 FileType = Read_uint16(parser);
					uint32 FileType = Read_uint32(parser);
					uint64 FileType = Read_uint64(parser);
					...
					...
				}


there are times that this method is a useful thing.
but the reason why He doens't think it's useful here is becuz, it is just way more work
in our current method. it is a straight up cast
				


			    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);    
			    if(ReadResult.ContentsSize != 0)
			    {
			        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
			        uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
			        Result = Pixels;
			    }

we are casting byte array to bitmap_header*. it is a straight up cast. it is the most trivial thing that the compiler
has to handle.

the separate primitive way is just way to much work without gaining much.

There are times you would want to write this:
For example that if you do have endianness issue, and endianness things has to be reversed. Then you can bake the endianness reversal 
into the read.

Another time is predicated files. 
meaning for example 

				struct bitmap_header
				{
				    uint16 FileType;
				    uint32 FileSize;
				    uint16 Reserved1;
				    uint16 Reserved2;
				    uint32 BitmapOffset;
				    uint32 Size;
				    int32 Width;
				    int32 Height;
				    uint16 Planes;
				    uint16 BitsPerPixel;
				};

let say the existance of Reserved 2 depends the value of Reserved1. 
like if Reserved1 is 1 then Reserved2 exisits. while if Reserved1 is 0 then Reserved2 is gone.

then that means this pre planned layout does not work anymore,
then the primitive method would make sense.




1:15:00
how would we handle memory when reading files? would it be wasteful to read it into our memory pool, then use part of it?

we will go deeper later. But in short, we will be doing resource streaming.  which means what will be doing instead of using DEBUGLoadingBMP calls is that. we will be reading in stuff we have preprossed into a format that is gonna be real easy for us to read in to stream in as fast as possible.

we will be focusing on how to make that pipeline.

Meaning the data we directly read from disk, probably wont be useful directly. more on that later.




1:21:50

I hate templates cuz their syntax is bad, and their implementation in the debugger and the compiler is bad.

if you use templates, error message the debug visibility are bad. you are bascially introducing complexity for little gain

lets examine what has to happen. 

they are easy to read, you can see which one they are calling. 





1:28:06

metaprogramming:

use C code to generate C code


