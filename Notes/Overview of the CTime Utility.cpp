Overview of the CTime Utility

Summary:

Keyword:




1:06
Casey talking about a utility that he wrote. 
this is a utility is designed to give you a perspective on what your build times actually are,
and how much time you are waiting for you project for it to run




essentially the way you use it, is the same where you have timed measuring blocks, but for builds 



7:08
Casey showing us how to use it.
you would add a "ctime -begin handmade_hero.ctm" at the beginning of the build.bat file

and "ctime -end handmade_hero.ctm"

                build.bat 

                ctime -begin handmade_hero.ctm

                ...
                ...
                ctime -end handmade_hero.ctm

then in the command prompt output we see that the it will print out the actual build times:


8:30
Casey mentions that anything that takes under 2 ~ 5 seconds, hes okay with it.

9:48
Casey showing off the "ctime -stats handmade.h" command 



Casey does mention that this code kind of only currently works on the windows platform.


34:14
any code with from the STD library, stands for super bad code. 