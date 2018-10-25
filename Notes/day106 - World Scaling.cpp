Handmade Hero Day 106 - World Scaling

Summary:
trying make scaling to work.

To me, this still just feels like it is working on things that can be solved through matrix transformation
if you keep things in sim space and world space properly and cleanly, this is an easy fix

discussed out to do alpha fade out, fade in properly

Keyword:
rendering, entity alpha fade in fade out.


11:24
in terms of scaling, Casey mentiosn he is unsure of two approaches
1.	scale each individual sprite
2.	render everything into a buffer, and scale that

there are pros and cons for each of these approach

scaling is more expensive since, so if you are doing the 2nd approach, you are just scaling up once 
its cheaper then scaling each individual one

But that will incur one extra copy from the buffer to the screen. 

so if you do the first approach and if you do it well, the first approach may be faster


14:05
another thing that seems to be better for approach 1 is that we do not incur any problems with scope
For example, if we want to scale it down, ... (i didnt quite understand this part)


15:03
it argues for actually rendering things the size they are supposed to be.



26:44
mentioned that the tip of the tree is still not correct.

Mentions that still needs a pixel of emptiness around our sprite for the subpixel rendering 
to work properly. (recall that we are doing binlinear sampling in subpixel rendering)



41:32 
Casey starts discussing level1 fading out 


41:50
demonstrated the stupid way of doing alpha, which is to just set the alpha of various components

and he mentions that lots of indie games do this. 


supposed we were introduced an alpha fade value 


				struct render_group
				{
				    real32 GlobalAlpha;

				    ...
				    ...
				};


then in PushBitmap(); we just multiply alpha to all the colors,

				inline void
				PushBitmap(render_group *Group, loaded_bitmap *Bitmap, v3 Offset, v4 Color = V4(1, 1, 1, 1))
				{
				    render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap);
				    if(Entry)
				    {
				        Entry->EntityBasis.Basis = Group->DefaultBasis;
				        Entry->Bitmap = Bitmap;
				        Entry->EntityBasis.Offset = Group->MetersToPixels*Offset - V3(Bitmap->Align, 0);
				        Entry->Color = Group->GlobalAlpha*Color;
				    }
				}

then he played around the alpha value 


44:13
Casey showing us what happens with this stupid approach

entities on screen are not properly fading out, they are turnig into a ghost version of themselves.
for example, you can see through the character head and see the background. 
lets say if there is an object behind your character, if you use this approach, you will be seing it, and that shouldnt happen.

Casey says that is a sign of an amatuer work



45:48

the idea is that 
assuming we are at level1,
we want a way in which as we move from level1 to level2, 
we want to make level2 to do something like an alpha fade to come in


	A		level2		_________________________________________		
										__|
									 __|
								  __|
							   __|
			level1		______|__________________________________ 	the level we are at 




	C		level0		_________________________________________



we will have a renderbuffer that will have the level that is either fading out or fading in 


assume we have RenderBuffer A for level2 and renderBuffer C for level0, whenever we go up or down,
level2 or level0 fade in or fade out properly 

