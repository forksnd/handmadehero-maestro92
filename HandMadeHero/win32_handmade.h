#ifndef WIN32_HANDMADE_H_
#define WIN32_HANDMADE_H_


struct win32_offscreen_buffer
{
    // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
};


struct win32_window_dimension
{
    int width;
    int height;
};

struct win32_sound_output
{
    int samplesPerSecond;
    uint32 runningSampleIndex;
    int bytesPerSample;
    int secondaryBufferSize;
    real32 tSine;
    int latencySampleCount;
};

#endif
