/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "handmade_render_group.h"


#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_SRGB8_ALPHA8                   0x8C43

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

// NOTE(casey): Windows-specific
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

struct opengl_info
{
    b32 ModernContext;
    
    char *Vendor;
    char *Renderer;
    char *Version;
    char *ShadingLanguageVersion;
    char *Extensions;
    
    b32 GL_EXT_texture_sRGB;
    b32 GL_EXT_framebuffer_sRGB;
};

internal opengl_info
OpenGLGetInfo(b32 ModernContext)
{
    opengl_info Result = {};

    Result.ModernContext = ModernContext;
    Result.Vendor = (char *)glGetString(GL_VENDOR);
    Result.Renderer = (char *)glGetString(GL_RENDERER);
    Result.Version = (char *)glGetString(GL_VERSION);
    if(Result.ModernContext)
    {
        Result.ShadingLanguageVersion = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    }
    else
    {
        Result.ShadingLanguageVersion = "(none)";
    }
    
    Result.Extensions = (char *)glGetString(GL_EXTENSIONS);

    char *At = Result.Extensions;
    while(*At)
    {
        while(IsWhitespace(*At)) {++At;}
        char *End = At;
        while(*End && !IsWhitespace(*End)) {++End;}

        umm Count = End - At;        
        
        if(0) {}
        else if(StringsAreEqual(Count, At, "GL_EXT_texture_sRGB")) {Result.GL_EXT_texture_sRGB=true;}
        else if(StringsAreEqual(Count, At, "GL_EXT_framebuffer_sRGB")) {Result.GL_EXT_framebuffer_sRGB=true;}

        At = End;
    }
    
    return(Result);
}

internal void
OpenGLInit(b32 ModernContext)
{
    opengl_info Info = OpenGLGetInfo(ModernContext);
        
    OpenGLDefaultInternalTextureFormat = GL_RGBA8;
    if(Info.GL_EXT_texture_sRGB)
    {
        OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
    }

    // TODO(casey): Need to go back and use extended version of choose pixel format
    // to ensure that our framebuffer is marked as SRGB?
    if(Info.GL_EXT_framebuffer_sRGB)
    {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
}       

inline void
OpenGLSetScreenspace(s32 Width, s32 Height)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    r32 a = SafeRatio1(2.0f, (r32)Width);
    r32 b = SafeRatio1(2.0f, (r32)Height);
    r32 Proj[] =
    {
         a,  0,  0,  0,
         0,  b,  0,  0,
         0,  0,  1,  0,
        -1, -1,  0,  1,
    };
    glLoadMatrixf(Proj);
}

inline void
OpenGLRectangle(v2 MinP, v2 MaxP, v4 Color)
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

inline void
OpenGLDisplayBitmap(s32 Width, s32 Height, void *Memory, int Pitch,
                    s32 WindowWidth, s32 WindowHeight)
{
    Assert(Pitch == (Width*4));
    glViewport(0, 0, Width, Height);
    
    glBindTexture(GL_TEXTURE_2D, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Memory);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_TEXTURE_2D);
    
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    OpenGLSetScreenspace(Width, Height);

    // TODO(casey): Decide how we want to handle aspect ratio - black bars or crop?
    
    v2 MinP = {0, 0};
    v2 MaxP = {(r32)Width, (r32)Height};
    v4 Color = {1, 1, 1, 1};

    OpenGLRectangle(MinP, MaxP, Color);
}

// TODO(casey): Get rid of this
global_variable u32 TextureBindCount = 0;
internal void
OpenGLRenderCommands(game_render_commands *Commands, s32 WindowWidth, s32 WindowHeight)
{    
    glViewport(0, 0, Commands->Width, Commands->Height);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    OpenGLSetScreenspace(Commands->Width, Commands->Height);
    
    u32 SortEntryCount = Commands->PushBufferElementCount;
    tile_sort_entry *SortEntries = (tile_sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);

    tile_sort_entry *Entry = SortEntries;
    for(u32 SortEntryIndex = 0;
        SortEntryIndex < SortEntryCount;
        ++SortEntryIndex, ++Entry)
    {
        render_group_entry_header *Header = (render_group_entry_header *)
            (Commands->PushBufferBase + Entry->PushBufferOffset);
        
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

                if(Entry->Bitmap->Handle)
                {
                    glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);
                }
                else
                {
                    Entry->Bitmap->Handle = ++TextureBindCount;
                    glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);

                    glTexImage2D(GL_TEXTURE_2D, 0, OpenGLDefaultInternalTextureFormat, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);
    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                }
                
                OpenGLRectangle(Entry->P, MaxP, Entry->Color);
            } break;

            case RenderGroupEntryType_render_entry_rectangle:
            {
                render_entry_rectangle *Entry = (render_entry_rectangle *)Data;
                glDisable(GL_TEXTURE_2D);
                OpenGLRectangle(Entry->P, Entry->P + Entry->Dim, Entry->Color);
                glEnable(GL_TEXTURE_2D);
            } break;

            case RenderGroupEntryType_render_entry_coordinate_system:
            {
                render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Data;
            } break;

            InvalidDefaultCase;
        }
    }
}