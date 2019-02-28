Handmade Hero Day 237 - Displaying an Image with OpenGL

Summary:
explained how OpenGL transformations work.

use OpenGL to render our pixel buffer to screen in the Win32DisplayBufferInWindow(); function.
he did this using OpenGL fixed immediate pipeline
he rendered the screen as a texture quad, loading our pixel buffer as a texture.

explained how opengl textures work. Mentioned the concept of OpenGL Binding textures.

explained how some of the OpenGL funciton works, including 
glTexImage2d(); 
glTexParameteri();

Keyword:
OpenGL


5:04
so right now we want to tell OpenGL to draw a rectangle.
there are two ways. we either draw to triangles, 


                 _________________________
                |__                       |    
                |  |___                   |            
                |      |___               |            
                |          |___           |              
                |              |__        |              
                |                 |___    |                
                |                     |___|              
                |_________________________|

or draw a giant triangle and clip that triangle to a rectangle          
                 ___
                |   |___
                |       |____ 
                |            |___
                |                |________ 
                |_________________________|_____
                |                         |     |__
                |                         |        |_____    
                |                         |              |____
                |                         |                   |____
                |                         |                        |_____
                |                         |                              |__
                |                         |                                 |
                |_________________________|_________________________________|

OpenGL has ways of clipping things.
this method is called sciccoring, and openGL has a function called glScissor(); 



the two triangle = rectangle is the more general interesting case becuz this method would allow you to draw 
rectangles anywhere you want without having to reset any clipping mode, which you have to do for the 2nd method.

depending on the graphics card, resetting clipping mode could be an expensive operation.

so we will focus on the how you would general draw quads. 




8:15 
so casey will now write code to render quads in OpenGL in old school, fixed function mode (which is deprecated); 

all pretty straight forward stuff

                win32_handmade.cpp

                internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, 
                    HDC DeviceContext, int WindowWidth, int WindowHeight)
                {
                    glViewport(0, 0, WindowWidth, WindowHeight);

                    glEnable(GL_TEXTURE_2D);
                    
                    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    glMatrixMode(GL_TEXTURE);
                    glLoadIdentity();
                    
                    glMatrixMode(GL_MODELVIEW);
                    glLoadIdentity();

                    glMatrixMode(GL_PROJECTION);
                    glLoadIdentity();
                    
                    glBegin(GL_TRIANGLES);

                    r32 P = 1.0f;

                    // NOTE(casey): Lower triangle
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex2f(-P, -P);

                    glTexCoord2f(1.0f, 0.0f);
                    glVertex2f(P, -P);
                    
                    glTexCoord2f(1.0f, 1.0f);
                    glVertex2f(P, P);

                    // NOTE(casey): Upper triangle
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex2f(-P, -P);

                    glTexCoord2f(1.0f, 1.0f);
                    glVertex2f(P, P);

                    glTexCoord2f(0.0f, 1.0f);
                    glVertex2f(-P, P);
                    
                    glEnd();
                    
                    SwapBuffers(DeviceContext);
                }


14:14
so casey explaining the differece between fixed function pipeline vs programmable pipeline.
the programmable pipeline is using shader
 
fixed function pipeline came when GPU werent programmable


Casey went on to explain all the matrix transformation
Object coordinates ---> eye coordinates ---> clip coordinates ---> NDC coordinates ---> window coordinates

this is just how OpenGL defines it 


44:29
Casey says that glTexImage2d(); is one of the worst API ever devised
https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml


3D card dont do mipmapping properly. they never have.

Casey goes on to explain what the "internal format" and "format" argument is for in the glTexImage2d(); function call


"format" 
is for how we are submitting our pixels.
so this essentially means, the order of our RGB its 


"type"
The size of each components 
so "format" specifies the order, "type" specifies the size

notice that Casye is using the spec from the msdn site
https://docs.microsoft.com/en-us/windows/desktop/opengl/glteximage2d

the thing about the "type" argument is that, the spec says 
                    
    "The data type of the pixel data. The following symbolic values are accepted: 
    GL_UNSIGNED_BYTE, GL_BYTE, GL_BITMAP, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_INT, GL_INT, and GL_FLOAT."
    

so we are specifying the type of data for the "const GLvoid  *pixels" data.
since it is taking in a GLvoid* (effectively a void*); data, we need to specify the type of that.
while we pixel data is structured in the format of BGRA, it is still just bytes of data. It still just an compact 
array of bytes. Therefore we pass in GL_UNSIGNED_BYTE



"internal format"
that is asking us, how do we want OpenGL to store the pixels, which is really just a suggest. OpenGL doesnt have to store
it this way. 

-   



so Casey wrote the following code to specify the texture in OpenGL

                win32_handmade.cpp

                internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, 
                    HDC DeviceContext, int WindowWidth, int WindowHeight)
                {
                    glViewport(0, 0, WindowWidth, WindowHeight);

                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Buffer->Width, Buffer->Height, 0,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Buffer->Memory);

                    ...
                    ...

                    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    ...
                    ...
                    
                    SwapBuffers(DeviceContext);
                }

50:30
so when we first wrote the code above, it doesnt work. 
when we submit a texture to OpenGL driver through the glTexImage2d(); function

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Buffer->Width, Buffer->Height, 0,
                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, Buffer->Memory);


what happens is that OpenGL driver will snap a copy of "Buffer->Memory" into its own memory, and then later 
will kick of a transfer to the GPU card(this mental model of this was explained in day 236); 

then we will call all the glTexParameteri(); and glVertex2f(); which will build up the command buffer, and the OpenGL
driver will transfer it to the GPU, some later time.

again, none of these functions may get executed at the exact time of us calling them.
its all sort of us telling the driver, what do we want to be done, and the driver will do whatever it wants to make that happen. 



51:50
back to the question. Why dont we see anything?
first we have to enable texture. so we call 

                glEnable(GL_TEXTURE_2D);

-   full code below:

                win32_handmade.cpp

                internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, 
                    HDC DeviceContext, int WindowWidth, int WindowHeight)
                {
                    glViewport(0, 0, WindowWidth, WindowHeight);
                    
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Buffer->Width, Buffer->Height, 0,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Buffer->Memory);

                    ...
                    ...

    ----------->    glEnable(GL_TEXTURE_2D);
                    
                    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    ...
                    ...
                    
                    SwapBuffers(DeviceContext);
                }





52:20
so enabling it is still not enough, 
so one thing OpenGL can have is, we can have more than one texture. 
like in games, you see multiple textures.

so one thing that OpenGL wants to be able to do is to store multiple textures on the card,
so as we draw things, we can select which texture do we want to use. 

although glTexImage2d(); submits a texture to the card, 
OpenGL has this concept of what texture is bound. The idea is that you need to create slots that name the texture that you will submit,
so that you can refer to them later.

essentially, we need to create handles to our textures, and let OpenGL know that you are binding them 

so to get a texture handle, we just ask OpenGL for one.

so we first define a handle 

                win32_handmade.cpp

                global_variable GLuint GlobalBlitTextureHandle;


then we create it in our Win32InitOpenGL(); we ask one from OpenGL through the glGenTextures(); function
the glGenTextures(); is like giving us a pointer

                internal void Win32InitOpenGL(HWND Window)
                {
                    HDC WindowDC = GetDC(Window);

                    ...
                    ...
                    HGLRC OpenGLRC = wglCreateContext(WindowDC);
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        // NOTE(casey): Success!!!
                        glGenTextures(1, &GlobalBlitTextureHandle);
                    }
                    else
                    {
                        InvalidCodePath;
                        // TODO(casey): Diagnostic
                    }
                    ReleaseDC(Window, WindowDC);
                }



then in the rendering code, we bind 

                internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, 
                    HDC DeviceContext, int WindowWidth, int WindowHeight)
                {
                    glViewport(0, 0, WindowWidth, WindowHeight);
                    
    ----------->    glBindTexture(GL_TEXTURE_2D, GlobalBlitTextureHandle);

                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Buffer->Width, Buffer->Height, 0,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Buffer->Memory);

                    ...
                    ...

                    glEnable(GL_TEXTURE_2D);
                    
                    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    ...
                    ...
                    
                    SwapBuffers(DeviceContext);
                }




55:50
so even after all of that, we still wont see our buffer->memory.
what we still need to do is we need to setup the rules for how the texture is sampled and applied to our image. 

-   so we have to first setup the texture environment 
    what this allows us to do is to specify how we want our texture to affect the color of the pixels that we are drawing.

                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    GL_MODULATE means mulitply, and the reason why we would want that is it means, when we set colors, whatever 
    our texture actually has in it, it will just get mulitplied by those colors. 

    essentially it says, when you sample a texture, and you have an incoming color, multiply the two. 

-   one thing you notice in glTexImage2d(); is that while the function took in Buffer->Width and Buffer->Height
    as arguments, we didnt specify is the stride or packing is on the "Buffer->Memory" pointer.

    if you recall our "Buffer->Memory" pointer is bottom up, and it has all kinds of attributes.

    so later on, we can let OpenGL know how OpenGL is expecting the data in "Buffer->Memory".
    there are some function we can call to set that up. 

    in this episode, we got lucky, in which we didnt have to specify anything extra about Buffer->Memory
    and OpenGL is reading it exactly how we packed Buffer->Memory, and we were able to render our Buffer->Memory
    on screen


-   the other thing we need to do is glTexParameterf();
        
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    

    so for GL_TEXTURE_MIN_FILTER, it is asking when you are shrinking the image down, how do you want to render it?
    there are options for bilinear filtering. but for now we are just gonna pick GL_NEAREST.



                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    

    although we use u,v, to talk about textures
    OpenGL use the term s and t to talk about textures after they have been transformed



-   full code below:


                internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, 
                    HDC DeviceContext, int WindowWidth, int WindowHeight)
                {
                    glViewport(0, 0, WindowWidth, WindowHeight);
                    
                    glBindTexture(GL_TEXTURE_2D, GlobalBlitTextureHandle);

                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Buffer->Width, Buffer->Height, 0,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Buffer->Memory);
                    
    ----------->    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

                    glEnable(GL_TEXTURE_2D);
                    
                    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    ...
                    ...
                    
                    SwapBuffers(DeviceContext);
                }


Q/A
1:23:18
Casey explaining the difference between glTexImage2d(); vs glTexSubImage2D(); if you want to 
replace the whole image.

    glTexImage2d(); ramps over the entire texture. so it just replaces everything in the image. 
     _______________
    |               |
    |               |
    |               |
    |               |
    |               |
    |_______________|

    glTexSubImage2D(); replaces just a piece. so the rest of the texture stays the same
     _______________
    |               |
    |    _____      |
    |   |     |     |
    |   |_____|     |
    |               |
    |_______________|


what happens in glTexImage2d();, is that, imagine the CPU and GPU 

when I call glTexImage2d(); the driver copies the pixel data and issues a copy to the GPU
and that creates a dependency chain 

so lets say for frame 0, our swap buffers call depends on the Begin/End call, which then depends onthe glTexImage2d(); call 
then when you get to frame 1, you do some settings to your rendering call 

so visually, you have: [0 means for frame 0]


    Texture 0           Texture 1
        |                   |
        v                   v
    Begin/End 0         Begin/End 1
        |                   |
        v                   v
    Swap Buffers 0      Swap Buffers 1

essentially, the OpenGL driver can issues these two calls in complete parallel




what happens if you have glTexSubImage2D, your dependency chain looks like 



    Texture 0             ---- Texture Sub 1  
        |                 |         | 
        v                 |         v
    Begin/End 0           |    Begin/End 1
        |                 |         |
        v                 |         v
    Swap Buffers 0   <-----    Swap Buffers 1



so for frame 1, it wont use a separate texture, and it has to wait for SwapBuffers 0 to finish becuz 
SwapBuffers 0 is using Texture 0


but in the end, we have to time it or ask an actual driver person to tell us 