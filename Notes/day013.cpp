3:10
function overloading

you're allowed to define multiple functions with the same name, but with different inputs

17:40
regarding input, you are always one frame behind

18:45
in a frame worth of time, out of all the inputs the user did, what user input do we save

input devices has polling frequency



21:00

people often process inputs as such
		

					for(int eventIndex = 0; eventIndex < eventCount; ++eventIndex)
					{
						switch (event[eventIndex])
						{
							case event[eventIndex].stickX:
								break;

							...
							...
						}

					}


	or it will just be a state of the joystick

this way of doing this stuff has a limitation
that limiation is that you have to have somewhere to store the events.
And the number of events you choose to store, determines how many buttons you can press in a given frame.


for minimal storage, u just need
the state of the button now, and how many half transitions it went through


1:20:08
all of our functiosn are internal. They are all internal becuz we only have one translation unit, so everything has static in front of it.
so that is faster for the compiler, so it will know that it does not have to do any external linking for anything

