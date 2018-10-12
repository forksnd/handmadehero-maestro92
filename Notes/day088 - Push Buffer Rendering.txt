Handmade Hero Day 088 - Push Buffer Rendering

Summary:

started and made the "skeleton" of the rendering system

added handmade_render_group.h and handmade_render_group.cpp

refactored all the rendering related calls to go through the RenderGroup 

Keyword:
Rendering, Renderer 




58:00
Casey summarized what he did in this lecture
pretty much introduced the idea of going through all the entities / bitmaps we want to render

and push all the relevant information onto a memory (which we call it renderMemory);

then we render this entire list

				Memory
				 ___________________________			
				|							|
				|			Head			|	
				|___________________________|
				|							|
				|			Cape			|
				|___________________________|	
				|							|
				|			Torso			|
				|___________________________|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|___________________________|
				

In our code, the list is called RenderGroup
and for each of the entity / bitmap, we convert it into a "entity_visible_piece", and we push it onto the RenderMemory 

this sets us up for sorting this list and rendering everything with the proper z depth in the game.



10:09
added handmade_render_group.h and handmade_render_group.cpp
and starting move all the rendering related stuff to these to files 



14:57
starting to restructure the rendering code. 
in the first for loop, we go through all the entities, and we push their rendering pieces to the renderGroup.

then in the 2nd for loop, we loop through all the render pieces and render


				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{ 
					...
					...

				    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4),
                                                    GameState->MetersToPixels);

				    // TODO(casey): Move this out into handmade_entity.cpp!
				    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
				    {

				    	...
				    	...

			            switch(Entity->Type)
			            {
		                   	PushBitmap(RenderGroup, &HeroBitmaps->Head, V2(0, 0), 0, HeroBitmaps->Align);        		
		                   	...
		                   	...
        		           	PushBitmap(RenderGroup, &HeroBitmaps->Head, V2(0, 0), 0, HeroBitmaps->Align);
							...
							...
		                    PushBitmap(RenderGroup, &HeroBitmaps->Head, V2(0, 0), 0, HeroBitmaps->Align);
							...
			            }

				    }


				    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize; )
				    {
				        entity_visible_piece *Piece = (entity_visible_piece *)(RenderGroup->PushBufferBase + BaseAddress);

				        ...
				        ...
				        ...

				        if(Piece->Bitmap)
				        {
				            DrawBitmap(.....);
				        }
				        else
				        {
				        	...
				            DrawRectangle(...;
				        }
				    }
				}




18:55
as you can see in the code shown in 14:57. we have a problem of not knowing where to render entity_visible_piece.
so what we do is remember the location when we iterate through the entities

-	16:21
we created the render_basis struct in handmade_render_group.h, 
and we added a render_basis point in both entity_visible_piece class and the render_group.

this way when we render the render_group and entity_visible_piece, we know its sim position.

				handmade_render_group.h

				struct render_basis
				{
					v3 P;
				};


				struct entity_visible_piece
				{
					render_basis* Basis;
					...
					...

					real32 R, G, B, A;
					...
				};


				struct render_group
				{
				    render_basis *DefaultBasis;
				    ...
				    ...

				};








20:46
when we iterate through the entities, we just get a basis out of transient memory arena.

-	not sure why "RenderGroup->DefaultBasis = Basis;" is used. RenderGroup->DefaultBasis is never used. ??????????

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{ 
					...
					...

				    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4),
                                                    GameState->MetersToPixels);


				    // TODO(casey): Move this out into handmade_entity.cpp!
				    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
				    {

				    	...
				    	...
				    
				        render_basis *Basis = PushStruct(&TranState->TranArena, render_basis);
        			    RenderGroup->DefaultBasis = Basis;
		            
        			    ...
        			    ...

				    }


				    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize; )
				    {
				        entity_visible_piece *Piece = (entity_visible_piece *)(RenderGroup->PushBufferBase + BaseAddress);
				        BaseAddress += sizeof(entity_visible_piece);

				        v3 EntityBaseP = Piece->Basis->P;

				        ...
				        ...
				        ...

				        if(Piece->Bitmap)
				        {
				            DrawBitmap(.....);
				        }
				        else
				        {
				        	...
				            DrawRectangle(...;
				        }
				    }
				}


22:59
with this set up, we can now then sort all the entity_visible_piece in RenderGroup, so that we can fix the character going behind trees problem.
However, according to Casey, now is not a good time to teach sorting....




23:50
giving the RenderGroup its own memory. and we also added code to clean up its memory
Note: Casey mentioned that this is not what he wants to do in the long run

			    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
			    // TODO(casey): Decide what our pushbuffer size is!
			    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4),
			                                                    GameState->MetersToPixels);
			    
			    				    // TODO(casey): Move this out into handmade_entity.cpp!
			    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
			    {
			    	...
			    	...			    
			    }


			    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize; )
			    {
			        ...
			        ...
			    }


			    EndSim(SimRegion, GameState);
			    EndTemporaryMemory(SimMemory);
			    EndTemporaryMemory(RenderMemory);




25:50 
proceeds to change all rendering calls to go through the RenderGroup

For example, for our ground buffers, we used to have 


			    for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex)
			    {
			        ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
			        if(IsValid(GroundBuffer->P))
			        {
			        	...
			        	...       
			            DrawBitmap(RenderGroup, Bitmap, Delta.XY, Delta.Z, 0.5f*V2i(Bitmap->Width, Bitmap->Height));
			        }
			    }

now we have 

			    for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex)
			    {
			        ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
			        if(IsValid(GroundBuffer->P))
			        {
			        	...
			        	...       
			            PushBitmap(RenderGroup, Bitmap, Delta.XY, Delta.Z, 0.5f*V2i(Bitmap->Width, Bitmap->Height));
			        }
			    }

			    ...
			    ...

			    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize; )
			    {
			        entity_visible_piece *Piece = (entity_visible_piece *)(RenderGroup->PushBufferBase + BaseAddress);
			        BaseAddress += sizeof(entity_visible_piece);

			        v3 EntityBaseP = Piece->Basis->P;

			        ...
			        ...
			        ...

			        if(Piece->Bitmap)
			        {
			            DrawBitmap(.....);
			        }
			        else
			        {
			        	...
			            DrawRectangle(...;
			        }
			    }


as you can see, as we iterate through the ground chunks, we call PushBitmap. PushBitmap is essentially saving all the commands
needed for DrawBitmap;

Then, when we go through all the entity_visible_piece in RenderGroup, we offically call DrawBitmap or DrawRectangle



43:30
refactored render_group. 

previously we had
				struct render_group
				{

					entity_visible_piece[4096];
				};

now we have 
The idea is that now render_group has a chunk of memory you can use. 
we are doing this becuz when we call PushBitmap or PushRectangle calls, the Bitmap that comes with it 
will have different sizes.
so when we add entity_visible_piece to render_group, it will use different sizes of memory as well

				struct render_group
				{
					...
					...

				    uint32 MaxPushBufferSize;
				    uint32 PushBufferSize;
				    uint8 *PushBufferBase;
				};
									



44:17
created the AllocateRenderGroup function call.

-	notice there are multiple calls to Allocate memory

-	first we allocate the memory for Render_Group itself

-	then the "Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize);"
	allocates more memory 

-	thirdly, the allocate memory for DefaultBasis


				handmade_render_group.cpp

				internal render_group *
				AllocateRenderGroup(memory_arena *Arena, uint32 MaxPushBufferSize, real32 MetersToPixels)
				{
				    render_group *Result = PushStruct(Arena, render_group);
				    Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize);

				    Result->DefaultBasis = PushStruct(Arena, render_basis);
				    Result->DefaultBasis->P = V3(0, 0, 0);
				    Result->MetersToPixels = MetersToPixels;
				    Result->PieceCount = 0;

				    Result->MaxPushBufferSize = MaxPushBufferSize;
				    Result->PushBufferSize = 0;
				    
				    return(Result);
				}


in memory, it loos like this 
				 ___________________________			
				|							|
				|		Render_Group		|	
				|___________________________|
				|							|
				|		PushBufferBase		|
				|							|	<---- where all the PushBitmap commands will push to
				|							|
				|	MaxPushBufferSize		|
				|		of memory 			|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|___________________________|
				|							|
				|		DefaultBasis		|
				|___________________________|

				

51:06
so we wrote these two functions.

-	the line "if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)"
	that checks if we can fit into our memory


				inline void *
				PushRenderElement(render_group *Group, uint32 Size)
				{
				    void *Result = 0;

				    if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
				    {
				        Result = (Group->PushBufferBase + Group->PushBufferSize);
				        Group->PushBufferSize += Size;
				    }
				    else
				    {
				        InvalidCodePath;
				    }

				    return(Result);
				}


and we proceed to refactor the PushPiece function.

				inline void
				PushPiece(render_group *Group, loaded_bitmap *Bitmap,
				          v2 Offset, real32 OffsetZ, v2 Align, v2 Dim, v4 Color, real32 EntityZC)
				{
				    entity_visible_piece *Piece = (entity_visible_piece *)PushRenderElement(Group, sizeof(entity_visible_piece));
				    Piece->Basis = Group->DefaultBasis;
				    Piece->Bitmap = Bitmap;
				    Piece->Offset = Group->MetersToPixels*V2(Offset.X, -Offset.Y) - Align;
				    Piece->OffsetZ = OffsetZ;
				    Piece->EntityZC = EntityZC;
				    Piece->R = Color.R;
				    Piece->G = Color.G;
				    Piece->B = Color.B;
				    Piece->A = Color.A;
				    Piece->Dim = Dim;
				}


Notice that PushBitmap just calls PushPiece




so right now our memory looks like this 

				 ___________________________			
				|							|
				|		Render_Group		|	
				|___________________________|
				|							|
				|	entity_visible_piece	|
				|							|	
				|				bitmap -----|-----> somewhere in Transient memory
				|___________________________|
				|							|
				|	entity_visible_piece	|
				|							|	
				|				bitmap -----|-----> somewhere in Transient memory
				|___________________________|
				|							|
				|	entity_visible_piece	|
				|							|	
				|				bitmap -----|-----> somewhere in Transient memory
				|___________________________|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|							|
				|___________________________|
				|							|
				|		DefaultBasis		|
				|___________________________|


53:21
with this format, we change the for loop where we iterate our entity_visible_pieces. Since we do not have a "entity_visible_piece" array,
we have to step through by "sizeof(entity_visible_piece". 

			    ...
			    ...

			    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize; )
			    {
			        entity_visible_piece *Piece = (entity_visible_piece *)(RenderGroup->PushBufferBase + BaseAddress);
			        BaseAddress += sizeof(entity_visible_piece);

			        v3 EntityBaseP = Piece->Basis->P;

			        ...
			        ...
			        ...

			        if(Piece->Bitmap)
			        {
			            DrawBitmap(.....);
			        }
			        else
			        {
			        	...
			            DrawRectangle(...;
			        }
			    }








