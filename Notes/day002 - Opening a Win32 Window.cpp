4:30
a struct in C/C++ may have alignments/padding

4:50
they want to support both C and C++
in C when you want to declare a struct, you need to add the "struct" in front

struct foo
{
	int x;
};

struct foo Foo;

in C++, they got rid of that

of course, in C mode, you can still use typedef

Examples:

				typedef struct foo foo;
				foo Foo;

				typedef struct foo pFoo;
				pFoo Foo;


 10:28
 you can initlaize a struct with "{xxx, xxx}" style

Example:

				MyStruct myStruct = {5, "nice"};


if you just put "{}", that just makes memory all 0


17:57
DC:	device context
something windows uses to keep the state of drawing while we are interfacing with it to draw to our window

Windows typically keeps a few DCs

when ppl need one, windows give them one, and they get one. When they are finished, they return it to Windows

if you use CS_OWNDC, you wont have to deal with the overhead of request and releasing DC. 




43:43
need a message loop
windows does not by default start sending messages to your window unless you actually start pulling them off "the queue"

anytime you have an application in windows, it creates a queue of messages for you that essentially starts filling up with messages that windows is trying to send you
or anyone else that is sending you message thru the windows system.




