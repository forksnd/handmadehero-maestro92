Handmade Hero Day 203 - Debug UI State Caching

Summary:
continuing on the work from day 201 and day 202

Keyword:
debug system



continuing on day 201 and day 202, doing lots of debugging


Just to recap, we have three things going on here 

we have debug_variable is the data, which is just the data.
debug_view is the UI rendering side of it.
keep these two decoupled.



[so i have quite a bit of confusion here

the tree structure that Im used to is. since we always see this on Leetcode

                struct TreeNode
                {
                    int value;
                    TreeNode* left;
                    TreeNode* right;
                };
or with something more general 

                struct TreeNode
                {
                    int value;
                    vector<TreeNode*> children;
                };



but what Casey have here is something like 

                struct TreeNode
                {
                    union
                    {
                        int value;
                        vector<TreeNode*> children;
                    }
                };

this is useful for cases like 

                { {3,4}, 2 }


so if you have somethingl ike { {3,4}, 2 }, and you try to use the 

                struct TreeNode
                {
                    int value;
                    vector<TreeNode*> children;
                };

struct to represent this hierarchy, you will waste memory. So you use a union. 
so to use this version to represent { {3,4}, 2 } 
we would have. If you noticed only leaf nodes will have data
data only lives in the leaf nodes

                 ________________
                |  root          |
                |  Node1, Node2  |
                |___/_______\____|
                   /         \
                  /           \
         ______________      _________
        |              |    |         |
        | Node3, Node4 |    |   2     |
        |__/_______\___|    |_________|
          /         \
         /           \
     ____            ____
    | 3  |          |  4 |
    |____|          |____|


but again Casey here uses a linked list instead of a vector
so previously if we have a C++ vector, memory wise we usually think of them as everything compact in memory 

assume you have {3, 4, 5, 6, 7}

                Root 
                 _______________________________________
                |       |       |       |       |       |
                |   |   |   |   |   |   |   |   |   |   |
                |___|___|___|___|___|___|___|___|___|___|
                    |       |       |       |       |
                    |       |       |       |       |
                    v       v       v       v       v
                   ____    ____    ____    ____    ____  
                  | 3  |  | 4  |  | 5  |  | 6  |  | 7  | 
                  |____|  |____|  |____|  |____|  |____|


but now with a linked list, you can still think of them as compact in memory, it doesnt really matter if they are
a vector or a linked lst. Its just that each linked list node uses more memory

                struct TreeNode
                {
                    int value;
                    LinkedListNode* list;
                };


                struct LinkedListNode
                {
                    LinkedListNode* prev;
                    LinkedListNode* next;
                    TreeNode* treeNode;
                };


you can think of them as compact in memory 

                Root 
                 ________________________________________________________________
                |            |            |            |            |            |
                |   next   --|->  next  --|->  next  --|->  next  --|->  next ---|---> ...
                |            |            |            |            |            |
                |   value    |   value    |   value    |   value    |   value    | 
                |____|_______|____|_______|____|_______|____|_______|____|_______|
                     |            |            |            |            |
                     v            v            v            v            v
                    ____         ____         ____         ____         ____  
                   | 3  |       | 4  |       | 5  |       | 6  |       | 7  | 
                   |____|       |____|       |____|       |____|       |____|


so if you think of all the LinkedListNode memory are compact, these two are effectively the same
so if you want to print the value of the first child, you would do the following in the two cases.

                    TreeNode.children[0].value;
                    TreeNode.list->treeNode->value;
]

so converting from what I described above to Casey_s code, you have something 

                struct debug_variable_link
                {
                    debug_variable_link *Next;
                    debug_variable_link *Prev;
                    debug_variable *Var;
                };

                struct debug_variable
                {
                    debug_variable_type Type;
                    char *Name;

                    union
                    {
                        b32 Bool32;
                        s32 Int32;
                        u32 UInt32;
                        r32 Real32;
                        v2 Vector2;
                        v3 Vector3;
                        v4 Vector4;
                        debug_profile_settings Profile;
                        debug_bitmap_display BitmapDisplay;
                        debug_variable_link VarGroup;
                    };
                };


meaning debug_variable is our TreeNode struct, which contains both data and the hierarchy
debug_variable_link, is our LinkedListNode struct

[I prevously had the massive confusion about whether debug_variable is just data or actually a tree node.
    now I understand that debug_variable is a treeNode]







27:13
so in the DEBUGDrawMainMenu(); we are iterating through our debug_variable nodes [from here im gonna call them debug_variable nodes,
so I can personally understand it]

so we give each debug_variable node the concept of a debug_view. (I would call it debug_variable_node_view to be more explicit);

so here we use the pointer pointing to the node and the debug_tree pointer (I personally feel the entire debug_tree struct is redundant, anything 
Casey is using the debug_tree for can be accomplished with his debug_variable_node struct);

but you can see that we are retrieving the debugView for the node. [Why not just assing Ids to each node?]

                handmade_debug.cpp

                internal void DEBUGDrawMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    for(debug_tree *Tree = DebugState->TreeSentinel.Next;
                        Tree != &DebugState->TreeSentinel;
                        Tree = Tree->Next)
                    {
                        ...
                        ...
                        while(Depth > 0)
                        {
                            debug_variable_iterator *Iter = Stack + (Depth - 1);
                            if(Iter->Link == Iter->Sentinel)
                            {
                                --Depth;
                            }
                            else
                            {
                                Layout.Depth = Depth;
                                
                                debug_variable_link *Link = Iter->Link;
                                debug_variable *Var = Iter->Link->Var;
                                Iter->Link = Iter->Link->Next;

                                ...
                                ...

                                debug_view *View = GetOrCreateDebugViewFor(DebugState, DebugIDFromLink(Tree, Link));

                            }
                        }
                    }
                }


here we see the GetOrCreateDebugViewFor(); function.
recall there are two version of hashtable: the linkedlist version or non-linkedlist version 
we are doing in the LinkedList version of hashtable (opening chaning); hence the NextInHash variable

                internal debug_view * GetOrCreateDebugViewFor(debug_state *DebugState, debug_id ID)
                {
                    // TODO(casey): Better hash function
                    u32 HashIndex = (((u32)ID.Value[0] >> 2) + ((u32)ID.Value[1] >> 2)) % ArrayCount(DebugState->ViewHash);
                    debug_view **HashSlot = DebugState->ViewHash + HashIndex;

                    debug_view *Result = 0;
                    for(debug_view *Search = *HashSlot; Search; Search = Search->NextInHash)
                    {
                        if(DebugIDsAreEqual(Search->ID, ID))
                        {
                            Result = Search;
                            break;
                        }
                    }
                    
                    if(!Result)
                    {
                        Result = PushStruct(&DebugState->DebugArena, debug_view);
                        Result->ID = ID;
                        Result->Type = DebugViewType_Unknown;
                        Result->NextInHash = *HashSlot;
                        *HashSlot = Result;
                    }
                    
                    return(Result);
                }

and here we have the DebugIDFromLink.
[I would just use numerical ID]

                inline debug_id
                DebugIDFromLink(debug_tree *Tree, debug_variable_link *Link)
                {
                    debug_id Result = {};

                    Result.Value[0] = Tree;
                    Result.Value[1] = Link;

                    return(Result);
                }


