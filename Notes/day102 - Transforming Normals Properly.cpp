Handmade Hero Day 102 - Transforming Normals Properly

Summary:
attempted to explain why you need the inverse transpose of your model matrix for your normals.
He offers an intuitive explanation, but I still prefer stack overflows formal proof.

lots of debugging

Keyword:
Normal Matrix. Rendering



4:01 
attempts to explain how to transform normals from another angle 


				N = perp ( p1 - p0 )


10:48
assuming a scale matrix S. the reason why you cant just use S on the normal matrix is becuz 
as you increase S.x, it is actually the y axis of the normal that it is affecting



				N = perp ( p1 - p0 )

				  = perp ( V )
				  
				  = |	-V.y 	|
				  	|	 V.x	|



assume our scale matrix is 
				
					| 2 0 |
					| 0 1 |

so our new normal should be 

                N_ = perp( V_ )
                
                   = |  -V.y   |
                     |  2V.x   |
 
as you can see, it is the y axis that is elongated now


13:15
pretty much becuz the normal is the perpendicular vector of p1 - p0, so when you apply a scale matrix,
they are affected by a perpendicular way.

I personaly prefer some answers from this stack overflow post  
https://stackoverflow.com/questions/13654401/why-transforming-normals-with-the-transpose-of-the-inverse-of-the-modelview-matr

the answer by wcochran is my favourite

one thing about his answer though, i will write M^-T as  transpose(M^-1);
that way it is more clear



14:40
we only want to treat the scaling differently for the normal



18:42
although our vector has three components, we do not allow z to rotate. z is the one point out into the screen.
we are only rotating x and y.



did a shit ton of debugging and tweaking. Nothing very conclusive. Nothing too productive..






1:26:57
one neat trick he did to help debug is to color the texels that he is sampling from on the top plane texture


