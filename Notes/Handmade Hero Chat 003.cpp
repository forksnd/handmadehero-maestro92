Handmade Hero Chat 003


1:23

why would a game need more than one file (actual question asked at 8:02);

so first Casey brought up a concept: conceptual resource vs physical resource

conceptual resource is something you think and talk about as a asset or something you use as part of a game.

for example: a 3D mesh, that you made in maya 

            in handmade hero, a bitmap is a conceptual resource 

so there is nothing specific about them, at the end, they are just bits and bytes of data. 
but as a bundle, we are thinking of them as a bitmap. So this is a conceptual resource. 

a conceptual resource has literally no relationship with a physical resource. 



a physical resource is something you actually have to get, store, load. something that is actually binary data
coming from somewhere in some form of package. a physical resource might literally be a file on a drive or a file that is on a network somewhere 
or a url addressable thing.

for example, we may have something a xxx.txt file, this is a physical asset in the sense that it is on the drive, as a file. 

the reason why Casey wants to emphasize the distinction is becuz this text file might have many conceptual resources in it. 
for example, this txt file might have different section, and each section might be conceptuall different. 

More to the point, we may have something like Handmade.art, or even the asset file we are currently using test.hha, test2.hha and so on. 
these are physical resource, its a file on the drive. However, its not a conceptual resource, becuz in it, its a bunch of conceptual resources. 

there is tons of tons of bitmap, tons of tons of sounds.

so this is a very important distinction. 

so back to our question that the dude asked:
1.  why would a game need more than one file? 
2.  the only thing I can think of is that if you want to have different sprite sheets that you want to swap 
in and out for a character, then you might want to have more than one file?
3.  are there api / file size limits in this decision?


so Casey says:
1.  so seprate from question 2 and 3, just a game by it self, is there a reason to have more than one file?
    the answer is: potentially yes. the reason is becuz expansion/patching/modding/etc.

    concetually speaking, you may want to break things up into more than one file, becuz those files are actually things that a user 
    may want to address in someway. meaning a user may want to mod a particular thing and distribute that mod, and that may be easiest 
    to do by allowing multiple files in the directory. 

    so iterating a number of files, and then merging their results, is a way of providing interface to the user, for editing the game. 
    furthermore, it provides a convenient interface for something like an expansion, where you can ship an installer, where the installer 
    is something that copies an new .hha file into the game directory.

    then if the game is smart enough to just read all the .hha file in that directory (pretty much our current set up 
        in handmade hero);, then you dont have to do antyhing

    that way your expansion pack shipment is literray that one extra asset file     

2.  i dont really understand this question. Even having one single physical asset files, you can still load your conceptual resource in parts
    so its no different then loading separate physical resource

3.  Yes. certain file system impose a file system limit 
    another limitation is donwload. if people have to download, you might to make it small so people can check your progress in your downloads 

    are there api limitation?
    Yes and no 

    "memory mapped file". a memory mapped file is when you basically tell the OS, look i got a memory range, that I want to synchornize 
    with the content of a file on disc

    Memory
     ___________
    |           |            ___________
    |___________|           |           |
    |           |           | File on   |
    |           | ------->  | disc      |
    |___________|           |           |
    |           |           |___________|
    |           |           
    |___________|

    on a side note, Casey says he doesnt think memory mapped files are a good idea.
    memory mapped files is a way of synching some bytes in memory to some bytes on the drive. 
    and if you only have a 32 bit address space on your memory, that means you can only have a 32 bit sized file. 

    that means it puts a 4GB limit on the api


31:55
why do you think memory mapped files are a bad idea?

becuz you have little control regarding how are they overalapped and when are they issued in terms of when they are read from any 
evicted parts of the file. So I just dont think its a good performacne oriented programming practice to memory map a file. 


