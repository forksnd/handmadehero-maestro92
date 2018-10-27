Handmade Hero Day 112 - A Mental Model of CPU Performance

Summary:
STARTING optimizations!!!!!!!!

talked about how CPU + GPU works 

talked about why are modern processors SIMD

mentioned what are cycles in the CPU specs are 

suggested we should always know the base amount of performacne you can work with per frame 

looked at pipelining, memory access and all that stuff 

mentioned prefetching, hyperthreading 

Keyword:
CPU Processors, SIMD, Cache, Memory



2:03 
first step of optimizations is to measure your performance

mentioned that he has limited knowledge on optimizations 
there is a certain level in which he wont be able to guide you 
for example crazy things such as cache line aliasing issues


4:09
there is a CPU + GPU in your computer, that takes instructions and do actions  

	instructions 
	 _______________
	|				| 	
	|				|
	|				|
	|				|
	|				|
	|				|
	|				|
	|_______________|



instructions fall into two broad categories 


1.	loads/stores, things that modify memory
	
	eithering grabbing stuff from memory and loading them to registers or cache

	or 

	we got values that we write to memory 


2.	ALU instructions or math instructions


	the math instructions tends to be wide, or SIMD, single instructions multiple data

	we have one instructions but it operates on multiple data at once 
	typically in the minimal case, we operate on 4 things 
		 _________________________________
		|___0___|___1___|___2____|___3____|

	so when you write a single instruction like x = y + 3, that is not what is happening in the processor, 


	in modern days 
	what happens is the processor will do this operation four times at once 
	meaning you will do x = y + 3 four times, once in each of the slots 

	and it will only throw away 3 of them and keep only one

	the reason is becuz most processors are designed in the SIMD way
	3 is the minimum you will be throwing away 


	intel CPUS nowadays are going 8 or 16

		 ______________________________________________________________________________
		|___0___|___1___|___2____|___3____|___4____|___5____|___6____|___7____|___8____|


	in GPU, it could have as many as 64



8:11 
the reason why we do this is becuz it is not free for CPU and GPUs to decode instruction streams


12:56
so if you can write your code in the SIMD style, it saves a lot of bandwidth memory that is going to the processor

13:02 
mentioned I Cache, instruction cache

13:17
all processors works this way, SIMD is everywhere



14:03
this is the Model


			instructions  					cache                          memory
			 _______________   			  _______________              _______________
			|				|            |				 |            |				  |  	 	
			|				|            |				 |            |				  | 	
			|				|            |				 |            |				  | 	
	     	|				|            |				 |            |				  |		
			|				|            |				 |            |				  | 	
			|				|            |				 |            |			 	  | 	
			|				|            |				 |            |			 	  | 	
			|_______________|            |_______________|            |_______________|



16:08
cache is usually on the processor

L1, L2, L3

you can think of register as "L0" registers 



19:59
showed that his computer has 3.2 ghz,  
giga hertz is 9 zeros: 1000,000,000  

3.2 ghz is 3,200,000,000 cycles per second


21:55
if we are running 30 frames per second
that gives us 3,200,000,000 / 3 = 107,000,000 cycles per frame


22:17 
Casey says, we should always know what this number is before starting optimizations

107,000,000 is on 1 Core, 1 CPU

depending machine, they may have multiple cores. Each core will can give you 107,000,000 cycles 



23:36
casey mentioned 107,000,000 is a very low number for our game.

Think about how many pixels we gotta render


24:46
you wont actually get all 107,000,000 

for example, there are times where we call windows to display the frame 


26:33
what is a cycle?


so we got code in memory, and when instructions go from code memory to I cache 
it is actually a Load and Decode operation

then you have the processor fetching the instructions out of the I cache


                 Code in memory 				I cache                             processor  
                 _______________ 			 _______________                      _______________
                |               | 			|               | 	 	             |               |
                |               | 	load &	|               |  fetch             | INST 0        |
                |               |   decode  |   "microcode" | <--------------    |               |
                |               | --------> |               |                    | INST 1        |
                |               | 			|               |                    |               |
                |               | 			|               |                    | INST 2        |
                |               | 			|               |                    |               |
                |_______________| 			|_______________|                    |_______________|

and also processor instructions fetches instructions out of order 
for example if you have instructions A B C D E F G

if C depends on A B,
then the processor will first run A B D E, then it will remember to run C later on once A B are done.

Modern processors run instructiosn heavily out of order

then it goes into cs384 and talks about Pipelining things, pretty much washer and dryer things 


40:09 
mentions throughtput and Latency

Latency: the delay from being instantiated and being completed
Throughput: how many i can do 

Latency is the cycles you measure to do one instruction
if you want to think about scalablity, like hundred or thousands of cyclees,
then the number you want is throughput


take the example of washer and dryer vs washer dryer 2 in one system

	 ____       ____
    | W  |  +  | D  |
    |____|     |____|

this is 2 latency, 1 throughput system 


	 _______  
    | W + D | 
    |_______|  

this is a 2 latency, 2 throughput system



43:00 
when we talk about instructions, we will only care about throughput



44:29
bandwidth is the total amount of memory that we can move through the processor at a maximum speed
it is kind of like the bandwidth number

there is a component called the bus that takes memory from the motherboard and drives it into the caches 

and there are lines that goes from cache to registers

46:43
bandwidth is typically given as Gigab bytes per second Gb/s



51:11
hyper threading 
the processor keeping two states internally
lets say i have State 0 and State 1, its like have two processors
whenever it sees that one of the state will stall on a memory access,
it will switch to the other state and check to see if it can do some work

pretty much its another tech that intel has came up with to get around that memory access is slow



56:00
Efficiency
means not doing work. Essentially Algorithm. Algorithm is the study of not doing work. so that is Efficiency


1:02:43
how does prefetching work? Do we manually prefetch memory or that is inferred by the CPU depending how we access memmory?

It can actually be both, 


1:05:42
Casey says when he optimizes something, he always does two things:
1. at peak speed, how fast it should go 
2. figure out what my target speed is 





1:09:50
from cheapest to most expensive

		Register
		L1
		L2
		L3
		Memory
		Drive / Network

I issue drive read 2 seconds in advance




1:15:39
if you write stuff in a typical straight forwards C programming style 
your code tends to end up looking like optmized code 


