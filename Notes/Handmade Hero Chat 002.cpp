Handmade Hero Chat 002


0:48
what are the pros and cons of baking assets like images, into your executable, rather shipping them separately and loading them dynamically.?

there is none. 

lets say you have your executable on your disc/harddrive


    EXE
     ___________
    |           |
    |           |
    |           |
    |           |
    |           |
    |           |
    |           |
    |___________|


so executables have their format (which I just learned about from the linker and loader bok);
there is actually two formats, one is 32 bit exe, one for 64 bit exe.

there is like different segments such as text, data, relocation table and etc etc 

so really what we are talking about here is, if we want an image, do we want to store it in another file 

       .exe              image file        
     ___________       ___________        
    |           |     |     A     |              
    |           |     |___________|              
    |           |     |           |              
    |           |     |     B     |           
    |           |     |___________|             
    |           |     |     C     |             
    |___________|     |___________|               


or just storing it in the executable 

        EXE
     ___________
    |           |
    |           |
    |           |
    |           |
    | _________ |
    ||  image  ||
    ||_________||
    |___________|


whats the actual difference between two things?

1.  if you put in the executable, the exectuable is already open, you dont have to get a new handle to it 
    there is no chance it could fail, there is no chance that you cant read from it. its sort of a little bit fault tolerant.
    
2.  might be slightly faster, in some cases, becuz the linker will pre insert the offset to the images into your code, 
    and so that saves you a little bit of work. But we are talking about a little little bit of work.

on the whole there isnt much of a difference. Casey doesnt consider that a particularly important thing 

there is one fringe benefit: the user cant screw up. if the user wants to move the executable to another folder, and all the data
is in the executable, they can just move the .exe file and that will still work. 


nowadays, users dont tend to do that anymore, they dont really know about executables anymore. They just run something through steam or 
downloading something through an app store 


4:30
lets talk about perhaps a more interesting issue.
do we want this or this 
the left side we have an image file, which as data for image A, data for image B all concatenated 

or separate image files 


       image      
     ___________             ___________
    |     A     |           |     A     |
    |___________|           |___________|
    |           |           
    |     B     |            ___________
    |___________|           |     B     |           
    |     C     |           |___________|
    |___________|         
                             ___________
                            |     C     |           
                            |___________|


this is the difference between packed files vs separate files 


for the first questions, whether having image data inside the executable or not, there just arent many compelling reasons for either case.

but there are some pros and cons between these two approaches. and they are worth nothing. 

a packed file requires a lot less negotiation with the OS to use. 
and that is actually somewhat important.

if you do the separate image files approach, everytime you want to 
load an image file from a separate image, i have to go through the OS, and it has to do a trememdous amount of work.

i have to pass the filename: "filename.png", and it has to go on and figure out what directory structure am I in, look for a match for this name,
find that match, open it up, generate a handle, insert a thing in the handle table, blah blah blah blah blah.

there is a tremendous amount of work that goes in here.

when you are talking about hundreds and thousands of files that has to get open. Thats just a lot of uncessary traffic back and forth to the OS,
a lot of uncessary work in the OS that really does not need to happen. 

it also limits the amount of option for overlapping operations. for example, something like an open call, might not be very easy to overlap in terms of 
multithreading. You might have to create a separate thread to baby sit the open call. There-s a whole bunch of extra stuff that happens there. 

so there is a pretty good argument for packed files. 
a Packed file is very nice, becuz you open it once, and all you ever have to issue to the OS from then on is just Offset reads, just like what 
we are doing in handmade hero. 


so having a small number of large packed files, is usually a better idea for most operating circumstances, than lots of separate files. 

so typically when you ship, you want to go with the large packed files approach. 

typcially its not to hard to patch these large packed files, if you want to patch them. you can design your packed file format to allow for easier 
patching, or you can do sort of stuff we did in handmade hero where it allows for reading in multiple packed file, and make rules about how you want 
certain asset files take precedence.

so you should be able to design your patch file so that patching isnt too hard. 




20:23
dont installer programs typically bake information about the .exe they are installing. is that how installers are typically done on the windows?

there are two ways you can approach this problem:

so you know you got the linker, and you are shoving a bunch of .o files to it 


                a.o     ----->

                b.o     ----->  Linker  ----> executable 

but it can also grab resource files 


                a.o     ----->

                b.o     ----->  Linker  ----> executable 

        resrouce file   ----->


it can grab stuff like images, or wathever and it can link them 
on windows it has the resource compiler, it introduces .res files and whatever 

so you can get data in there even if the data wasnt a .obj file 

and the linker output an executable that has the information, in microsoft_s format, with tables that says where it is 


the 2nd approach you can do is, which is once you have your executable, without the resource file, just with only .o files

                a.o     ----->

                b.o     ----->  Linker  ----> executable 

now you ask, is there anyway for you to put any data in there. 

the answer is yes, its super super super simple 

lets say you have your executable
                    
                    EXE
                 ___________
                |           |
                |           |
                |           |
                |           |
                |           |
                |___________|

all you have to do is to make a little program,
you can even use the c runtime library for this.

call fread on your executable, then call fwrite to a new file (you would name this new file something.exe);

then fread your data files, then call fwrite to append them to your something.exe

             
                something.exe
                 ___________
                |           |
                |           |
                |           |
                |           |
                |           |
                |___________|
                |  image1   |
                |___________|
                |  image2   |
                |___________|


until i have a giant executable, where the first part is my executable, and every thing that comes after it
are my data files.

and at the very last thing, i put a footer, the footer is just like the header, which is like a table, containing information 
about each images 

           
                something.exe
                 ___________
                |           |
                |           |
                |           |
                |           |
                |           |
                |___________|
                |  image1   |
                |___________|
                |  image2   |
                |___________|
                |  Footer   |
                |___________|


[kind of like the ELF file format, where you have the program header table as the second section and the section table at the bottom]

then you just need to add code in your something.exe, that opens itself to read the footer to use the assets 

you can totaly do this.
there is no reason to, bu you totaly can do this. 