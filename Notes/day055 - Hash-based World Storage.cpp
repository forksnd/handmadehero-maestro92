Handmade Hero Day 055 - Hash-based World Storage

Summary: Talks about hash table for the first 30 minutes

uses a hash table for Sparse World Storage

implements a hash table from scratch. 
implements a crappy hash function for the hash table



27:11
compares the two common ways to resolve hash table collision resolution
linked list vs open addressing

linked list has a better worst case scenario



28:38
so if you just want a constant amount of memory (which we often do in Game Development);, it is better to use the open addressing version
Example: if you just have 512 mb of storage

512 in the open addressing case will be all devoted to the hash table elements


whereas in the linked list version, you will have to split the two up:
maybe 256 for hash table elements, then 256 for linked list nodes.

so hash table collisions will be more frequent in the lineked list version if we are using constant amount of memory






32:21
refactored the tile_chunk class
notice that the tile_chunk has a pointer to another tile_chunk,
that is becuz we are doing the linked_list version of hashtable here. hence the pointer

                struct tile_chunk
                {
                    int32 TileChunkX;
                    int32 TileChunkY;
                    int32 TileChunkZ;

                    // TODO(casey): Real structure for a tile!
                    uint32 *Tiles;

                    tile_chunk *NextInHash;
                };




34:01
he implements a hashtable using a simple array of fixed size


                struct tile_map
                {
                    int32 ChunkShift;
                    int32 ChunkMask;
                    int32 ChunkDim;
                    
                    real32 TileSideInMeters;

                    // NOTE(casey): At the moment, this must be a power of two!
                    tile_chunk TileChunkHash[4096];
                };


it is worth nothing that making it powers of 2 is not necessarily the smartest thing to do
since when we do the mapping, there is a number of theoretical reasons why you may not want a power of 2

powers of 2 directly truncates the bits that come out of a hash function.



39:19
implements a random hash function

                uint32 HashValue = 19*TileChunkX + 7*TileChunkY + 3*TileChunkZ;
                uint32 HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);
                Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));
                
for the second line 

                HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);

we know that ArrayCount(TileMap) is a power of 2 (something we hardcoded at the time of this episode. we just made TileChunkHash[4096])

for powers of two, it will always look like "100000000", a "1" followed by a bunch of zeros.
doing "ArrayCount(TileMap->TileChunkHash) - 1", creates a mask of "0111111111".
making HashValue & "0111111111" makes sure that our HashSlot value is within the array max index,

which is the reason why we added an assert line 

                Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));




41:34
adds more checks in the hash function implementation


                Assert(TileChunkX > -TILE_CHUNK_SAFE_MARGIN);
                Assert(TileChunkY > -TILE_CHUNK_SAFE_MARGIN);
                Assert(TileChunkZ > -TILE_CHUNK_SAFE_MARGIN);
                Assert(TileChunkX < TILE_CHUNK_SAFE_MARGIN);
                Assert(TileChunkY < TILE_CHUNK_SAFE_MARGIN);
                Assert(TileChunkZ < TILE_CHUNK_SAFE_MARGIN);
                
                // TODO(casey): BETTER HASH FUNCTION!!!!
                uint32 HashValue = 19*TileChunkX + 7*TileChunkY + 3*TileChunkZ;
                uint32 HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);
                Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));
                
reason that is cuz we want a way to know if a slot in the hash table is filled or not.
Example if your hash slot is initialized to be {0,0,0}, and your entity position happens to hash to {0,0,0}
we can not tell if the slot is taken or not.

so we made the assumption that entities will be inside the TILE_CUNK_SAFE_MARGIN_ZONEs






1:22:43

would u consider splitting off the hash table code so that other system can use this hash table?

I understand that this can be a tempting thing to do. But I do not think it is a good idea to do it in this case.

in this case, the only thing that is common among hash tables is 


                uint32 HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);
                Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));

so you are only saving two lines


1:35:59
RLE: Run Length Encoding




