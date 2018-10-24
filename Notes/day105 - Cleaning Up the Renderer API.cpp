Handmade Hero Day 105 - Cleaning Up the Renderer API

Summary:
cleaning up the renderer code
alot of it is finalizing calling conventions 
locking down coordinate system rules.

Nothing interesting

Keyword:
renderer clean up. 


8:10
placed all the declaration of all the rendering API calls in handmade_render_group.h


11:09
mentioned that he wants to make 
all color values specified to the renderer as V4s are in NON-premultiplied alpha

the renderer will be responsible turning it into premultiplied alpha


17:40
specified calling conventions for the bitmaps
put the anchor point of the loaded_bitmap into the struct itself.

				struct loaded_bitmap
				{
				    v2 Align;
				    
				    int32 Width;
				    int32 Height;
				    int32 Pitch;
				    void *Memory;
				};


35:52
fixing the shadow
the shadows was "following" the character as the jump.
mentioned that, it is the users responsiblity to figure out where the ground is, and render the shadow on to the ground
not the renderers responsiblity


1:06:19
rendering the ground onto the ground plane, giving a peek look at how the entity lighting will look


1:08:56
mentioned there is a bug on the edges for the env_map lighting


1:15:23
turning o2 on does not mean you have optimized code.

o2 that means the compiler will no longer write stupid things like writing vairalbles to and from the stack for every operation
when they could have just stayed in registers

that all o2 does. o2 does not optimized your code. It just code that the compielr is not sutpid things. your code is still stupid

