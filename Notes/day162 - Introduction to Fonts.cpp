Handmade Hero Day 162 - Introduction to Fonts

Summary:

talked about what fonts are and what fonts file contain
described the difference ascii and unicode
described how unicode worked and its pros and cons in modern days system 

described what kerning is, "mono spaced" vs "proportional spaced" fonts

descrbied potential approaches to render font in handmade hero

Keyword:
debug, Fonts




6:48
fonts is a way of describing how text looks on the screen.

you have also seen ttf files, 
"TTF"   -   True Type Font Files. 
Which is what we see on windows alot

also you may have seen 
"Adobe Type I"

"MetaFont"

there have been many fonts
these are all different ways to describe how text look like, and they are usually described in terms of outlines

7:52
for example, a typical font file might include vertices to describe the letter T 


            #-----------------------------------------#
            |                                         |
            |                                         |
            #---------------#           #-------------#
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            #-----------#

thats the first part of a font file, which descrbies some filled space.

also these description intends to be resolution independent. There is no notion of pixels here
This abstract space could be scaled as big or as little as you want it, and it can still be 
infinitly precise.

10:38
in short, they describe a relatively reesolution independent outline for the font that will allow the program 
to render and fill in 

10:51
the 2nd thing they describe a mapping from some "character set encoding" to these outline shapes 

what we mean by "character set encoding"

one example is 
ASCII / ANSI 

this is pretty much a character set encoding that allows you to do most roman derived languages in a standard way.
where we basically defined what all the characters are. 

we have mapped all the characters to some symbols 

for example, 'T' maps to 84

so in the font file, if it is using an ASCII encoding, then we knkow 


84 maps to 


            #-----------------------------------------#
            |                                         |
            |                                         |
            #---------------#           #-------------#
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            |           |
                            #-----------#



so the character set encoding is very important cuz it defines what character you can encode 
for example

the symbols for meat in chinese 肉, has no encoding in ascii, so you cant encode it.
so if you were to pick a font description that only allowed you to use ascii, then you wont be able to include the 肉 character in your font


14:03
what you could do instead is that, lets say for 

symbol 223, which maps to a weird square
http://www.asciitable.com/

we dont need that weird square, and we can swap it for the meat symbol, 肉. you can do that locally in your game. 

another thing you can do is to use code pages, which are ways to swtich out ascii for a different character encoding set
which will work with chinese.

with only 256 glyphs, which is what you will get out of ASCII, you wont be able to represent chinese.


14:54
what the programming world has moved to recently is "Unicode" as the more standard thing.

ASCII is like the more old school standard

unicode is a way of representing drastically more characters. It goes up to 16 bits in the naive implementation like in windows 
called UTF-16

UTF-16 gives you up to 65536 representations.

UTF-8 gives you even more. Cuz that is variable length encoding, where its still 8 bit characters, just like ASCII.
But you can kind of double up on the character 
for example, you can have the first character, which will specify a 2nd character that will come after me. That 2nd character 
will combine with me to produce the actual value. So you can kind of chain them to be as long as you want. 

You can get up to 24 bit of address space, which is 16 million glyphs


15:59
so the unicode is the way we break through the ascii table which is just one byte per character. ascii got insufficient.

Unicode has a good and a bad to it. 

the good thing is that, all glyphs has found their home. 
you can find the most obscure language there is, dialects that are no longer spoken.
someone have allocated some set of the unicode mapping for these symbols 

what that means, a single font could literally describe all of the symbols for all human language in all human history. 
and thats pretty cool 


however the problem is that you are stuck with this giant encoding. You cant just use a table that is 65k entries or 16 million entries long.
That is a giant table to use in your game, and most of the entries will be 0 cuz your game is translated into all human languages. 

so usually what ends up happening, typically you have to treat unicode as a intermediate mapping, because your look ups cant be dense anymore.
you typically go through a translation process that you go through, to give you a dense table that describes what your font actually has


                                        Font Table
                                         ___________
                                        |           |
                                        |           |
                                        |           |
        unicode ----> mapping ----->    |           |
                                        |           |
                                        |           |
                                        |           |
                                        |___________|


this Font Table will actually be dense 


so for example, the unicode is 24 bit address space, which has 16 million symbols
and our Font Table may just only have 500 glyphs



                                        Font Table
                                         ___________
                                        |           |
                                        |           |
        24 bit                          |           |
        unicode ----> mapping ----->    |           |
                                        |           |
                                        |           |
                                        |           |
                                        |___________|

this mapping we are doing is essentially crunthing this 16 million symbols down to the size of my font table, essentially
figuring out where in my dense table do I have my valid entries and glyphs

this is just something that people should be aware of unicode becuz unicode is suppose to be "universal encoding" that 
can describe any glyph. Its actually not really good at describing the glyphs that you have. 

so typically what you ending up doing if you want to support arbituary languages in a system is that you accept unicode
as your encoding, but then whenever you get a font, and you want to start displaying something on a screen, you will 
go through a mapping to produce your dense table of glyphs that you actually have. 



19:28
entries in to the unicode table are called codepoints.  

so when you hear people say that "codepoint", they are referring to that numerical slot that has been reserved for this glyph. 

for example: slot 632341 which correspondes to whatever glyph, that is a codepoint

20:04
what the font files end up being is typically a unicode font file is 
here is an outline of a T, here is the unicode codepoint that describes what glyphs im talking about.

the codepoint is almost all the information we need. But there is still some information missing.

That is becuz we dont actually know how to assemble multiple characters together into words. 

for example in basic typography, you have ways to describe how a letter is written 

in english we have tihs baseline, and it describes what part of a character is below or above that baseline 

apparently parts below the baseline is called the descender. so here 'y' has a descender

parts above the line above the base line is called the ascender. so here 't' and 'l' has ascender


-----------------------------------------------------
                                            ##
                                            ##   
                ###                         ##
                ###                         ##
-------------#########-----##---------##----##-------
                ###         ##       ##     ##
                ###          ##     ##      ##
                ###           ##   ##       ##
                 ###           ## ##        ##
-------------------###----------###---------##-------   <--- baseline
                                ##
                               ##
                              ##

-----------------------------------------------------
                     
so in a font file, it will describe how a character glyph relate to that.

or out of all the character that extends below the baseline, which character has the longest descender height.

or out of the ascenders which one the tallest.

or what is the line height, spacing between lines.


and most important of all, which unfortunately most font files dont do this very well
your font file has this thing called "kerning". Kerning is a way of understanding how to adjust the positioning 
of letters based on which pairs of letters you are talking about. 


23:50
to understand kerning, we have to understand the difference between  
"mono spaced" vs "proportional spaced" fonts 

the mono spaced font is the old school kind that was on typewriters or terminals. It was technology where people couldnt 
advance the write position by a variable number of pixels or space on a type page.

so all characters have to have equal amount of space. 

so if you have a 'g' and an 'i', they both have to take up the same amount of space width wise 

            |     |     |
            |  g  |  i  |
            |     |     |
            
which looks pretty dumb, becuz the 'g' is much fatter than the 'i'.
so you pretty much find a spacing that would work for all characters.

so the word "gill" will look something like 


            g  i  l  l

where the spacing in between is consistent among the center points, whereas typographers would prefer 

            gill 



25:44
so once "proportional spaced" fonts came out, that was definitely the preferred way to do so cuz its a lot more readable. 

so in computing we want to do the same thing. when we render a glyph, we want to have some understanding of how far to go 
to render the next glyph. and its actually a pretty tricky problem becuz the actual distance you want to go is not actually 
consistent depending on the letter. 

A naive implementation of "proportional spaced" font is for every letter, theres a distance that I will advance. 
that is fine in general. 

what it doesnt solve the concept of depending on which two letters you put back to back, you might choose a different amount of  
space. 

For example, suppose I have 'g' and 'j', becuz that we dont want the descender of the 'j' touch 'g', we want more space
in comparison to 'i' and 'j'

-----------------------------------------------------
                                            
                                               
                                      ##
                                          
------------------####-####-----------##-------------
                 ###     ###          ##     
                ###      ###          ##     
                ###      ###          ##     
                 ###     ###          ##     
-------------------#########----------##------------  
                         ###          ##
                        ###   ##      ##
                 ###   ###     ##    ##
                   ######        ####
-----------------------------------------------------




and you see that becuz of 'i' doesnt have a descender, the spacing between 'i' and 'j' is much smaller

-----------------------------------------------------
                                            
                                               
                                 ##   ##
                                          
---------------------------------##---##-------------
                                 ##   ##     
                                 ##   ##     
                                 ##   ##     
                                 ##   ##     
---------------------------------##---##------------  
                                      ##
                              ##      ##
                               ##    ##
                                 ####
-----------------------------------------------------

this is a backwards looking example. the same thing can be said about the forward direction 

if I have 'f' and 'h', i dont want the ascender of 'f' touching the 'h'
and comapre that to 'f' and 'i', the spcaing would be much shorter. 

in fact, this is so common that there is this thing called the "ligature", (which we wont go in depth in handmade hero)
where the fontagrapher designed a unified glyph for 'f' and 'i' together. 



-----------------------------------------------------
                                  #####          
                                 ##   ##           
                                 ##   ##
                                 ##         
------------------------------##########-------------
                                 ##   ##     
                                 ##   ##     
                                 ##   ##     
                                 ##   ##     
---------------------------------##---##------------  
                                      



-----------------------------------------------------

its essentially a combination of an 'f' and an 'i'

28:37
point being, the distance between 'f' and 'i' can be dependent on the pair of letters, not just the letter
that came first and just advancing a fixed amount. 

this is when kerning comes in, which is the idea of modifying the inter character spacing to account for that pairwise 
difference.

so what you end up having is another table in the font file, that describes this information


      first  2nd     kerning 
      char   char    value
     _______________________
    |       |       |       |   
    |   g   |   j   |   5   |
    |_______|_______|_______|
    |       |       |       |   
    |   i   |   j   |   1   |
    |_______|_______|_______|
    |       |       |       |   
    |       |       |       |
    |_______|_______|_______|
    |       |       |       |   
    |       |       |       |
    |_______|_______|_______|
    |       |       |       |   
    |       |       |       |
    |_______|_______|_______|
    |       |       |       |   
    |       |       |       |
    |_______|_______|_______|


often times the way kerning is represented for encoding reasons, is not the total amount spacing, 
kerning typically refers to the delta between normal spacing and this pair of spacing 

so you would specify a normal spacing, anormal advancement for a 'g'
so you would say 'g' advances 10 units.

and the kerning for 'g' and 'j' would be, 10 - 3 = 7

      first  2nd     kerning 
      char   char    value
     _______________________
    |       |       |       |   
    |   g   |   j   |   3   |
    |_______|_______|_______|
    |       |       |       |   
    |   i   |   j   |   1   |
    |_______|_______|_______|
    |       |       |       |   
    |       |       |       |
    |_______|_______|_______|
    |       |       |       |   
    |       |       |       |
    |_______|_______|_______|
    |       |       |       |   
    |       |       |       |
    |_______|_______|_______|
    |       |       |       |   
    |       |       |       |
    |_______|_______|_______|


thats just how that goes. 
the idea behind that is to save space. Recall this table is essentially a combination table, so its (N^2); in space. 
we save space cuz, recall that we have a default value of spacing, and we only put an entry in this kerning table 
if we want to modify it. so the pair of characters is not in the kerning table, you just go with the default value

so kerning became this thing that adjusts the default spacing


32:26
finally, font files contains this thing called "hinting". we wont probably care much about "hinting".
if you are trying to make fonts appear very very tiny, then there are thing that you dont really want to do.

so if you want to rasterize fonts on a very small scale.

lets say we want to render the T. The T here is half a pixel wide 
     _______________________________________________________________
    |       |       |       |       |       |       |       |       |   
    |   ####|#######|#######|#######|#######|###    |       |       |  
    |___####|#######|#######|#######|#######|###____|_______|_______|  
    |   ####|#######|#######|#######|#######|###    |       |       |     
    |       |       |    ###|###    |       |       |       |       |  
    |_______|_______|____###|###____|_______|_______|_______|_______|
    |       |       |    ###|###    |       |       |       |       |          
    |       |       |    ###|###    |       |       |       |       |       
    |_______|_______|____###|###____|_______|_______|_______|_______|
    |       |       |    ###|###    |       |       |       |       |           
    |       |       |    ###|###    |       |       |       |       |      
    |_______|_______|____###|###____|_______|_______|_______|_______|
    |       |       |    ###|###    |       |       |       |       |         
    |       |       |    ###|###    |       |       |       |       | 
    |_______|_______|____###|###____|_______|_______|_______|_______|
    |       |       |    ###|###    |       |       |       |       |     
    |       |       |    ###|###    |       |       |       |       | 
    |_______|_______|_______|_______|_______|_______|_______|_______|
    |       |       |       |       |       |       |       |       |     
    |       |       |       |       |       |       |       |       |  
    |_______|_______|_______|_______|_______|_______|_______|_______|
    |       |       |       |       |       |       |       |       |     
    |       |       |       |       |       |       |       |       |  
    |_______|_______|_______|_______|_______|_______|_______|_______|

even with sub-pixel rendering, what we would get is a very wide gray T


     _______________________________________________________________
    |@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|       |       |       
    |@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|       |       |  
    |@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|_______|_______|  
    |@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|       |       |     
    |@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|       |       |  
    |@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|@@@@@@@|_______|_______|
    |       |       |@@@@@@@|@@@@@@@|       |       |       |       |          
    |       |       |@@@@@@@|@@@@@@@|       |       |       |       |       
    |_______|_______|@@@@@@@|@@@@@@@|_______|_______|_______|_______|
    |       |       |@@@@@@@|@@@@@@@|       |       |       |       |           
    |       |       |@@@@@@@|@@@@@@@|       |       |       |       |      
    |_______|_______|@@@@@@@|@@@@@@@|_______|_______|_______|_______|
    |       |       |@@@@@@@|@@@@@@@|       |       |       |       |         
    |       |       |@@@@@@@|@@@@@@@|       |       |       |       | 
    |_______|_______|@@@@@@@|@@@@@@@|_______|_______|_______|_______|
    |       |       |@@@@@@@|@@@@@@@|       |       |       |       |     
    |       |       |@@@@@@@|@@@@@@@|       |       |       |       | 
    |_______|_______|@@@@@@@|@@@@@@@|_______|_______|_______|_______|
    |       |       |       |       |       |       |       |       |     
    |       |       |       |       |       |       |       |       |  
    |_______|_______|_______|_______|_______|_______|_______|_______|
    |       |       |       |       |       |       |       |       |     
    |       |       |       |       |       |       |       |       |  
    |_______|_______|_______|_______|_______|_______|_______|_______|


and thats not what you want for readability. What you want is to snap to the pixel 

     _______________________________________________________________
    |#######|#######|#######|#######|#######|       |       |       |   
    |#######|#######|#######|#######|#######|       |       |       |  
    |#######|#######|#######|#######|#######|_______|_______|_______|  
    |       |       |#######|       |       |       |       |       |     
    |       |       |#######|       |       |       |       |       |  
    |_______|_______|#######|_______|_______|_______|_______|_______|
    |       |       |#######|       |       |       |       |       |          
    |       |       |#######|       |       |       |       |       |       
    |_______|_______|#######|_______|_______|_______|_______|_______|
    |       |       |#######|       |       |       |       |       |           
    |       |       |#######|       |       |       |       |       |      
    |_______|_______|#######|_______|_______|_______|_______|_______|
    |       |       |#######|       |       |       |       |       |         
    |       |       |#######|       |       |       |       |       | 
    |_______|_______|#######|_______|_______|_______|_______|_______|
    |       |       |       |       |       |       |       |       |     
    |       |       |       |       |       |       |       |       | 
    |_______|_______|_______|_______|_______|_______|_______|_______|
    |       |       |       |       |       |       |       |       |     
    |       |       |       |       |       |       |       |       |  
    |_______|_______|_______|_______|_______|_______|_______|_______|
    |       |       |       |       |       |       |       |       |     
    |       |       |       |       |       |       |       |       |  
    |_______|_______|_______|_______|_______|_______|_______|_______|

so what hinting was to describe this information, where you try to line them up with pixels
to make them rasterized more legitble.

its not very relevant anymore on super high resolution display such as retina display. 
when the pixel count high enough, this doesnt matter anymore.


35:30
our job is to build something that will allows take these font files and use them in handmade hero.


36:14
1.  we can load .ttf files. we can rasterize the fonts directly. We have a few approaches
we would load in the font glyphs, we will turn the character outlines into triangles. this is a process called 
tesselation. 

theres a caveant. if you have any curve on any character, we have to some discretization. 
[you have done the zillow clone, you know this]

and if you zoom in on this, you will see its no longer a curve, but discrete lines 

of course we can always solve that by writing a very fast tesselator, and we tesselate dynamically.
how big do you want the 'S', and we will tesselate it depending on the size. 

so approach 1 is load .ttf and tesselate.



2.  load TTF, implicitly Rasterize.
we actually did this when we did our renderer 
                ________________
               /               /
              /               / 
             /               /  
            |       o       /   
            |              /    
            |             /     
            |            /      
            |___________/

we determine whether a pixel is filled by doing edge tests on all edges. if a point is on the inside of all edges,
then its a point inside our glyph outline.


essentially this is determine if a point is inside a polygon.
[we did this in our zillow clone]

the benefit of this approach is that it works for any resolution. not matter how you shrink or scale or anything, we will
have smooth curves.



both option 1 and 2 are all overkill for 99.9% of all games. we dont want to go down this route unless we have to. 

3.  
pre-rasterized fonts 

basically pre rasterized fonts basically turns fonts into something your game can already handle. we already know 
we can blip bitmaps. So pre-rasterized turns this problem into blitting bitmaps. 

so we build bitmaps that capture fonts at a certain solution.
For example, for handmade hero, we want 1920 x 1080, and lets say we want fonts to show up roughly 40px wide and high.  
we will save the results of that rasterization into a set of bitmaps or one sprite sheet of bitmaps. 

and when we want to draw these, we will put them back on the screen by blitting them. 

this is also nice cuz its not happening at run time. For option1 and option2, you have to start considering performance. 
no matter how complicated the glyphs is, it wont effect option3.


43:48
so how do we do pre-rasterized fonts. Typically what you do is that you start with a bitmap, and you start packing glyphs 
into it. 
         _________
        | A B C D |
        | E F G H |
        | I J K L |

               
and you end up with a bitmap that has all your letters in it. 

and handmade hero, we dont really pack bitmaps so far, so we can also store them separately, as separate assets in our asset file.


44:44
so what do we ant to store in the bitmap. we dont need the color information. The fonts will mostly likely be colored
at runtime. 
so it can just be a monochrome bitmap. It can just be a 8-bit value that tells me how much coverage there was. 

you then need a bunch of tables. 

first we need the mapping of codepoints to bitmaps.

so if you are just storing bitmaps, it will map to bitmap id 
if you are doing the sprite sheet approach, it will need the location of the rectangle. 

then we need the positioning information of the glyph (above the baseline or below, how much ascender etc etc.);

finally we need the kerning table. 



49:55
someone in the Q.A ask if there arey licensing concerns in implementing font.
For option 1 and 2, its kind of a gray already
option 3, you never need licensing, if you are in the US.

if you are in europe, that may not be true. 
Fonts in the US can not be copyrighted.



54:33 
someone in the Q/A asked will bitmap fonts remain relevant in the coming years, or we will starting 
working with vector fonts?
Casey says, he doesnt think bitmap fonts are really relevant anymore except in retro style looking games.
Vector fonts are pretty much the thing, and you tend to render them down to bitmaps. 

