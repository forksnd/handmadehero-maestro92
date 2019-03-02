@echo off

echo TODAY WE ARE IN THE MIDDLE OF REDOING SOME CODE, SO THIS BUILD DOES NOT COMPILE.
echo INTENTIONALLY.
echo THANK YOU,
echo - THE MANAGEMENT

exit

set CommonCompilerFlags=-Od -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7
set CommonCompilerFlags=-DHANDMADE_STREAMING=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 %CommonCompilerFlags% 
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib

REM TODO - can we just build both with one exe?

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

del *.pdb > NUL 2> NUL

REM Simple preprocessor
cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS ..\handmade\code\simple_preprocessor.cpp /link %CommonLinkerFlags%
pushd ..\handmade\code
..\..\build\simple_preprocessor.exe > handmade_generated.h
popd

REM Asset file builder build
REM cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS ..\handmade\code\test_asset_builder.cpp /link %CommonLinkerFlags%

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
REM Optimization switches /wO2
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% -O2 -I..\iaca-win64\ -c ..\handmade\code\handmade_optimized.cpp -Fohandmade_optimized.obj -LD
cl %CommonCompilerFlags% -I..\iaca-win64\ ..\handmade\code\handmade.cpp handmade_optimized.obj -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender -EXPORT:DEBUGGameFrameEnd
del lock.tmp
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags%
popd
