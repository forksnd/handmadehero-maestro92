Handmade Hero Day 021 - Loading Game Code Dynamically


Summary:

Keyword: 


6:10

one benefit of doing scripting language is that you can edit things on the fly.


8:14
edit and continue





9:08
we have the platform code and the non-platform code compiled separately. Is there a way that doesnt kill the executable?




9:40
we have the ability to load code out of a library dynamically.

we can load a "dll" file and call GetProcAddress on it. GetProcAddress will give us a pointer to that function, 
and we can just call the function that way.




22:50

instead of actually callin functions that were in its executable that the linker resolved,
now is that it makes function calls through pointers that we are loading dynamically

