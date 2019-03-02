Handmade Hero Day 238 - Making OpenGL Use Our Screen Coordinates

Summary:
mentioned that we will refactor the code to have the software rendering path as well as the OpenGL rendering path 
wrote the openGL version of the RenderGroupToOutput(); function

derived our projection matrix in our RenderGroupToOutput(); function.

Keyword:
OpenGL


10:01
Casey mentioned that currently in our renderer, we want to be able to support swapping between software renderer and OpenGL.

but even if you dont want to swap between software renderer and OpenGL, you may want to swap between two hardware paths,
lets say OpenGL and vulcan



10:55
if you are a professional game programmer, you have to got the habit of keeping the renderer separated out, even before
you get the the point when you really need to 

So Casey will now try to extract out/refactor rendering code, so that we can choose whether we want to render hardware or software 


12:33
so if you look at the function RenderGroupToOutput(); This is where all the rendering time is spent.
Pretty much any code that determines what the code looks like on screen goes through this function. 

so if we want to render to a piece of hardware, instead of rendering through our software renderer,
really what we need to do is to produce a copy of this function, that instead of dispatching to our software renderer,
we dispatches to OpenGL.

                handmade_render_group.cpp

                internal void RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget, rectangle2i ClipRect)
                {
                    IGNORED_TIMED_FUNCTION();

                    u32 SortEntryCount = RenderGroup->PushBufferElementCount;
                    tile_sort_entry *SortEntries = (tile_sort_entry *)(RenderGroup->PushBufferBase + RenderGroup->SortEntryAt);
                    
                    real32 NullPixelsToMeters = 1.0f;

                    tile_sort_entry *Entry = SortEntries;
                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)
                            (RenderGroup->PushBufferBase + Entry->PushBufferOffset);
                        
                        void *Data = (uint8 *)Header + sizeof(*Header);
                        switch(Header->Type)
                        {
                            case RenderGroupEntryType_render_entry_clear:
                            {
                                ...
                            } break;

                            case RenderGroupEntryType_render_entry_bitmap:
                            {
                                ...
                            } break;

                            ...
                            ...

                            InvalidDefaultCase;
                        }
                    }
                }


13:43
so Casey created a new file called handmade_opengl.cpp
and we created the OpenGL version of RenderGroupToOutput(); 
                
all the code is pretty straight-forward. 

                internal void RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget, rectangle2i ClipRect)
                {
                    IGNORED_TIMED_FUNCTION();

                    glEnable(GL_TEXTURE_2D);

                    glViewport(0, 0, OutputTarget->WindowWidth, OutputTarget->WindowHeight);

                    glMatrixMode(GL_TEXTURE);
                    glLoadIdentity();
                    
                    glMatrixMode(GL_MODELVIEW);
                    glLoadIdentity();

                    glMatrixMode(GL_PROJECTION);
                    r32 a = SafeRatio1(2.0f, (r32)OutputTarget->WindowWidth);
                    r32 b = SafeRatio1(2.0f, (r32)OutputTarget->WindowHeight);
                    r32 Proj[] =
                    {
                         a,  0,  0,  0,
                         0,  b,  0,  0,
                         0,  0,  1,  0,
                        -1, -1,  0,  1,
                    };
                    glLoadMatrixf(Proj);

                    u32 SortEntryCount = RenderGroup->PushBufferElementCount;
                    tile_sort_entry *SortEntries = (tile_sort_entry *)(RenderGroup->PushBufferBase + RenderGroup->SortEntryAt);

                    tile_sort_entry *Entry = SortEntries;
                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)
                            (RenderGroup->PushBufferBase + Entry->PushBufferOffset);
                        
                        void *Data = (uint8 *)Header + sizeof(*Header);
                        switch(Header->Type)
                        {
                            case RenderGroupEntryType_render_entry_clear:
                            {
                                render_entry_clear *Entry = (render_entry_clear *)Data;
                    
                                glClearColor(Entry->Color.r, Entry->Color.g, Entry->Color.b, Entry->Color.a);
                                glClear(GL_COLOR_BUFFER_BIT);
                            } break;

                            case RenderGroupEntryType_render_entry_bitmap:
                            {
                                render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                                Assert(Entry->Bitmap);

                                v2 XAxis = {1, 0};
                                v2 YAxis = {0, 1};
                                v2 MinP = Entry->P;
                                v2 MaxP = MinP + Entry->Size.x*XAxis + Entry->Size.y*YAxis;
                                    
                                OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                            } break;

                            case RenderGroupEntryType_render_entry_rectangle:
                            {
                                render_entry_rectangle *Entry = (render_entry_rectangle *)Data;
                                OpenGLRectangle(Entry->P, Entry->P + Entry->Dim, Entry->Color);
                            } break;

                            case RenderGroupEntryType_render_entry_coordinate_system:
                            {
                                render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Data;
                            } break;

                            InvalidDefaultCase;
                        }
                    }
                }



16:32
so in our software renderer, in the RenderGroupEntryType_render_entry_bitmap path, we have 
this is gonna be the one that is somewhat hard to convert to opengl becuz we dont have the notion of getting our textures
down to the card.

we will solve this problem when we come to it.

                case RenderGroupEntryType_render_entry_bitmap:
                {
                    render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                    Assert(Entry->Bitmap);

                    v2 XAxis = {1, 0};
                    v2 YAxis = {0, 1};
                    DrawRectangleQuickly(OutputTarget, Entry->P,
                                         Entry->Size.x*XAxis,
                                         Entry->Size.y*YAxis, Entry->Color,
                                         Entry->Bitmap, NullPixelsToMeters, ClipRect);
                } break;






20:10
then we define the OpenGLRectangle(); function below:

of course we will change this later on to the OpenGL programmable path.

                handmade_opengl.cpp

                inline void OpenGLRectangle(v4 MinP, v4 MaxP, v4 Color)
                {                    
                    glBegin(GL_TRIANGLES);

                    glColor4f(Color.r, Color.g, Color.b, Color.a);
                    
                    // NOTE(casey): Lower triangle
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex2f(MinP.x, MinP.y);

                    glTexCoord2f(1.0f, 0.0f);
                    glVertex2f(MaxP.x, MinP.y);
                    
                    glTexCoord2f(1.0f, 1.0f);
                    glVertex2f(MaxP.x, MaxP.y);

                    // NOTE(casey): Upper triangle
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex2f(MinP.x, MinP.y);

                    glTexCoord2f(1.0f, 1.0f);
                    glVertex2f(MaxP.x, MaxP.y);

                    glTexCoord2f(0.0f, 1.0f);
                    glVertex2f(MinP.x, MaxP.y);
                    
                    glEnd();
                }


25:24
So we wrote the handmade_opengl file, but we arent currently using it yet.

Casey went back to the win32_handmade.cpp file and fix the projection matrix. 

what we want to do with the projection matrix is to accomplish the following

     ______________                          __________________
    | screen space | ---------------------> | OpenGL Unit cube |
    |______________|    projection          |__________________|
                        matrix 



recall the math 

        P_clip = M_proj * M_modelView * P;

our M_modelView is just the identity matrix 

so all we are doing is 

        P_clip = M_proj * P;



so the big question is what is our Projection matrix



recall OpenGL renders into a unit cube 


                                               (1, 1)
                 _______________________________
                |               |               |
                |               |               |
                |               |               |
                |               |               |
                |               |               |
                |               |               |
                |_______________|_______________|
                |               |               |
                |               |               |
                |               |               |
                |               |               |
                |               |               |
                |               |               |
                |_______________|_______________|

            (-1, -1)


but we are currently rendering it into the range of [0, width]

so we want to map 0 to -1, and width to 1

[notice how this is kind of dumb, becuz opengl will literally do
clipped space ----> NDC space ----> window space
so essentially reversing our mapping. 

but we are doing this mapping to shoehorn it into OpenGL Rendering pipeline]



33:17
so our set up is 


    | x1|   | a  b  0  e |   | x |
    | y1| = | c  d  0  f | * | y |
    | 0 |   | 0  0  1  0 |   | 0 |
    | 1 |   | 0  0  0  1 |   | 1 |


        x1 = ax + by + e
        y1 = cx + dy + f


since we dont have rotation, we can ignore b and c

    | x1|   | a  0  0  e |   | x |
    | y1| = | 0  d  0  f | * | y |
    | 0 |   | 0  0  1  0 |   | 0 |
    | 1 |   | 0  0  0  1 |   | 1 |


        x1 = ax + e
        y1 = dy + f

so for the x1 function, essentially we have
        
        f(0) = -1; 
        f(width) = 1;

so e = -1, and f = -1;

a = 2/width
d = 2/height 



so our projection matrix is 


    | x1|   | 2/width   0           0       e |   | x |
    | y1| = | 0         2/height    0       f | * | y |
    | 0 |   | 0         0           1       0 |   | 0 |
    | 1 |   | 0         0           0       1 |   | 1 |



44:24
Casey introduces the call glLoadMatrix();
https://docs.microsoft.com/en-us/windows/desktop/opengl/glloadmatrixf


we could have did 

                r32 Proj[] =
                {
                     2/width,   0,          0,  -1,
                     0,         2/height,   0,  -1,
                     0,         0,          1,  0,
                     0,         0,          0,  1,
                };

but just to be safe, since there isnt anything that is saying that width or height can be zero,
so Casey did 

                r32 a = SafeRatio1(2.0f, (r32)OutputTarget->WindowWidth);
                r32 b = SafeRatio1(2.0f, (r32)OutputTarget->WindowHeight);
                r32 Proj[] =
                {
                     a,  0,  0,  -1,
                     0,  b,  0,  -1,
                     0,  0,  1,  0,
                     0,  0,  0,  1,
                };



48:16
so one other thing about this projection matrix is the position of -1,


so usually in our math book, we would think that our matrix is 
                r32 Proj[] =
                {
                     a,  0,  0,  -1,
                     0,  b,  0,  -1,
                     0,  0,  1,  0,
                     0,  0,  0,  1,
                };

but this is incorrect due to an encoding issue. 
so in our classroom math we have the concept of viewing a matrix by column or by rows 

for example [notice these two matrix multiplication give you the same answer]

        | a b c d |   | a |
        | e f g h | * | b |
        | i j k l |   | c |
        | m n o p |   | d |


                        | a e i m | 
        | a b c d |  *  | b f j n | 
                        | c g k o | 
                        | d h l p | 

so depending on the what matrix multiplication, we will view the matrix either by row or by column


but in computers, we have a problem, cuz if we define something like below, that is just an array
they are just continuious integers in memory. there is no rigid rule inside the computer about which way 
you are specifyinig your computer 

so when you are telling OpenGL you want a matrix * vector, it doesnt know if you have 

        | a b c d |   | a |
        | e f g h | * | b |
        | i j k l |   | c |
        | m n o p |   | d |

or doing  

                        | a e i m | 
        | a b c d |  *  | b f j n | 
                        | c g k o | 
                        | d h l p | 



so in OpenGL, they consider multiplication by columns. 
so we have:

                r32 Proj[] =
                {
                     a,  0,     0,  0,
                     0,  b,     0,  0,
                     0,  0,     1,  0,
                     -1,  -1,   0,  1,
                };

that is just OpenGL convention.
Actually in the OpenGL specs, we have 

                a0  a4  a8   a12 
                a1  a5  a9   a13 
                a2  a6  a10  a14
                a3  a7  a11  a15 




-   as you can see below: we set up our projection matrix like below:

                win32_handmade.cpp

                internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext, int WindowWidth, int WindowHeight)
                {
                    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    glMatrixMode(GL_TEXTURE);
                    glLoadIdentity();
                    
                    glMatrixMode(GL_MODELVIEW);
                    glLoadIdentity();

                    glMatrixMode(GL_PROJECTION);
                    r32 a = SafeRatio1(2.0f, (r32)Buffer->Width);
                    r32 b = SafeRatio1(2.0f, (r32)Buffer->Height);
                    r32 Proj[] =
                    {
                         a,  0,  0,  0,
                         0,  b,  0,  0,
                         0,  0,  1,  0,
                        -1, -1,  0,  1,
                    };
                    glLoadMatrixf(Proj);

                    ...
                    ...
                }


Q/A
57:27
will handmade_opengl.cpp be included in the platform specific layer or game layer?

it will be game layer. it wont be in the platform-specific layer becuz it can be shared between multiple
implementations of openGL. Like OpenGL code for mac vs OpenGL code for linux is 99% the same

so typically you having the handmade_opengl thing as a separate platform tier, you dont want to use any platform 
stuff there, that way you can reuse it. 


1:01:45
regarding our push buffer in our software renderer, in 2D game when you know you would want to sort
for transparency reasons, you have to do this "deferred" style. You have to buffer all the push draw commands 
and render later. 

1:05:39
do we use any element buffer or vertex buffer in OpenGL. If no, why?

we will be using vertex buffer, the function call overhead of doing glVertex call is too much.


1:09:17
about glOrtho