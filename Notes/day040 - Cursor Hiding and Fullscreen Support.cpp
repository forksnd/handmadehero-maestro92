Handmade Hero Day 040 - Cursor Hiding and Fullscreen Support

4:33
the reason for 

				#define internal static
				#define local_persist static
				#define global_variable static

these three guys is becuz static as a keyword in C/C++ that means many different things

it is not about readability, more about searchability

for example, if you want to do "show me all the global_variables"

doing it this way allows for that



16:37
people are wondering why do we have a spinning cursor when entering the game?

the answer is that it is not that we are setting a spinning cursor, it is more becuz whatever the cursor was before entering our window.

mainly we were not responding the WM_SET_CURSOR event call and we are setting the cursor, so windows just does not touch the cursor as soon 
as we entered our window


17:50
there is a tool in visual studio called spy++ (under the "tools" drop down menu)

it shows a list of all the windows in the system. it is much more handy for applications, it is not something you really use for games a whole lot

it teaches you how to use the spy++ tool

and you can see the messages getting dispatched to our window.





39:30
if you have any very specific windows related questions, try to google Raymond Chen for it. Apparently Raymond Chen is an expert on it.
and he usually has the right answers.





50:40
whatever Raymond Chen says works



1:01:08
when the artist is working on art, I do not want them to think about engine problems. I want it to be as fluid for her to make the art, cuz that is when she performs the best. I do not want to create obstacles for her to create good art.




