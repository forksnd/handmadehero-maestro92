Handmade Hero Day 251 - Finishing the Debug Hierarchy

Summary:
cleaning up the debug hierarchy. Making the debug elements show up properly on the debug menu.

Keyword:
Debug System


nothing interesting. its essentially cleaning up the debug view, debug element and debug hierarchy so that 
the debug menu displays properly.


Q/A
1:23:12
right now our startup time is incredibly slow, espeically when being launched from the debugger. 
Casey thinks its because of wgl context creation in the driver.

Someone asked if its possible that visual studio 2015 is slow, where visual studio is doing memory allocations for the debug heap
(espeically when launched from the debugger);

So Casey tested that and found it its only slightly faster, but the startup time is still considerably slow. 

