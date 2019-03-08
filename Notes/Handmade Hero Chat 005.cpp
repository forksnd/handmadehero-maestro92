Handmade Hero Chat 005


0:28
what is SGX

this is Intel_s new thing, which is essentially DRM^2

So SGX is Intel Software Guard Extensions.
https://en.wikipedia.org/wiki/Software_Guard_Extensions


is a set of security-related instruction codes that are built into some modern intel CPUs


how do you make unbreakable DRM (Digital Rights Management);?

so the way things currently work on a CPU is that 

lets say we have a CPU, and it talks to a network card (NIC, the network interface card);
and and we purchase some game from Valve/steam.

    
     ___________        ___________                             ___________
    |           |      |           |       Internet            |           |
    |   CPU     |----->|   NIC     |-------------------------->|  Valve    |
    |___________|      |___________|                           |___________|


then Valve sends something back. our CPU handles the response, and writes
the game executable to your hardrive

     ___________        ___________                             ___________
    |           |      |           |       Internet            |           |
    |   CPU     |<-----|   NIC     |<--------------------------|  Valve    |
    |___________|      |___________|                           |___________|
            |
            |                ___________
            --------------->|           |
                            |   HD      |
                            |           |
                            |           |
                            |-----------|
                            |   exe     |
                            |___________|


lets assume Valve thinks DRM is a good idea. 
in reality Valve takes a neutral stance on it, which Casey commends them for. 
Valve does not force DRM on their customers, its the publisher_s decision. 
when you are steam, the publisher decides whether something has DRM or not. 

what can Valve do is that, every instance of a transaction, we knokw that the CPU, the NIC and the harddrive 
is on a user_s machine, and we know thate user has a steam id. 

and when the user sends a request it will most likely have to include your steam Id 
     ___________        ___________                             ___________
    |           |      |           |       Internet            |           |
    |   CPU     |<-----|   NIC     |<--------------------------|  Valve    |
    |___________|      |___________|                           |___________|
            |
            |                ___________
            --------------->|           |
                            |   HD      |
                            |           |
                            |           |
                            |-----------|
                            |   exe     |
                            |___________|


so when valve sends the game back, it can have the steamID baked into it in some way.

this is reasonable, but obviously if you are cracker, its not impossible to crack it. 

for instance, the cracker can observer multiple transactions and see how the executable sent back 
by valve is changed, and you can undo that change 


7:53
so how do we change this so we can prevent cracker cracking.
so what we do is we modify the diagram a little bit 

previously, our weakness is the steamId (that is assuming valve is using the steamId to encode the executable);
so we are treating our steamId is our key. 


so what we do now is to employ some public private key encryption


9:45
on a side note, Casey explaining what public key encryption is 

the idea is that you got two keys, a public key and a private key.

the idea is that the public key is out in the open. Anyone can gain access to the public key.
and anyone with a public key can encrypt a secret, but only people with a private key can decrypt the secret. 
     _____________
    | public key  |
    |_____________|
    | private key |
    |_____________|

unless you hack the encryption algorithm, which mathmatically speaking, requires brute force. 




so inside the CPU we store a private key. and it is backed into the CPU so that literally nobody can access it. 
[for instance like a hardware model number?, or some number deep in the CPU or something. essentially really something 
    that no one can access.]; 

its built into the circuitry of the CPU, and the only way you can ever get it out is to do some crazy hardware hacking 
on that CPU. Intel technology is very very very very good. so its impossible to do it. 

and we also have a corresponding public key

     ___________
    | public    |
    | key       |---------------------------------------->
    |___________|
     ___________        ___________                             ___________
    |           |      |           |       Internet            |           |
    |   CPU     |<-----|   NIC     |<--------------------------|  Valve    |
    |___________|      |___________|                           |___________|
    | private   |
    | key       |
    |___________|
    |___________|
            |                ___________
            --------------->|           |
                            |   HD      |
                            |           |
                            |           |
                            |-----------|
                            |   exe     |
                            |___________|



so now when we talk to Valve, when Valve asks for us our public key. 
Valve encrypt the package with that public key. 

and they send it back, and the CPU writes the encrypted game into the harddrive 

recall that no one can possbily decrypt this game. 
so the only guy who can decrypt this game is the CPU right now. So then you use Intel_s SGX
instruction set to load and execute the encrypted package. 

and it uses the private key that is in the CPU to decrypt the game code and its accompanying data, 
and the CPU is set up that no one can ever observe it. 

thats what the SGX extensions allows you to do. all of the memory is protected, no one can actually access it. 
so its completely isolated. so this creates "unbreakable DRM" in a very specific sense. 

so its not software "breakable", meaning no one will ever be able to break it in software, which is what people commonly do now. 

what will you have to do is to reverse engineer a specific CPU, with a game on it, and then you would do this process. 
and that will require a hardware lab that is too expensive that no one has access to. 

so will NSA, China be able to crack them? Yes. will regular hackers do? probably not. 



18:50
after reading the intel specs, it seems like. they sign the public key themselves with a key that only intel has. 
so intel can verify that intel has signed that key. 
     ___________        ___________                              ___________
    |           |      |           |       Internet             |           |
    |   CPU     |<-----|   NIC     |<-------------------------- | INTEL HQ  |
    |___________|      |___________|                            |___________|
    | private   |                                               | private   |
    | key       |                                               | key       |
    |___________|                                               |___________|
    | public    |                                               | public    |
    | key       |                                               | key       |
    |___________|                                               |___________|
            |                ___________
            --------------->|           |
                            |   HD      |
                            |           |
                            |           |
                            |-----------|
                            |   exe     |
                            |___________|


so right now in this diagram, the public key is also a random public key, its a signed public key, which means at intel HQ,
they have a private key, and a public key 


22:47
Casey summarizing the steps.

1:  we send public key to EA 
2:  EA encrypts simcity 12
3:  we Receive encrypted simcity 12, and store on on harddrive
-----------------------------------------------
4:  CPU encrypts and runs the game using pricate key 


so in step 1, when you send the public key to EA, they know not only the public key is valid cuz its signed by them.
this public key can be verified by EA, as one that actaully came from an Intel CPU.
so Emulation wont help cracking this. and emulator will have a private and public key that intel didnt sign.


34:22
the problem with this approach is that it takes control away from the user of the computer. 