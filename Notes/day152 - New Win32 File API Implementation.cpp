Handmade Hero Day 152 - New Win32 File API Implementation

Summary:

finished up the load_asset_work refactor for the LoadSound(); function

cleaned up some of the asset loading code.

implemented the windows file api 

internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin);
internal PLATFORM_GET_ALL_FILE_OF_TYPE_END(Win32GetAllFilesOfTypeEnd);
internal PLATFORM_OPEN_FILE(Win32OpenFile);
internal PLATFORM_FILE_ERROR(Win32FileError);
internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile);

talked about how API design works 

Keyword:
Asset, Asset file, Asset loading, File API 




10:42
as we refactored LoadBitmap(); to use load_asset_work in day 151,
Casey does the same refactor for LoadSound(); 



14:05
added a memory copy function();
a very brute force Implementation

                handmade.h

                inline void
                Copy(memory_index Size, void *SourceInit, void *DestInit)
                {
                    u8 *Source = (u8 *)SourceInit;
                    u8 *Dest = (u8 *)DestInit;
                    while(Size--) {*Dest++ = *Source++;}
                }

Casey made a comment that, usually when hes copying memory, he will have a bit more context.
for example, its aligned, or if he can copy in 128 bit chunks (if so, we can do SIMD); or not.
so this is a function he might not use too much. we dont care about performance of this code.

this is a throwaway version of a copy function



35:56
wrote the GetFileHandleFor(); function
it just literally looking up the file handle in our asset system

                handmade_asset.cpp

                inline platform_file_handle *
                GetFileHandleFor(game_assets *Assets, u32 FileIndex)
                {
                    Assert(FileIndex < Assets->FileCount);
                    platform_file_handle *Result = Assets->Files[FileIndex].Handle;
                    
                    return(Result);
                }


41:48
Casey went on to implement the win32_file API

                internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin);
                internal PLATFORM_GET_ALL_FILE_OF_TYPE_END(Win32GetAllFilesOfTypeEnd);
                internal PLATFORM_OPEN_FILE(Win32OpenFile);
                internal PLATFORM_FILE_ERROR(Win32FileError);
                internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile);


44:48
Casey first wrote 
-   Win32Handle is the kernel handle

                struct win32_platform_file_handle
                {
                    platform_file_handle H;
                    HANDLE Win32Handle;
                };




                internal PLATFORM_OPEN_FILE(Win32OpenFile)
                {
                    // TODO(casey): Actually implement this!
                    char *FileName = "test.hha";
                    
                    // TODO(casey): If we want, someday, make an actual arena used by Win32
                    win32_platform_file_handle *Result = (win32_platform_file_handle *)VirtualAlloc(
                        0, sizeof(win32_platform_file_handle),
                        MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                    if(Result)
                    {    
                        Result->Win32Handle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
                        Result->H.NoErrors = (Result->Win32Handle != INVALID_HANDLE_VALUE);
                    }

                    return((platform_file_handle *)Result);
                }



49:14
Casey doing the PLATFORM_READ_DATA_FROM_FILE

Caey introduced the concept of OVERLAPPED Overlapped in the windows ReadFile API.


-   53:46 apparently Windows can not do read bigger than 4 GB. Dont ask him why, Casey just know they cant.
    hence Casey did 

                uint32 FileSize32 = SafeTruncateUInt64(Size);


-   full code below:

                win32_handmade.cpp

                internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
                {
                    if(PlatformNoFileErrors(Source))
                    {
                        win32_platform_file_handle *Handle = (win32_platform_file_handle *)Source;
                        OVERLAPPED Overlapped = {};
                        Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
                        Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);
                    
                        uint32 FileSize32 = SafeTruncateUInt64(Size);
                    
                        DWORD BytesRead;
                        if(ReadFile(Handle->Win32Handle, Dest, FileSize32, &BytesRead, &Overlapped) &&
                           (FileSize32 == BytesRead))
                        {
                            // NOTE(casey): File read succeeded!
                        }
                        else
                        {
                            Win32FileError(&Handle->H, "Read file failed.");
                        }
                    }
                }



55:42
For PlatformNoFileErrors, right now we just out some debug strings,
and we set the NoErrors flag on.

                internal PLATFORM_FILE_ERROR(Win32FileError)
                {
                #if HANDMADE_INTERNAL
                    OutputDebugString("WIN32 FILE ERROR: ");
                    OutputDebugString(Message);
                    OutputDebugString("\n");
                #endif
                    
                    Handle->NoErrors = false;
                }


talked about how API design work in the entire Q/A