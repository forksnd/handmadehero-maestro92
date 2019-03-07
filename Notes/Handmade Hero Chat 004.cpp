Handmade Hero Chat 004

0:40
Can you explain cross compiling?



is when want to run the compiling + linking process for other operating systems. (on ones that you are not actually on);

so if you have a win32 machine, but you want to make it run on a linux machine.
if I was to setup a toolchian to build a linux executable on windows, that is cross compiling. 
Its simply the act of producing an executable on another OS. 

the way it works is incredibly, the process of compiling + linking doesnt involve the OS. Its all just reading and writing files.

and outputting opcode. at no time does it have to interface with the OS except for loading and saving files. (which we all know how to do,
you can even use the c runtime library for that);

so all of the code for the compilation, does not depend on the OS in anyway.


16:46
if you write your compiler code well, such as LLVM. you can just pick your target, one that generates x64 code, one that generates arm code ..
and it doesnt matter whether you are running on a x64 or arm processor, whatever you compiled LLVM for, has nothing to do with that code path.

Compiler its essentially a piece of code that knows how to write out files. These files contains machien code, that can run on certain OS. thats it.



20:04
so why do I still need linux to compile for linux?

you probably actually dont, but here are some reasons in pactice why you often need to be on a particular platform to compile for that platform. 

some important tool in the compile + linking process is not available on other OS. 
for example, not sure on linux if there is a windows linker. 

LLVM doesnt have a windows linker. 


for example, LLVM could very much output obj file, but it might not have a windows linker to do the linking process.

Also, nobody may have done the work, to setting up all the dependencies. So for example, in order to compile a program, often times you need all these 
system header files, all these .h file, all these library files. If you dont have all those, you need the compiler and linker to grab all them when they 
are required. So in order to build a linux executable on windows, you need to grab all these files and put them all on windows, and so that on windows, 
your compiler + linker can access them. 

Cross compiling, conceptually is incredibly simple, but as with many uncessarily complicated build processes, building for an OS is often 
more complicated than it needs to be, becuz there is all these libraries you need to link with, theres all these system header files, and so on. 
and you have to reproduce all that enviornment 