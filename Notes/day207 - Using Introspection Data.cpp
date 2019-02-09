Handmade Hero Day 207 - Using Introspection Data

Summary:
followed up on day 206, demonstarted how to add the introspection code into the debug system.
generated metaprogramming code from the simple_preprocessor.cs

Keyword:
introspection, metaprogramming

6:02
so currently in the ParseMember(); function we were printing out member variables from a struct 


                simple_preprocessor.cpp

                static void ParseMember(tokenizer *Tokenizer, token MemberTypeToken)
                {
                    bool Parsing = true;
                    bool IsPointer = false;
                    while(Parsing)
                    {
                        token Token = GetToken(Tokenizer);
                        switch(Token.Type)
                        {
                            case Token_Asterisk:
                            {
                                IsPointer = true;
                            } break;

                            case Token_Identifier:
                            {
        --------------->        printf("{Type_%.*s, \"%.*s\"},\n",
                                       MemberTypeToken.TextLength, MemberTypeToken.Text,
                                       Token.TextLength, Token.Text);                
                            } break;

                            case Token_Semicolon:
                            case Token_EndOfStream:
                            {

                                Parsing = false;
                            } break;
                        }
                    }
                }

so currently if you run the simple_preprocessor program, the output you get is 
something like:
                {
                {Type_world_chunk, "OldChunk"},
                {Type_uint32, "StorageIndex"},
                {Type_bool32, "Updatable"},
                {Type_entity_type, "Type"},
                {Type_uint32, "Flags"},
                {Type_v3, "P"},
                {Type_v3, "dP"},
                {Type_real32, "DistanceLimit"},
                ...
                ...
                }

now Casey wants more info on this, for example, how big they are and where they are in the structure.
this is quite difficult cuz you dont know if the compiler puts padding, or if we are in a #pragma pack section.

6:54
So Casey will change the code so that we can get these info from the compiler



7:13
Casey first adding code to figure out the offset of a member variable inside a struct.

                simple_preprocessor.cpp

                static void ParseMember(tokenizer *Tokenizer, token StructTypeToken, token MemberTypeToken)
                {
                    bool Parsing = true;
                    bool IsPointer = false;
                    while(Parsing)
                    {
                        token Token = GetToken(Tokenizer);
                        switch(Token.Type)
                        {
                            case Token_Asterisk:
                            {
                                IsPointer = true;
                            } break;

                            case Token_Identifier:
                            {
                                printf("{MetaType_%.*s, \"%.*s\"}, (u32)&((%.*s *)0)->%.*s},\n",
                                       MemberTypeToken.TextLength, MemberTypeToken.Text,
                                       Token.TextLength, Token.Text,
                                       StructTypeToken.TextLength, StructTypeToken.Text,
                                       Token.TextLength, Token.Text);          
                            } break;

                            case Token_Semicolon:
                            case Token_EndOfStream:
                            {

                                Parsing = false;
                            } break;
                        }
                    }
                }

this prints out 


                {
                {MetaType_world_chunk, "OldChunk"}, (u32)&((sim_entity *)0)->OldChunk},
                {MetaType_uint32, "StorageIndex"}, (u32)&((sim_entity *)0)->StorageIndex},
                {MetaType_bool32, "Updatable"}, (u32)&((sim_entity *)0)->Updatable},

                ...
                ...
                }


so what is going is that we first have a pointer pointing at 0. Then we cast it to a sim_entity pointer 

                (sim_entity *)0

then we access its OldChunk member variable. 
                
                ((sim_entity *)0)->OldChunk

then we print out the address of the "OldChunk" variable
                
                &((sim_entity *)0)->OldChunk

then we cast it into an u32

                (u32)&((sim_entity *)0)->OldChunk


so now we know the offset of each variables




11:05
in the build.bat file, Casey added the step to generate the code out of the simple_preprocessor.exe 

                ..\..\build\simple_preprocessor.exe > handmade_generated.h

so all the text output from simple_preprocessor.exe will be put into handmade_generated.h

so our handmade_generated.h literraly looks like 

-   also notice that we printed the an extra line of "member_definition MembersOf_sim_entity[] = "


                handmade_generated.h

                member_definition MembersOf_sim_entity[] = 
                {
                    {MetaType_world_chunk, "OldChunk"}, (u32)&((sim_entity *)0)->OldChunk},
                    {MetaType_uint32, "StorageIndex"}, (u32)&((sim_entity *)0)->StorageIndex},
                    {MetaType_bool32, "Updatable"}, (u32)&((sim_entity *)0)->Updatable},

                    ...
                    ...
                };


so this is what Casey describe as a metaprogramming workflow
so you can see that we are just generating data definitions



11:44
now Casey added a new file called "handmade_meta.h"

so this code will tell us all the information about the types that are introspected
you can think of this as the dictionary for all the meta code that we generated. 

    
for example, here in handmade_meta.h, we defined all the meta_type

                handmade_meta.h

                enum meta_type
                {
                    MetaType_world_chunk,
                    MetaType_uint32,
                    MetaType_bool32,
                    MetaType_entity_type,
                    MetaType_v3,
                    MetaType_real32,
                    MetaType_sim_entity_collision_volume_group,
                    MetaType_int32,
                    MetaType_hit_point,
                    MetaType_entity_reference,
                    MetaType_v2,
                    MetaType_world,
                    MetaType_world_position,
                    MetaType_rectangle2,
                    MetaType_rectangle3,
                    MetaType_sim_region,
                    MetaType_sim_entity,
                    MetaType_sim_entity_hash,
                    MetaType_sim_entity_collision_volume,
                };

                struct member_definition
                {
                    meta_type Type;
                    char *Name;
                    u32 Offset;
                };

as you can see, handmade_meta.h will work together with the handmade_generated.h 

we define the member_definition struct, and meta_type enum, this way the compiler can understand 
the content in handmade_generated.h



16:51
So now Casey will try to use this data in our debug system

so just as a proof of concept, Casey wrote the following test code 

                handmade_debug.cpp

                internal void DEBUGEnd()
                {

                    ...
                    ...

                    sim_entity TestEntity = {};
                    TestEntity.DistanceLimit = 10.0f;
                    TestEntity.tBob = 0.1f;
                    TestEntity.FacingDirection = 360.0f;
                    TestEntity.dAbsTileZ = 4;

                    DEBUGDumpStruct(ArrayCount(MembersOf_sim_entity), MembersOf_sim_entity, &TestEntity);

                    ...
                    ...
                } 


here is the DEBUGDumpStruct(); function

-   so we are literraly just printing out the values in the struct

-   notice that we pass in a void *StructPtr, this way we can take in any struct that we want.

-   notice how we retrive the memory of the member variable 

                void *MemberPtr = (((u8 *)StructPtr) + Member->Offset);

    also if it is a pointer, we do an extra level to dereference it 

                if(Member->Flags & MetaMemberFlag_IsPointer)
                {
                    MemberPtr = *(void **)MemberPtr;
                }


-   full code below:

                handmade_debug.cpp

                internal void DEBUGDumpStruct(u32 MemberCount, member_definition *MemberDefs, void *StructPtr, u32 IndentLevel = 0)
                {
                    for(u32 MemberIndex = 0; MemberIndex < MemberCount; ++MemberIndex)
                    {
                        char TextBufferBase[256];
                        char *TextBuffer = TextBufferBase;
                        for(u32 Indent = 0; Indent < IndentLevel; ++Indent)
                        {
                            *TextBuffer++ = ' ';
                            *TextBuffer++ = ' ';
                            *TextBuffer++ = ' ';
                            *TextBuffer++ = ' ';
                        }
                        TextBuffer[0] = 0;
                        size_t TextBufferLeft = (TextBufferBase + sizeof(TextBufferBase)) - TextBuffer;

                        
                        member_definition *Member = MemberDefs + MemberIndex;

                        void *MemberPtr = (((u8 *)StructPtr) + Member->Offset);
                        if(Member->Flags & MetaMemberFlag_IsPointer)
                        {
                            MemberPtr = *(void **)MemberPtr;
                        }

                        if(MemberPtr)
                        {
                            switch(Member->Type)
                            {
                                case MetaType_uint32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %u", Member->Name, *(u32 *)MemberPtr);
                                } break;

                                case MetaType_bool32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %u", Member->Name, *(b32 *)MemberPtr);
                                } break;

                                case MetaType_int32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %d", Member->Name, *(s32 *)MemberPtr);
                                } break;

                                case MetaType_real32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %f", Member->Name, *(r32 *)MemberPtr);
                                } break;

                                case MetaType_v2:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: {%f,%f}",
                                                Member->Name,
                                                ((v2 *)MemberPtr)->x,
                                                ((v2 *)MemberPtr)->y);
                                } break;

                                case MetaType_v3:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: {%f,%f,%f}",
                                                Member->Name,
                                                ((v3 *)MemberPtr)->x,
                                                ((v3 *)MemberPtr)->y,
                                                ((v3 *)MemberPtr)->z);
                                } break;

                                ...
                            }
                        }
                        
                        if(TextBuffer[0])
                        {
                            DEBUGTextLine(TextBufferBase);
                        }
                    }
                }

22:23
now we have the automated inspection 
and Casey was able to render it in the debug system



25:31 
Casey showing that this can work with other structs as well 

so Casey added the introspect keyword in front of the struct sim_region


                handmade_sim_region.h

                introspect(category:"regular butter") struct sim_region
                {
                    ...
                    ...
                };


then in the DEBUGEnd(); code we do DEBUGDumpStruct(); on both sim_entity and sim_region

                handmade_debug.cpp

                internal void DEBUGEnd()
                {
                    ...
                    ...

                    DEBUGTextLine("sim_entity:");
                    DEBUGDumpStruct(ArrayCount(MembersOf_sim_entity), MembersOf_sim_entity, &TestEntity);
                    DEBUGTextLine("sim_region:");
                    DEBUGDumpStruct(ArrayCount(MembersOf_sim_region), MembersOf_sim_region, &TestRegion);
                    
                    ...                        
                }




29:22
Casey made a change so that in simple_preprocessor.cpp, we go through all the files 


                int main(int ArgCount, char **Args)
                {
                    char *FileNames[] =
                    {
                        "handmade_sim_region.h",
                        "handmade_math.h",
                        "handmade_world.h",
                    };

                    for(int FileIndex = 0; FileIndex < (sizeof(FileNames)/sizeof(FileNames[0])); ++FileIndex)
                    {
                        char *FileContents = ReadEntireFileIntoMemoryAndNullTerminate(FileNames[FileIndex]);

                        ...
                        ...
                    }                    
                }

28:09
Casey does mention that if handmade_generated.h generates a new MetaType that wasnt included in the handmade_meta.h,
we have to manually add that type in handmade_generated.h. 

so this is one annoying part

but the there is another annoying part.

so take the example of MembersOf_sim_region               

                member_definition MembersOf_sim_region[] = 
                {
                    ...
                    ...
                    {MetaType_rectangle3, "Bounds", (u32)&((sim_region *)0)->Bounds},
                    {MetaType_rectangle3, "UpdatableBounds", (u32)&((sim_region *)0)->UpdatableBounds},
                    ...
                }

in the DEBUGDumpStruct(); we dont actually want to manully write the code to add the print 
statement for MetaType_rectangle2, MetaType_rectangle3 or for any other type 

                internal void DEBUGDumpStruct(u32 MemberCount, member_definition *MemberDefs, void *StructPtr, u32 IndentLevel = 0)
                {
                    for(u32 MemberIndex = 0; MemberIndex < MemberCount; ++MemberIndex)
                    {
                        ...
                        ...

                        if(MemberPtr)
                        {
                            switch(Member->Type)
                            {
                                case MetaType_uint32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %u", Member->Name, *(u32 *)MemberPtr);
                                } break;

                                case MetaType_bool32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %u", Member->Name, *(b32 *)MemberPtr);
                                } break;

                                case MetaType_int32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %d", Member->Name, *(s32 *)MemberPtr);
                                } break;

                                case MetaType_real32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %f", Member->Name, *(r32 *)MemberPtr);
                                } break;

                                ...
                            }
                        }
                    }
                }


so one thing you can do is to just do call DEBUGDumpStruct(); on that struct.
so you will have something like. This way we avoid writing the _snprintf_s(); for specific types.       

                handmade_debug.cpp

                internal void DEBUGDumpStruct(u32 MemberCount, member_definition *MemberDefs, void *StructPtr, u32 IndentLevel = 0)
                {
                    for(u32 MemberIndex = 0; MemberIndex < MemberCount; ++MemberIndex)
                    {
                        ...
                        ...

                        if(MemberPtr)
                        {
                            switch(Member->Type)
                            {
                                case MetaType_uint32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %u", Member->Name, *(u32 *)MemberPtr);
                                } break;

                                case MetaType_bool32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %u", Member->Name, *(b32 *)MemberPtr);
                                } break;

                                ...
                                ...

            ------------->      case MetaType_rectangle3:
                                {
                                    DEBUGDumpStruct(ArrayCount(MembersOf_rectangle3), MembersOf_rectangle3, MemberPtr);
                                } break;

                                ...
                            }
                        }
                    }
                }


Essentially, you will hard code and define the base types, such as uint32, bool, int32, real32.
but if there are any struct, you just call DEBUGDumpStruct on that struct

however, that still requires us to write 

                case MetaType_rectangle3:
                {
                    DEBUGDumpStruct(ArrayCount(MembersOf_rectangle3), MembersOf_rectangle3, MemberPtr);
                } break;

and we dont want to do that since we are lazy


39:00
so one thing to solve that is to have code generate all the cases for us.
in the simple_preprocessor, we actuall have all the types. so what we can do is to 
have the simple_preprocessor.cpp generate all the case for us.

so here we defined a linked list structure. 

                simple_preprocessor.cpp

                struct meta_struct
                {
                    char *Name;
                    meta_struct *Next;
                };
                static meta_struct *FirstMetaStruct;


then everytime we parse the struct, I will remember that struct
we literaly put it in our linked list 

                simple_preprocessor.cpp

                static void ParseStruct(tokenizer *Tokenizer)
                {
                    token NameToken = GetToken(Tokenizer);
                    if(RequireToken(Tokenizer, Token_OpenBrace))
                    {
                        printf("member_definition MembersOf_%.*s[] = \n", NameToken.TextLength, NameToken.Text);
                        printf("{\n");
                        for(;;)
                        {
                            token MemberToken = GetToken(Tokenizer);
                            if(MemberToken.Type == Token_CloseBrace)
                            {
                                break;
                            }
                            else
                            {
                                ParseMember(Tokenizer, NameToken, MemberToken);
                            }
                        }
                        printf("};\n");

        ----------->    meta_struct *Meta = (meta_struct *)malloc(sizeof(meta_struct));
                        Meta->Name = (char *)malloc(NameToken.TextLength + 1);
                        memcpy(Meta->Name, NameToken.Text, NameToken.TextLength);
                        Meta->Name[NameToken.TextLength] = 0;
                        Meta->Next = FirstMetaStruct;
                        FirstMetaStruct = Meta;
                    }
                }


41:58
so now at the end of our entire program, 
we just manually generate code that defines a #define 

                simple_preprocessor.cpp

                int main(int ArgCount, char **Args)
                {
                    char *FileNames[] =
                    {
                        "handmade_sim_region.h",
                        "handmade_math.h",
                        "handmade_world.h",
                    };
                    for(int FileIndex = 0;
                        FileIndex < (sizeof(FileNames)/sizeof(FileNames[0]));
                        ++FileIndex)
                    {
                        char *FileContents = ReadEntireFileIntoMemoryAndNullTerminate(FileNames[FileIndex]);

                        tokenizer Tokenizer = {};
                        Tokenizer.At = FileContents;

                        ........................................
                        ....... Parsing Logic ..................
                        ........................................
                    }

                    printf("#define META_HANDLE_TYPE_DUMP(MemberPtr, NextIndentLevel) \\\n");    
                    for(meta_struct *Meta = FirstMetaStruct;
                        Meta;
                        Meta = Meta->Next)
                    {
                        printf("    case MetaType_%s: {DEBUGTextLine(Member->Name); DEBUGDumpStruct(ArrayCount(MembersOf_%s), MembersOf_%s, MemberPtr, (NextIndentLevel));} break; %s\n",
                               Meta->Name, Meta->Name, Meta->Name,
                               Meta->Next ? "\\" : "");
                    }
                }

44:33
now if you look at the generated code in handmade_generated.h, we have this giant #define 
at the end

                handmade_generated.h


                member_definition MembersOf_sim_entity[] = 
                {
                   {MetaMemberFlag_IsPointer, MetaType_world_chunk, "OldChunk", (u32)&((sim_entity *)0)->OldChunk},
                   {0, MetaType_uint32, "StorageIndex", (u32)&((sim_entity *)0)->StorageIndex},
                   {0, MetaType_bool32, "Updatable", (u32)&((sim_entity *)0)->Updatable},
                   ...
                   ...
                }

                ...
                ...

                member_definition MembersOf_world_position[] = 
                {
                   {0, MetaType_int32, "ChunkX", (u32)&((world_position *)0)->ChunkX},
                   {0, MetaType_int32, "ChunkY", (u32)&((world_position *)0)->ChunkY},
                   {0, MetaType_int32, "ChunkZ", (u32)&((world_position *)0)->ChunkZ},
                   {0, MetaType_v3, "Offset_", (u32)&((world_position *)0)->Offset_},
                };

    ------>     #define META_HANDLE_TYPE_DUMP(MemberPtr, NextIndentLevel) \
                    case MetaType_world_position: {DEBUGTextLine(Member->Name); DEBUGDumpStruct(ArrayCount(MembersOf_world_position), MembersOf_world_position, MemberPtr, (NextIndentLevel));} break; \
                    case MetaType_rectangle3: {DEBUGTextLine(Member->Name); DEBUGDumpStruct(ArrayCount(MembersOf_rectangle3), MembersOf_rectangle3, MemberPtr, (NextIndentLevel));} break; \
                    case MetaType_rectangle2: {DEBUGTextLine(Member->Name); DEBUGDumpStruct(ArrayCount(MembersOf_rectangle2), MembersOf_rectangle2, MemberPtr, (NextIndentLevel));} break; \
                    case MetaType_sim_region: {DEBUGTextLine(Member->Name); DEBUGDumpStruct(ArrayCount(MembersOf_sim_region), MembersOf_sim_region, MemberPtr, (NextIndentLevel));} break; \
                    case MetaType_sim_entity: {DEBUGTextLine(Member->Name); DEBUGDumpStruct(ArrayCount(MembersOf_sim_entity), MembersOf_sim_entity, MemberPtr, (NextIndentLevel));} break; \
                    case MetaType_sim_entity_collision_volume_group: {DEBUGTextLine(Member->Name); DEBUGDumpStruct(ArrayCount(MembersOf_sim_entity_collision_volume_group), MembersOf_sim_entity_collision_volume_group, MemberPtr, (NextIndentLevel));} break; \
                    case MetaType_sim_entity_collision_volume: {DEBUGTextLine(Member->Name); DEBUGDumpStruct(ArrayCount(MembersOf_sim_entity_collision_volume), MembersOf_sim_entity_collision_volume, MemberPtr, (NextIndentLevel));} break; 


44:53
Casey also mentioned that if you are going hog wire on this, you can have it spit out all the enum_type in handmade_meta.h

                handmade_meta.h

                enum meta_type
                {
                    MetaType_world_chunk,
                    MetaType_uint32,
                    MetaType_bool32,
                    MetaType_entity_type,
                    MetaType_v3,
                    MetaType_real32,
                    MetaType_sim_entity_collision_volume_group,
                    MetaType_int32,
                    MetaType_hit_point,
                    MetaType_entity_reference,
                    MetaType_v2,
                    MetaType_world,
                    MetaType_world_position,
                    MetaType_rectangle2,
                    MetaType_rectangle3,
                    MetaType_sim_region,
                    MetaType_sim_entity,
                    MetaType_sim_entity_hash,
                    MetaType_sim_entity_collision_volume,
                };


45:35
so now in handmade_debug.cpp we can just put that #define in and that will handle all of our special cases 
                
                handmade_debug.cpp

                internal void DEBUGDumpStruct(u32 MemberCount, member_definition *MemberDefs, void *StructPtr, u32 IndentLevel = 0)
                {
                    for(u32 MemberIndex = 0; MemberIndex < MemberCount; ++MemberIndex)
                    {
                        ...
                        ...

                        if(MemberPtr)
                        {
                            switch(Member->Type)
                            {
                                case MetaType_uint32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %u", Member->Name, *(u32 *)MemberPtr);
                                } break;

                                case MetaType_bool32:
                                {
                                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, "%s: %u", Member->Name, *(b32 *)MemberPtr);
                                } break;

                                ...
                                ...
                                META_HANDLE_TYPE_DUMP();

                                ...
                            }
                        }
                    }
                }

58:58
Casey will not add support for pointers 

so casey first a enum member_definition_flag 

                handmade_meta.h

                enum member_definition_flag
                {
                    MetaMemberFlag_IsPointer = 0x1,
                };

then in our generated code, we would add the flags for everything 

                
                handmade_generated.h

                member_definition MembersOf_sim_entity_collision_volume[] = 
                {
                   {0, MetaType_v3, "OffsetP", (u32)&((sim_entity_collision_volume *)0)->OffsetP},
                   {0, MetaType_v3, "Dim", (u32)&((sim_entity_collision_volume *)0)->Dim},
                };
                member_definition MembersOf_sim_entity_collision_volume_group[] = 
                {
                   {0, MetaType_sim_entity_collision_volume, "TotalVolume", (u32)&((sim_entity_collision_volume_group *)0)->TotalVolume},
                   {0, MetaType_uint32, "VolumeCount", (u32)&((sim_entity_collision_volume_group *)0)->VolumeCount},
                   {MetaMemberFlag_IsPointer, MetaType_sim_entity_collision_volume, "Volumes", (u32)&((sim_entity_collision_volume_group *)0)->Volumes},
                };

                ...
                ...

1:03:40
so currently the way we handle pointers is very hacky.

for example, it doesnt handle pointer to pointers 

also it cant distinguish between array or just a pointer

if you look at the sim_entity_collision_volume_group

                handmade_sim_region.h

                introspect(category:"sim_region") struct sim_entity_collision_volume_group
                {
                    sim_entity_collision_volume TotalVolume;

                    // TODO(casey): VolumeCount is always expected to be greater than 0 if the entity
                    // has any volume... in the future, this could be compressed if necessary to say
                    // that the VolumeCount can be 0 if the TotalVolume should be used as the only
                    // collision volume for the entity.
                    uint32 VolumeCount;
                    sim_entity_collision_volume *Volumes; 
                };



when we debug, it will only print out the first value. We currently dont interpret it as an array. so we ignore the VolumeCount.


so one way we can do is to put mark ups 


                handmade_sim_region.h

                introspect(category:"sim_region") struct sim_entity_collision_volume_group
                {
                    sim_entity_collision_volume TotalVolume;

                    // TODO(casey): VolumeCount is always expected to be greater than 0 if the entity
                    // has any volume... in the future, this could be compressed if necessary to say
                    // that the VolumeCount can be 0 if the TotalVolume should be used as the only
                    // collision volume for the entity.
                    uint32 VolumeCount;
                    counted_pointer(params) sim_entity_collision_volume *Volumes; 
                };

something along the lines of that.



1:05:37
so if you want to take this metaprogramming concept to the next level, from this point forward, Instead of this simple_preprocessor.cpp being 
kind of a hack, what we would actually want to do is to build a an abstract syntax tree or data structure.
essentially we build up something that would represent all the information that we parsed. Then we do a bunch of operations on it 
in that meta form, and then we would spit it out. 

Q/A

1:08:25
someone recommended the C++ "offsetof" function
http://www.cplusplus.com/reference/cstddef/offsetof/

actually later at 1:15:48
Casey showed that the "offsetof" function in the C runtime library is exactly the code that he wrote 

                (size_t) (  (ptrdiff_t)&reinterpret_cast<const volatile char&>((((s *)0)->m)) )
                                                                                    
                                                                                    ^
                                                                                    |
                                                                                    |

so there is no point in using this function cuz you are introducing a header file dependency for exactly the same code 
that Casey just wrote



1:11:11
Casey actually dont know that other than basic arithmatic that isnt better in metaprogramming. 



