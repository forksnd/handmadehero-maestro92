Handmade Hero Day 234 - Implementing Radix Sort

Summary:
implemented radix sort

Keyword:
Radix Sort


10:59
so assume we have items a b c d e f g 
and each of these items have a 32 unsigned integer as the sort key
for our sort key, we put it into granularity of 8 bits or 1 byte.


so assume that g has the sort key 0x4ef1ba95

         _______________________________________
        | 24 - 31 | 16 - 23 |  8 - 31 |  0 - 7  |
        |_________|_________|_________|_________|
    g   |   4e    |   f1    |   ba    |   95    |   
        |_________|_________|_________|_________|
    a   |         |         |         |         |
        |_________|_________|_________|_________|
    c   |         |         |         |         |
        |_________|_________|_________|_________|
    b   |         |         |         |         |
        |_________|_________|_________|_________|
    e   |         |         |         |         |
        |_________|_________|_________|_________|
    f   |         |         |         |         |
        |_________|_________|_________|_________|
    d   |         |         |         |         |
        |_________|_________|_________|_________|

you would think that we would want to start from the most significant byte, but it actually doesnt matter. you can 
go from least significant byte to most significant byte or the other way around. doesnt really matter. it will produce the same result



21:40
so the followup question is why do we want to chop up our sort key?
why are we making our 32 bit sort key, sort 4 different times?

the reason is becuz once you chop down the total number of possible values for the sort key,

for example in our case, we chop it down to 8 bits of granularity

8 bits means 256 possible values. On modern day computers, its trivial to do somoething like 
                
                int count[256];

so what that means, if we can shrink your comparison problem down to something small, 
in which we can then use a table logic to reduce the actual work that you have to do. 


[if you go on youtube or wiki, you will find that they would refer this table as buckets.
so essentially, it will allow us to assign the items into manageble buckets]


So for the algorithm:

For each iteration, we do two passes on the elements + a pass on the buckets
1st pass: we go through all the sort keys as we are only looking at 1 byte at a time, 
(there is only 256 possible values); and then re increment the count value to tell us how many of that sort key do we have 

example:
if we have 

         _______________________________________
        | 24 - 31 | 16 - 23 |  8 - 31 |  0 - 7  |
        |_________|_________|_________|_________|
    g   |         |         |         |   10    |   
        |_________|_________|_________|_________|
    a   |         |         |         |   11    |
        |_________|_________|_________|_________|
    c   |         |         |         |   11    |
        |_________|_________|_________|_________|
    b   |         |         |         |   30    |
        |_________|_________|_________|_________|
    e   |         |         |         |   45    |
        |_________|_________|_________|_________|
    f   |         |         |         |   128   |
        |_________|_________|_________|_________|
    d   |         |         |         |   56    |
        |_________|_________|_________|_________|


then in the first pass, we would have 

        count[10] = 1
        count[11] = 2
        count[30] = 1
        count[45] = 1
        count[56] = 1
        count[128] = 1


then this is where we do the extra pass of going through all the buckets
we go through all the buckets and calcualte the accmulative starting offsets for each bucket 

so the buckets become

        count[10] = 0
        count[11] = 1
        count[30] = 3
        count[45] = 4
        count[56] = 5
        count[128] = 6

essentially, all the numbers that fell into the bucket of count[10], their starting index offset is 0 
(that is the index into our partially sorted array);

starting index offset for all numbers in the bucket of count[11] is 1
starting index offset for all numbers in the bucket of count[30] is 3
...
...


then the 2nd pass to go through all the items again and put them at the right place.

so when we get to g, 
when we get to a, which has the high byte of 11, it will b placed at 1 cuz that is the starting index for numbers in that bucket

    _ g _ _ _ _ _  



so the run time is O(2N);


then if we do four 8-bit passes, we will have our items sorted. then our total runtime is 
O(4 * 2n); = O(8n);




27:09
so now Casey goes on to implement it. The tricky thing for us is that we dont have an integer sort key,
we have a floating point sort key 
               

-   we define the table of 256
                
                u32 SortKeyOffsets[256] = {};




-   full code below:

                Handmade_render_group.cpp

                internal void RadixSort(u32 Count, tile_sort_entry *First, tile_sort_entry *Temp)
                {
                    tile_sort_entry *Source = First;
                    tile_sort_entry *Dest = Temp;
                    for(u32 ByteIndex = 0; ByteIndex < 32; ByteIndex += 8)
                    {
                        u32 SortKeyOffsets[256] = {};

                        // NOTE(casey): First pass - count how many of each key
                        for(u32 Index = 0; Index < Count; ++Index)
                        {
                            u32 RadixValue = SortKeyToU32(Source[Index].SortKey);
                            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;
                            ++SortKeyOffsets[RadixPiece];
                        }

                        // NOTE(casey): Change counts to offsets
                        u32 Total = 0;
                        for(u32 SortKeyIndex = 0; SortKeyIndex < ArrayCount(SortKeyOffsets); ++SortKeyIndex)
                        {
                            u32 Count = SortKeyOffsets[SortKeyIndex];
                            SortKeyOffsets[SortKeyIndex] = Total;
                            Total += Count;
                        }

                        // NOTE(casey): Second pass - place elements into the right location
                        for(u32 Index = 0; Index < Count; ++Index)
                        {
                            u32 RadixValue = SortKeyToU32(Source[Index].SortKey);
                            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;
                            Dest[SortKeyOffsets[RadixPiece]++] = Source[Index];
                        }

                        tile_sort_entry *SwapTemp = Dest;
                        Dest = Source;
                        Source = SwapTemp;
                    }
                }

41:48
so we wrote a function that would transform our sort key to a u32 SortKeyToU32();
the sort key we pass in is essentially the z value, so its just a float.

apparently using the float in the radix is not a problem 
     _______________________________________________________
    |   sign    |      exponent    |    mantissa            |
    |   1 bit   |      8 bits      |    23 bits             |
    |___________|__________________|________________________|

the only caveat is the sign bit, becuz for negative numbers, the sign bit gets a 1 
for positive numbers, the sign bit is 0

for the most part, we dont have a problem with sorting. 

this is becuz the exponent is the most important part, and if we just compare the bits on the exponent 
it will sort properly.

Refer to my "floating point" Google doc.

essentially the exponent doesnt use 2s complement to represent positive or negative numbers,
it uses biased notation, meaning it applies a bias of 127. As a matter of fact, they designed the floating point
number exponent field this way becuz this will make direct exponent comparison easier.

and therefore, we can just do direct comparisons on the exponent field 

the same works for mantissa. for the mantissa, if it is larger, it will sort properly. so we can do direct comparisonss

so only the sign bit is trick to deal with


-   notice the first line 
                
                u32 Result = *(u32 *)&SortKey;

    we arent casting the float into a u32, we are strickly copying the bits into a u32

-   first thing we do is we check if the sign bit is set. if so (meaning, we were originally negative);, we flip our bits

    0x80000000 is just 0x80 00 00 00, which is 10000000 00000000 00000000 00000000

    ~ makes all bits in a number are flipped


    then if the sign bit isnt set (meaning we were originally positive);
    we just set the sign bit on.

    essentially we are trying to make the negative numbers small, positive numbers big.



-   full code below:

                Handmade_render_group.cpp

                inline u32 SortKeyToU32(r32 SortKey)
                {
                    // NOTE(casey): We need to turn our 32-bit floating point value
                    // into some strictly ascending 32-bit unsigned integer value
                    u32 Result = *(u32 *)&SortKey;
                    if(Result & 0x80000000)
                    {
                        Result = ~Result;
                    }
                    else
                    {
                        Result |= 0x80000000;
                    }
                    return(Result);
                }
