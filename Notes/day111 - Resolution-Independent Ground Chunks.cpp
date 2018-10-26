Handmade Hero Day 111 - Resolution-Independent Ground Chunks

Summary:
turned the rendering ground chunks back on 
discussed that in the future we would want Orthographic Projection for rendering ground chunks
since we dont want it to change based on resolution

For this episode, Casey hacked it to make it render

ended the episode with extremely slow code, which sets up 
for the upcoming optimizations for the renderer

Keyword:
rendering


turned rendering ground back on 
lots of bug fixing


20:25
FillGroundChunk was done in pixel space. changed it so that it is done in world space 
that way it is consistent with everything else

previously, we were intializing ground chunk bitmaps to be 256 x 256


			    uint32 GroundBufferWidth = 256;
			    uint32 GroundBufferHeight = 256;

			    ...
			    ...

			    if(!TranState->IsInitialized)
			    {
			        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
			                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));

			        // TODO(casey): Pick a real number here!
			        TranState->GroundBufferCount = 64;
			        TranState->GroundBuffers = PushArray(&TranState->TranArena, TranState->GroundBufferCount, ground_buffer);
			        for(uint32 GroundBufferIndex = 0;
			            GroundBufferIndex < TranState->GroundBufferCount;
			            ++GroundBufferIndex)
			        {
			            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
			            GroundBuffer->Bitmap = MakeEmptyBitmap(&TranState->TranArena, GroundBufferWidth, GroundBufferHeight, false);
			            GroundBuffer->P = NullPosition();
			        }





23:59
Casey notices a problem for the ground chunks 
in the FillGroundChunk function, we allocate a RenderGroup.
And all the RenderGroup right goes through the Perspective Projection Calculation in the Renderer

We dont really want our ground chunks to differ depending on the resolutions

				internal void
				FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
				{
				    // TODO(casey): Decide what our pushbuffer size is!
				    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);

				    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4),
				                                                    Buffer->Width, Buffer->Height);

				    ...
				    ...


25:15
so the solution Casey came up with is to put the GroundChunk RenderGroup in Orthographic Projection Mode
we dont want perspective projection for the ground

but we wont do this here for now, we will just hack it to make it work
essentially, for the hack we are rendering the bitmaps in perspective modes into the GroundChunks
so Casey is just tweaking numbers to make it fill properly 



47:39
the game ended up very very slow, which made Casey to decide to start optimizations for the renderer

55:27
discussed memory foot print of a game

Casey says: the thing you do not want to do is use memory for no benefit
if a program uses all the memory in the machine and did something cool. That is good work

so the goal should be, use all the memroy in the machine and do something cool.

it doesnt matter if is a 2D game, as long as we are doing something cool.

The goal really isnt to use the least amount the memory. There really is no benefit to that 

