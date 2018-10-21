Handmade Hero Day 096 - Introduction to Lighting


Summary:
talks about how difficult accurate lighting in Computer graphics is for the first 30 mins.
talks about the physcis behind lighting.

talks about what are diffuse, glossy and reflective surfaces

talked about the two majors problems of lighting in 2D games

mentioned a bunch of rendering concept: volumetric scattering, translucent surface..

recommended a book on lighting 

Physics Based Rendering: From Theory to Implementation
https://www.pbrt.org/


Keyword:
Rendering, Lighting



11:36
Casey quoting Dough Church, the guy who wrote Ultimate Underworld:

				"Lighting is the sound of graphics"


21:28 
the reason why we use RGB is becuz our eye actually only has receptors for Red, Green, and Blue
in your Retina you have sensors for Red, Green and Blue

26:40
accurate lighting is probably the hardest thing to do in computer graphics

27:36
talks about sub-surface scattering




32:57
typically we have diffuse, glossy and reflective surfaces

diffuse surfaces is something like chalk, something does not have a bright spot
if a light comes in, it just scatters in all direction. it "diffuses" the incoming light

highly reflective surfaces are like mirrors. Nothing scatters, light comes right in, then right out

glossy surface is somewhere in between

where when incoming light comes in, most of it goes towards outgoing reflection. Some of it gets scattered


38:42
mentioned "Brigade", a realtime raytracing renderer, not something you can use in a game engine yet

mentioned the "light probes" technique. you can also understand it as indirect light sampling.

the idea is that can we build some system, which compute effectively like look up tables which allows us 
a fast way to compute all of the contributions of the light that is coming in to a surface.


For example, 
for the cube map technique, where you look up texture value in the from the cubemap,
that is essentially a look up table for all the light coming from one pixel point







44:38
what are the problems of doing lighting in 2D?
1.	we dont know what the surfaces are. In 3D you always know.
	
one solution is to have a normal map for our 2D sprites


2.	we dont know what the light field is.

in 3D, you can place a camera anywhere and calculate lighting or used that for sampling

in 2D, we cant really place a camera in the scene, and rendering things in various directions to see what the lighting is 

so we need to come up with something that allows us to produce a light field that we can sample.

One solution is to use point lights. If worst comes to worst, point lights is all what we will do

we might be able to do something better, by doing "light rendering"
the idea is that we add a pre-render pass, (maybe donwscaled by 10x)
for example if you currently at 1920 x 1080, we might have our pre-render pass at 192 x 108

or maybe the closest power of 2 of that value 256 x 128

50:45 
so the idea is that we figure out a way to render the light sources in some kind of useful way 
into our render texture




1:12:15
mentioned an example of transparent vs translucent surface

an example of indirect light transport



1:25:30
Ambient Occlusion

it is baked into the sprites fairly well. its not a real thing, it is an effect 

ambient means: light from all direction
occlusion means: to block


1:33:17
recommended a book on lighting 

Physics Based Rendering: From Theory to Implementation
https://www.pbrt.org/



1:37:13 
volumetric scattering
