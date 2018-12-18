Handmade Hero Day 154 - Finding Asset Files with Win32

Summary:

pretty much a whole day about windows API.

wrote the PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(); function which loops through all the files 
using Window API.

wrote the PLATFORM_OPEN_FILE(); function which opens the file using Windows API as well.

mentioned that dont ever use STL in Q/A

mentioned the reason he does 
    
                #define PLATFORM_OPEN_FILE(name) platform_file_handle *name(platform_file_group *FileGroup)
                typedef PLATFORM_OPEN_FILE(platform_open_next_file);

style coding for the platform functions

Keyword:
windows API, 


6:02
Casey starting to work on the PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(); function.

Casey showing, how on windows, to iterate through directory.

Pretty much we have to use the FindFirstFileA windows API.

its pretty much about learning how to use windows API.

Casey also agrees that this is horrible API design by windows.



-   Casey told us that we have to use the FindFirstFileA(); windows API 
    this is kind of like a linked list type thing 

    as you can see in the specs 

    "If the function succeeds, the return value is a search handle used in a subsequent call to FindNextFile or FindClose,"
    https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstfilea

    so we first call "HANDLE FindHandle = FindFirstFileA(WildCard, &FindData);" to get a handle, then we will be calling 
    FindNextFile(); to get actual files.

    for the FindFirstFileA(); function call, we give it a "WildCard" string, and then we put the results in the FindData object


-   here we are constructing the WildCard string that we can pass in to the FindFirstFileA(); function

                    char *TypeAt = Type;
                    char WildCard[32] = "*.";
                    for(u32 WildCardIndex = 2; WildCardIndex < sizeof(WildCard); ++WildCardIndex)
                    {
                        WildCard[WildCardIndex] = *TypeAt;
                        if(*TypeAt == 0)
                        {
                            break;
                        }

                        ++TypeAt;
                    }
                    WildCard[sizeof(WildCard) - 1] = 0;

    notice that it is NULL terminated. So we added a '0' at the end.


-   in the the while loop, we are essentially counting the number of files.

                    WIN32_FIND_DATAA FindData;
                    HANDLE FindHandle = FindFirstFileA(WildCard, &FindData);
                    while(FindHandle != INVALID_HANDLE_VALUE)
                    {
                        ++Win32FileGroup->H.FileCount;
                        
                        if(!FindNextFileA(FindHandle, &FindData))
                        {
                            break;
                        }
                    }

    you can see that the main thing is incrementing "++Win32FileGroup->H.FileCount;"
    we do this until FileHandle becomes INVALID_HANDLE_VALUE


-   full code below:

                win32_handmade.cpp

                internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
                {
                    // TODO(casey): If we want, someday, make an actual arena used by Win32
                    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)VirtualAlloc(
                        0, sizeof(win32_platform_file_group),
                        MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                    char *TypeAt = Type;
                    char WildCard[32] = "*.";
                    for(u32 WildCardIndex = 2; WildCardIndex < sizeof(WildCard); ++WildCardIndex)
                    {
                        WildCard[WildCardIndex] = *TypeAt;
                        if(*TypeAt == 0)
                        {
                            break;
                        }

                        ++TypeAt;
                    }
                    WildCard[sizeof(WildCard) - 1] = 0;
                    
                    Win32FileGroup->H.FileCount = 0;

                    WIN32_FIND_DATAA FindData;
                    HANDLE FindHandle = FindFirstFileA(WildCard, &FindData);
                    while(FindHandle != INVALID_HANDLE_VALUE)
                    {
                        ++Win32FileGroup->H.FileCount;
                        
                        if(!FindNextFileA(FindHandle, &FindData))
                        {
                            break;
                        }
                    }
                    FindClose(FindHandle);

                    Win32FileGroup->FindHandle = FindFirstFileA(WildCard, &Win32FileGroup->FindData);
                    
                    return((platform_file_group *)Win32FileGroup);
                }



28:12
Casey also writing the PLATFORM_GET_ALL_FILE_OF_TYPE_END(); function
pretty much here, we just free the Win32FileGroup memory that we allocated.
we do this by calling VirtualFree(); and we passed the MEM_RELEASE flag.


                win32_handmade.cpp

                internal PLATFORM_GET_ALL_FILE_OF_TYPE_END(Win32GetAllFilesOfTypeEnd)
                {
                    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup;
                    if(Win32FileGroup)
                    {
                        FindClose(Win32FileGroup->FindHandle);

                        VirtualFree(Win32FileGroup, 0, MEM_RELEASE);
                    }
                }


32:25
Casey moving on to work on the PLATFORM_OPEN_FILE(); function
pretty much we are calling FindNextFileA(); again.

-   when we call CreateFileA();, we get the filename from the FindData.

                char *FileName = Win32FileGroup->FindData.cFileName;
                Result->Win32Handle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);



-   the FindNextFileA(); returns false, we terminate the the fileGroup iteration

                FindClose(Win32FileGroup->FindHandle);
                Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;


-   full code below:

                internal PLATFORM_OPEN_FILE(Win32OpenNextFile)
                {
                    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup;
                    win32_platform_file_handle *Result = 0;

                    if(Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
                    {    
                        // TODO(casey): If we want, someday, make an actual arena used by Win32
                        Result = (win32_platform_file_handle *)VirtualAlloc(
                            0, sizeof(win32_platform_file_handle),
                            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                        if(Result)
                        {
                            char *FileName = Win32FileGroup->FindData.cFileName;
                            Result->Win32Handle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
                            Result->H.NoErrors = (Result->Win32Handle != INVALID_HANDLE_VALUE);
                        }
                        
                        if(!FindNextFileA(Win32FileGroup->FindHandle, &Win32FileGroup->FindData))
                        {
                            FindClose(Win32FileGroup->FindHandle);
                            Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
                        }
                    }
                    
                    return((platform_file_handle *)Result);
                }


53:43
dont use the STL
the stl is very bad.
STL strings is one of the worst parts of it 

STL is architected very bad. anything you want to use something, it starts pulling in templates, which makes your code 
compile time to be gigantic.

Never ever ever use STL for production game dev code.


1:08:59
someone asked in the Q/A: why does Casey you #defines for platform functions such as PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN();

The reason is becuz we have to at least do a typedef cuz we store function pointers to them. 
For example:

                handmade_platform.h
    
                #define PLATFORM_OPEN_FILE(name) platform_file_handle *name(platform_file_group *FileGroup)
                typedef PLATFORM_OPEN_FILE(platform_open_next_file);


                typedef struct platform_api
                {
                    ...
                    ...

                    platform_open_next_file *OpenNextFile;

                    ...
                    ...

                } platform_api;


the platform_api needs to have a pointer to OpenNextFile.
platform_api is like a dispatch table.


the #define and typedef is 2 different things 
we are doing the typedef for the pointer reason 

the #define is strictly for convenience.
consider the case 


                handmade_platform.h
    
                #define PLATFORM_OPEN_FILE(name) platform_file_handle *name(platform_file_group *FileGroup)
                typedef PLATFORM_OPEN_FILE(platform_open_next_file);

and for the definition, we have 

                win32_handmade.cpp

                internal PLATFORM_OPEN_FILE(Win32OpenNextFile)
                {
                    ...
                    ...
                }



if casey wants to make a change, lets say we want to change platform_open_next_file to platform_open_next_file2
with this #define, we only need to change one place.


                handmade_platform.h
    
                #define PLATFORM_OPEN_FILE(name) platform_file_handle* name(platform_file_group *FileGroup)
                typedef PLATFORM_OPEN_FILE(platform_open_next_file2);


                win32_handmade.cpp

                internal PLATFORM_OPEN_FILE(Win32OpenNextFile)
                {
                    ...
                    ...
                }



without the #define, he will have to change two places 


                handmade_platform.h
    
                typedef platform_file_handle* platform_open_next_file2(platform_file_group *FileGroup)


                win32_handmade.cpp

                internal platform_open_next_file2 Win32OpenNextFile
                {
                    ...
                    ...
                }

for questions on typedef, see 
https://stackoverflow.com/questions/4295432/typedef-function-pointer




