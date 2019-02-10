Handmade Hero Day 012 - Platform-independent Sound Output

Summary:
describes the difference between the demands of outputting sound vs the demands of outputting video in a game

Keyword:
audio, sound


5:03
sound is tricker since sound is temporal.


6:00
audio is mandatorily temporal. Audio happens over time. So when you are listneing to the audio output of a game. Its not optional
how long it will be playing for. It has to play continuously.

if I have 1/8 second worth of music, and i let it play for 1/16 second, the user is gonna hear garbage for the other 1/16 second.

in contrast to video, if I play the image to the screen, I can sort of let it play as long as I want (in some sense);
if I leave the same image on for a long time, for the user, its just telling them that my frame rate is very low. 
it wont appear to them that the game has a serious glitch.  

essentially video is just a static snapshot that gets held for a 1/30 or 1/60. 
sound doesnt have that option. you basically have to fill up all the time. If you dont fill up sound, your brain 
immediatley hears the error.

the glitch is very immediate, it will tell the brain that its a flat out bug. 

when we prepare the image for the frame, we really dont have to worry about how long that frame will be on the screen. 

with sound I have to say I want the sound that starts at a particular point in time, and I need this much of it. 
if you miss judge, you will get a glitch(or a gap of playing garbage sound data);




12:52 
Casey proceeds to write the first draft of the usage code for sound 

the GameOutputSound(); function takes two arguments:

-	the number of samples worth of sound do you want me to output
-	the buffer that you want me to output to.

				handmade.cpp

				internal void 
				GameUpdateAndRender(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
				{
					GameOutputSound(SampleCountToOutput, SoundBuffer);
					RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
				}













23:10
sound is just not very big

48000 samples per second for sound
60 frames / second 

each frame 800 samples. Each sample is 2 bytes long

1600 bytes per second 

1600 * 2 cuz there are two of them for stereo

3200 bytes.

so 3200 is literally nothing


1:05:30
we dont know going into a game, what is gonna be feasible, frame rate wise

1:17：08
allocate is not a function call, it is intrisic to the compiler




