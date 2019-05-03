Handmade Hero Day 275 - Passing Rotation and Shear to the Renderer

Summary:
passing shear and rotation information to the renderer. Recovering rendering rotated and sheared sprites in the renderer

Keyword:
renderer 

23:34
Casey now wants to add more information to the renderer, such as rotaiton and shear
so currently we have the render_entry_bitmap struct:

                handmade_render_group.h

                struct render_entry_bitmap
                {
                    loaded_bitmap *Bitmap;
                    
                    v4 Color;
                    v2 P;
                    v2 Size;
                };


now we add the XAxis and YAxis information for Rotation

                struct render_entry_bitmap
                {
                    loaded_bitmap *Bitmap;
                    
                    v4 Color;
                    v2 P;
                    
                    // NOTE(casey): X and Y axes are already scaled by the dimension.
                    v2 XAxis;
                    v2 YAxis;
                };

and the rest of the stream is just Casey trying to recover rendering sheared and rotated sprites in the renderer

59:03
Casey show casing some results 