Handmade Hero Day 232 - Examples of Sorting Algorithms

Summary:
summarized some of learnings from a video that someone posted in the handmade hero chat from day 231
explained some of the concept of P and NP topic

mentioned how constant terms effects algorithm performance 

mentioned the different types of sorting 

mentioned why would you want and not want to use radix sort.

mentioned what is sort stability in the Q/A

Keyword:
P and NP, sorting algorithm

0:41
mentioned that the link offered by a guy in day 231 chat
gives a pretty good explanation about P and NP stuff
https://www.youtube.com/watch?v=moPtwq_cVH8


Casey briefly mentioned what he got out of that video

assume we have an axis describing how  "difficult" a problem is 
"difficult" is not the same as "complexity". this is just a general sense of how difficult a problem is 


you can image all the P problems fall under the first section
NP problems fall under the first two sections
and so on and so forth 


    |--------------|-------------|---------------|-------------|--------------|------->

    |------ P -----|

    |------------ NP ------------|

    |----------------------- exp ----------------|

    |----------------------------- r --------------------------|


the thing to point out here is that NP is a super set of P, not an exclusive set that is different from P


5:58
also mentioned in the video, it clarified that "NP-Complete" and "NP-Hard"

so "hard" means >=. (note that its greater or equal to NP);
so when people say "NP-hard" that includes the the hardes problem in he set of NP

                             NP-Hard Point 
                                 |
                                 |
                                 V

    |--------------|-------------|---------------|-------------|--------------|------->

    |------ P -----|

    |------------ NP ------------|


also the video explained what "NP-Complete" means.

it doesnt mean that someone is in NP. what it means is that they have actually proved that a problem is NP and "NP-Hard"
so its on that NP-Hard point. its harder than any other problem in NP, and it is in NP.


9:55
the video also mentioned whats outside. exp is exponential time such as 2^n

r means finite time. it just means it finishes, it wont take infinite amount of time

there are problems outside of r, such as the "halting problem"
https://en.wikipedia.org/wiki/Halting_problem



26:52
Casey mentioned something very important about order notations, which is 

"scaling only matters if you actually scale"

the order notations only matters when you have a lot of items. so this thing is good at describing the performance
when you have tons of things.
its not really good at describing when you dont have lots of things 

unfortunately, "lots of things" can differ depending on the algorithm
the reason is becuz there is a constant term.


30:00
so Casey went inside the game and wanted to collect some data on how many render elements we will be sorting.
and for now, the highest we ever gotten is 160

160^2 is 25600

so if we are comparing two Algorithms, one is N^2, the other linear, and our n is 160

    A =     c0 * N^2 vs     c0 * 25600
    B =     c1 * N          c1 * 160


c1 has to be 160 times slower than c0 for us to favor N^2

what operation is in the maginute of 160 times slower? 
a cache miss.

so if B is extremely not cache-friendly, u would want to favor A



33:04
Casey also mentioned quicksort

remember when we are talking about O(N^2); vs O(N); we are referring to worst case.

so we dont meant this algorithm will take O(N^2); time, we are saying it might take O(N^2); time
it might take O(N); it might take O(1);, we dont know for sure. we just know that it is guaranteed that it wont take 
more than O(N^2); time 


so the the worst case for quicksort is O(N^2); 

mergesort is O(n * log2(N));

so you would think that merge sort is faster, but the C runtime library chooses quicksort, this is becuz most of the time 
acutally doesnt go N^2. Most of the time it goes n * log2(n);

and it turns out quicksort is faster than mergesort 




37:40
Casey mentioned some other sorting algorithm

bubble sort,

merge sort

quick sort  ----> this is the one in the C runtime library
    so if you look at the function qsort
    http://www.cplusplus.com/reference/cstdlib/qsort/

    Casey thinks quicksort is a pretty weird algorithm, cuz to him, it feels like its a pretty randomized algorithm.



radix sort:

people often say that radix sort is an O(1); sort, but that isnt really true.
its actually O(kn);

k is referring to the number of digits of your sort key 
(digits could be bits or could be anything, its any granularity you have in your sort key);

for instance, lets say you are sorting by an 32-bit integer. then k is 32 
n is the number of elements

it works by peeling. 

take the example of a 4 bit sort key

    _ _ _ _ 


if we look at the first bit, we can then split items into groups that have the first bit set,
and ones that dont have the first bit set 

so you get two groups of 

    0 _ _ _ and 1 _ _ _ 

then you do the same for the 2nd bit. 

    0 0 _ _, 0 1 _ _ , 1 0 _ _ and 1 1 _ _ 

and then you repeat for all the bits, then you will have all your items sorted.

so this algorithm is kind of up to the implementer on how he wants to implement the granularity of the sort key, 
essentially the k variable.

this algorithm does require space, so its not something you can implement in place 



insertion sort:
not that useful. 


Q/A
1:01:35
why wouldnt people always use radix sort?

so people have proven that for sorting, O(N log2(N)); is proven to be the lower bound. Nothing can be lower than this.

so you just do a math comparison 

        O(n log N);
        O(k n);

the other thing is that, it may be impossible to make the sort key.
sometimes when you sort things, it may not be possible to represent it as a quantity.
for exmaple, when you want to sort based on a string  

                struct entity
                {
                    char* name;
                };

the radix sort need the sort key to be fixed length. 

also for other types, you can put complicated logic to sort 
lets say you have 
        
                "dave bower"
                "carl warlington"
                "big tuna"


and if the first name is carl, you want it to be behind dave, otherwise we sort it alphabetically. 
if you want to do that radix sort, you would have to preprocess to somehow make it work



1:12:30
what is sort stability?

if we were to take some input in our algorithm, and we have duplicates 

    a b c a d e f 

we want to output 

    a a b c d e f
    

will it be detemrinistic? Essentially sort stability refers to sort determinism.

