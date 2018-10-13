Handmade Hero Day 090 - Bases Part I

Summary:

porting more stuff to the renderer
talked about linear algebra "bases" (or axis) in math and how is it useful in our renderer system 


Keyword:
renderer







10:43

implemented the RenderGroupEntryType_render_entry_clear case int he renderer
we do so by just drawing a full screen Recntangle


				internal void
				RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
				{
				    v2 ScreenCenter = {0.5f*(real32)OutputTarget->Width, 0.5f*(real32)OutputTarget->Height};

				    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize;)
				    {
				        render_group_entry_header *Header = (render_group_entry_header *)(RenderGroup->PushBufferBase + BaseAddress);

				        switch(Header->Type)
				        {
				            case RenderGroupEntryType_render_entry_clear:
				            {
				                render_entry_clear *Entry = (render_entry_clear *)Header;

				                DrawRectangle(OutputTarget, V2(0.0f, 0.0f), V2((real32)OutputTarget->Width, (real32)OutputTarget->Height), Entry->Color.r, Entry->Color.g, Entry->Color.b, Entry->Color.a);

				                BaseAddress += sizeof(*Entry);
				            } break;

				            case RenderGroupEntryType_render_entry_bitmap:
				            {
				            	...
				            	...
				            } break;

				            case RenderGroupEntryType_render_entry_rectangle:
				            {
				            	...
				            	...

				            } break;

				            InvalidDefaultCase;
				        }
				    }
				}






15:53
examine the FillGroundChunk(); function. right now the FillGroundChunk(); is calling the bitmap routines
directly. but Casey planst to port that to the renderer as well




21:41
starting to port FillGroundChunk(); to go through the renderer();

22:48
we probably want the FillGroundChunk to do its job on a separate thread, so we probably want this wo work with a 
different renderGroup


				internal void
				FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
				{
				    // TODO(casey): Decide what our pushbuffer size is!
				    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);
				    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4), 1.0f);

				    Clear(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f));

				    loaded_bitmap *Buffer = &GroundBuffer->Bitmap;

				    ...
				    ...

				    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
				    {
				        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
				        {
				        	...

				            for(uint32 GrassIndex = 0; GrassIndex < 100; ++GrassIndex)
				            {
				            	...
				                PushBitmap(RenderGroup, Stamp, P, 0.0f, V2(0, 0));
				            }
				        }
				    }

				    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
				    {
				        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
				        {
				        	...

				            for(uint32 GrassIndex = 0; GrassIndex < 50; ++GrassIndex)
				            {
				            	...
				                PushBitmap(RenderGroup, Stamp, P, 0.0f, V2(0, 0));
				            }
				        }
				    }

				    RenderGroupToOutput(RenderGroup, Buffer);
				    EndTemporaryMemory(GroundMemory);
				}




31:49
starting to introduce the concept of coordinate systems for the renderer 

"Basis" = "Axis"

axis in game tend to have two property 
-	unit length
-	orthogonal 


Lieanr independence





50:18
gonna start thinking about object space, screen space and world space


when we do "Meters to pixels" or negate the y axis, we are just applying
a transformation from world space to screen space 


				y

				^
				|
				|
	y = 1 meter |		
				|
				|
				|
				------------------------->  x
						x = 1 meter

			world space 



							x = 1 meter to pxiels
						
						------------------------->  x
						|
y = 1 meter to pxiels	|		
						|
						|
						|
						v
								

			screen space 





1:07:73
why are compute coordinates upside down?

Casey: i dont have an offical answer, but the way I think of it is that 

it came from (CRT machines, the ones you play smash on); Cathope Ray Tube
lasers will always shoot to the top left corner of your tv screen.

so when computers came along, engineers would store pixel values in memory
to match the layout and order of the Cathrope Ray exciting each "pixel" on tv screen. 


so as I go up in y in the world coordinate, that means I am going down in y screen space
