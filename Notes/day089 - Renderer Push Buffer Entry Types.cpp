Handmade Hero Day 089 - Renderer Push Buffer Entry Types

Summary:
writing more on the Renderer system. 

support Pushing different types of render entry to our renderer

currently supporting three types: 

RenderGroupEntryType_render_entry_clear
RenderGroupEntryType_render_entry_bitmap
RenderGroupEntryType_render_entry_rectangle


Keyword:
Renderer




5:16 
continuitng on what we started last episode, the idea is that 

we want the game to keep on pushing rendering commands to our "Push Buffer"
the game eventually renders all these commands and discards it entirely


									Push Buffer

									Memory
		Game					 	 ___________________________			
									|							|
		 ___________				|			Head			|	
	    | 			|	------->	|___________________________|
		| 			|				|							|
		| 			|				|			Cape			|
		| 			|				|___________________________|	
	 	|___________|				|							|
									|			Torso			|
									|___________________________|
	generate draw calls				|							|
	minimally						|							|
									|							|
									|							|
									|							|
									|							|
									|							|
									|							|
									|___________________________|
							

Reasons to do this
1. 	sort
	you can sort the entities in the order they render and not do this Push Buffer thing. But that complicates game code
	unnecessarily. We do not want to build the whole game architecture just around how you want to render the entities.

	In the old days, you actually had to build the game architecture around how you want to render becuz you do have enough
	processing power and memory. Your game literally wont be able to run unless you do so

2.	have the flexibility to define output target based on our target
	OpenGL
	Direct3D
	Mental
	our software renderer
		SSE2
		NEON
		AVX
	...
	...

	this allows us to sort our render calls that is most efficient for the platform without having to rewrite our game code








14:41
we plan to write a software "GPU-Esque" renderer. We want to write our software renderer roughtly how a GPU would work

There is no point to ship a software renderer these days


17:01
Graphical explanation: (Couldnt draw diangoal lines)
Casey explains how a scan line renderer works. the old school way

not gonna do a scan line renderer.




18:29
the way we will do it in is a "implicit" surface rasterizer

what we will do get a rough approximiation of where they will be on some boundary

this is how GPU work nowadays.



20:07
so this is how our will renderer look


Push Buffer 		---->		Binning Phase 		---->		4 x 4 selection		---->	4 x 4 				
																				 			rasterization
						(figures out where on the
							screen stuff lives)												4 x SSE2	4 wide


																				(this step is also where our shaders will come in)


22:55
becuz we are a sprite based engine, and right now we are primarily working with rectangles
we probably will be working exlusively in rectangles. Although we may think about things in triangles on rare occation
as you will see when we get to the rasterization stage. it is as expensive to fill a rectangle as it to fill a triangle.
so we essentially save ourselves half of the work.




27:42
offically making the psuh to write this renderer

renaming renderer friendly 

				struct entity_visible_piece
				{
				    render_basis *Basis;
				    loaded_bitmap *Bitmap;
				    v2 Offset;
				    real32 OffsetZ;
				    real32 EntityZC;

				    real32 R, G, B, A;
				    v2 Dim;
				};

now we have


				struct render_entity_basis
				{
				    render_basis *Basis;
				    v2 Offset;
				    real32 OffsetZ;
				    real32 EntityZC;
				};







34:54

as mentioned earlier, we will be pushing all kinds of different draw calls to our Render Group.
So depending on the type, we need to process them differently 

we now have a switch statement here. Do note that we do not implement clear here yet.
that is done in episode day090




notice we added another maco 
#define InvalidDefaultCase default: {InvalidCodePath;} break

full code below 


				internal void
				RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
				{
				    v2 ScreenCenter = {0.5f*(real32)OutputTarget->Width, 0.5f*(real32)OutputTarget->Height};

				    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize;)
				    {
				        render_group_entry_header *Header = (render_group_entry_header *)
				            (RenderGroup->PushBufferBase + BaseAddress);
				        switch(Header->Type)
				        {
				            case RenderGroupEntryType_render_entry_clear:
				            {
				                render_entry_clear *Entry = (render_entry_clear *)Header;
				                
				                BaseAddress += sizeof(*Entry);
				            } break;

				            case RenderGroupEntryType_render_entry_bitmap:
				            {
				                render_entry_bitmap *Entry = (render_entry_bitmap *)Header;
				                v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCenter);
				                Assert(Entry->Bitmap);
				                DrawBitmap(OutputTarget, Entry->Bitmap, P.X, P.Y, Entry->A);
				                
				                BaseAddress += sizeof(*Entry);
				            } break;

				            case RenderGroupEntryType_render_entry_rectangle:
				            {
				                render_entry_rectangle *Entry = (render_entry_rectangle *)Header;
				                v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCenter);
				                DrawRectangle(OutputTarget, P, P + Entry->Dim, Entry->R, Entry->G, Entry->B);
				                
				                BaseAddress += sizeof(*Entry);
				            } break;

				            InvalidDefaultCase;
				        }
				    }
				}




38:38
defined the different types of render_group_entry

				enum render_group_entry_type
				{
				    RenderGroupEntryType_render_entry_clear,
				    RenderGroupEntryType_render_entry_bitmap,
				    RenderGroupEntryType_render_entry_rectangle,
				};
				struct render_group_entry_header
				{
				    render_group_entry_type Type;
				};

				struct render_entry_clear
				{
				    render_group_entry_header Header;
				    real32 R, G, B, A;
				};

				struct render_entry_bitmap
				{
				    render_group_entry_header Header;
				    loaded_bitmap *Bitmap;
				    render_entity_basis EntityBasis;
				    real32 R, G, B, A;
				};

				struct render_entry_rectangle
				{
				    render_group_entry_header Header;
				    render_entity_basis EntityBasis;
				    real32 R, G, B, A;
				    v2 Dim;
				};







45:26

did a #define trick 

notice that we defined 
				
				enum render_group_entry_type
				{
				    RenderGroupEntryType_render_entry_clear,
				    RenderGroupEntryType_render_entry_bitmap,
				    RenderGroupEntryType_render_entry_rectangle,
				};

which are huge and long names.
so to save ourselves some typing, we defined a macro 

				#define PushRenderElement(Group, type) (type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type)

we will add the "RenderGroupEntryType_" prefix for us.
(I personally do not like it, cuz this just save us a bit or typing)




				inline render_group_entry_header *
				PushRenderElement_(render_group *Group, uint32 Size, render_group_entry_type Type)
				{
				    render_group_entry_header *Result = 0;

				    if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
				    {
				        Result = (render_group_entry_header *)(Group->PushBufferBase + Group->PushBufferSize);
				        Result->Type = Type;
				        Group->PushBufferSize += Size;
				    }
				    else
				    {
				        InvalidCodePath;
				    }

				    return(Result);
				}





1:04:48
is it worth aligning render entries?

Casey says: at the moment no. Becuz all of our render entires will be 8 byte aligned anyway.  

so if see the following 
they will all be 8-byte aligned

				struct render_entry_clear
				{
				    render_group_entry_header Header;
				    real32 R, G, B, A;
				};

				struct render_entry_bitmap
				{
				    render_group_entry_header Header;
				    loaded_bitmap *Bitmap;
				    render_entity_basis EntityBasis;
				    real32 R, G, B, A;
				};

				struct render_entry_rectangle
				{
				    render_group_entry_header Header;	
				    render_entity_basis EntityBasis;
				    real32 R, G, B, A;
				    v2 Dim;
				};

https://stackoverflow.com/questions/2846914/what-is-meant-by-memory-is-8-bytes-aligned

when start to push stuff that are more variably sized, we will align those
For example, we do not want to leave things on a odd byte boundaries.

you never know with alignment. Sometimes it benefits you, sometimes it hurts you.
if you spend a lot of time aligning and padding, but it ends up taking more total space. 
you may find that your memory bandwidth or cache is worst.


1:06:13
any recommendations on rasterization except for google search?

Casey recommends:
https://fgiesen.wordpress.com/








1:12:25
Casey prefer memory based api design when you have a problem that is straightforward
like a renderer, that has very well defined ins and outs

