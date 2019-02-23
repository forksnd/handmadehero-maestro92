Handmade Hero Day 231 - Order Notation

Summary:
lecture on big O notation, P and NP

Keyword:
computer science, theory


20:02
we do care about the constant term come implementation time 
but if we want to know how something scales, we dont care about the constant term.

the big O notation is the answer to the scaling question
the performance question is more complicated, and that involves the constant term


often times, when people throw around the term of big O notation for algorithm, they are 
referring to the worst case.

so the worst case is like describing "what we can gurantee" for this algorithm



28:48
Casey explainw what P = NP is 

P means polynomial
NP means non deterministic polynomial

polynomial means: an expression of more than two algebraic terms, especially the sum of several terms that contain 
different powers of the same variables 


so polynomial is something like:

n^c0 + n^c1 + n^c2 + n^c3 ...

its always N to the power of something. that is just how polynomial is defined 
so something is like 

                n^12 + 3*n^5 + 10*n^3



32:26
so if an alogrithm is NP, that means its order notation is something like O(2 ^ n);
usually means, the N is in the expononent


34:23
so Casey graphed n^2 and 2^n side by side for comparison

essentially P is trackable
NP is not trackable  


36:00
so back to P = NP

we dont really know how to classify algorithms/problems into these two categories. 

there are a lot of practicel problems, where we dont know how to classify it 
for example the "3sat" problem

https://en.wikipedia.org/wiki/Boolean_satisfiability_problem


essentially given a bunch of boolean inputs, we want to know what boolean input values produces a true 


so lets say you have an expression:

    a and not (b and a) and (not c and d);

here we have a,b,c,d as inputs, so n = 4

you would think that you can solve this in polynomial time, but you quickly find out that you couldnt 

lets take the brute force approach, each input has two possible values: True or false.
so you would find out that this would be 2 * 2 * 2 * 2, which is 2 ^ n, which is NP 


so taking the brute force approach, we would think that this is NP. But scientist have yet to figure out that if that this is 
actually the case. We cant prove yet, whether ther exists a more clever way, that doesnt take NP. 


40:44
so this 3sat problem is just an example of a "NP complete" problem. Essentially its a type of problem where nobody has been 
able to prove that it takes polynomial time to solve (no one has come up with anyway to solve it); But they have not prove
that you cant solve it in polynomial time. 

And they have proven that you can verify the answer in polynomial time. 

for example for the 3 sat problem, if you have a list of inputs, I can verify whether that produces makes the original expression
produce an true value.

so "NP-Complete" problems has a "P verifier". It has the ability to verify the solution with polynomial time. 



42:30
so "NP-Complete" are problems that no one knows whether its P or NP. but we do know they can be verified in P time.

So the number of problems that fall into this category are fairly large, and people have done tremendous amount of work proving
they are equivalent. 


44:37
whats also interesting is that people have done work proving that all "NO-complete" problems are the same. Meaning if you have found 
a P solution for one of the problems, you can transform all the other ones and solve it in P  



45:36
dont assume a problem is "NP complete" becuz something is hard. For example, the traveling sales man problem

it is not "NP complete". It is actually harder than that since there is no P verifier for it. There is no way to verify 
this problem in anything other than a "NP" verifier 