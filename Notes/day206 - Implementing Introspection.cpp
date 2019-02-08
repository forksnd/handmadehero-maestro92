Handmade Hero Day 206 - Implementing Introspection

Summary:

following up on day 205, in which we wanted to add tech to examine entities in the debug system 
but mentioned that adding all the DEBUG_VALUE(); #defines is incredibily verbose 

so Casey added his own tech for introspection. 

added a "#define introspect"

added the "simple_preprocessor.cpp" file, and wrote code that parses the "handmade_sim_region.h" file

talked about how parsers work 

discussed what "Recursive Descent Parser" is in the Q/A

explained what introspection is in programming languages 

Keyword:
introspection, metaprogramming



3:31
recall in day 205, we wrote this bit of code to examine an entity in the debug system

                handmade.cpp 


                for(uint32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; ++VolumeIndex)
                {
                    sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        

                    v3 LocalMouseP = Unproject(RenderGroup, MouseP);


                    if((LocalMouseP.x > -0.5f*Volume->Dim.x) && (LocalMouseP.x < 0.5f*Volume->Dim.x) &&
                       (LocalMouseP.y > -0.5f*Volume->Dim.y) && (LocalMouseP.y < 0.5f*Volume->Dim.y))
                    {
                        v4 OutlineColor = V4(1, 1, 0, 1);
                        PushRectOutline(RenderGroup, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, OutlineColor, 0.05f);

                        DEBUG_BEGIN_HOT_ELEMENT(Entity);
                        DEBUG_VALUE(Entity->StorageIndex);
                        DEBUG_VALUE(Entity->Updatable);
                        DEBUG_VALUE(Entity->Type);
                        DEBUG_VALUE(Entity->P);
                        DEBUG_VALUE(Entity->dP);
                        DEBUG_VALUE(Entity->DistanceLimit);
                        DEBUG_VALUE(Entity->FacingDirection);
                        DEBUG_VALUE(Entity->tBob);
                        DEBUG_VALUE(Entity->dAbsTileZ);
                        DEBUG_VALUE(Entity->HitPointMax);
                        DEBUG_BEGIN_ARRAY(Entity->HitPoint);
                        for(u32 HitPointIndex = 0;
                            HitPointIndex < Entity->HitPointMax;
                            ++HitPointIndex)
                        {
                            DEBUG_VALUE(Entity->HitPoint[HitPointIndex]);
                        }
                        DEBUG_END_ARRAY();
                        DEBUG_VALUE(Entity->Sword);
                        DEBUG_VALUE(Entity->WalkableDim);
                        DEBUG_VALUE(Entity->WalkableHeight);
                        DEBUG_END_HOT_ELEMENT();
                    }
                }


which is a pain in the butt to write. So Casey is gonna try to implement introspection

4:27
So casey first created the simple_preprocessor.cpp file 
this is gonna be a utility file that will help us generate all this DEBUG_VALUE(); code 

the idea is that we are going to read in the handmade_sim_region.h file and we will preprocess it

                simple_preprocessor.cpp

                int main(int ArgCount, char **Args)
                {
                    char *FileContents = ReadEntireFileIntoMemoryAndNullTerminate("handmade_sim_region.h");

                    ...
                    ...
                }






7:56 
Casey will now write the ReadEntireFileIntoMemoryAndNullTerminate(); function.

we are literally allocating memory, load the entire file content into our memory allocated at Result
we do this by calling fread();

also notice that we did "Result[FileSize] = 0". That is just simply adding a null termination
                
                simple_preprocessor.cpp

                static char * ReadEntireFileIntoMemoryAndNullTerminate(char *FileName)
                {
                    char *Result = 0;
                    
                    FILE *File = fopen(FileName, "r");
                    if(File)
                    {
                        fseek(File, 0, SEEK_END);
                        size_t FileSize = ftell(File);
                        fseek(File, 0, SEEK_SET);

                        Result = (char *)malloc(FileSize + 1);
                        fread(Result, FileSize, 1, File);
                        Result[FileSize] = 0;
                        
                        fclose(File);
                    }

                    return(Result);
                }

10:39
Casey showed that you can use the Text Visualizer in the visual studio debugger to examine the content 
pointed at Result. 


11:16
Casey then mentioned that now we want to parse the "handmade_sim_region.h" file 
so to help with parsing, Casey will annotate the "handmade_sim_region.h" file.
This way, the parsing process will be easier

12:11
so in handmade.h, he added the introspect macro

                handmade.h 

                #define introspect(params)

this is just a macro that doesnt expand to anything. its literrally a macro that vanishes.

then in the struct sim_entity, we add this macro in front of it 

                handmade_sim_region.h

                introspect(category:"regular butter") struct sim_entity
                {
                    ...
                    ...
                };

since this macro doesnt expand to anything, the code will still compile.
This is gonna mainly be used for our simple_preprocessor


13:41
now back to our simple_preprocessor.cpp. in order to parse the file, we will 
start with the traditional parser structure. There are 2 phases.

1.  lexing

[from wikipedia, lexical analysis means 

In computer science, lexical analysis, lexing or tokenization is the process of 
converting a sequence of characters (such as in a computer program or web page) 
into a sequence of tokens (strings with an assigned and thus identified meaning)];

so in this case, we need to somehow recognize 'i' 'n' 't' 'r' 'o' 's' 'p' 'e' 'c' 't' means the "introspect" token

we also have the "category" token and so on. 

so the first thing we want to do is to write a thing that allows us to get tokens out of a file 

2.  [not mentioned... Casey skipped this part]



14:48
Casey will now write the code that gets tokens 

                simple_preprocessor.cpp

                struct token
                {
                    token_type Type;
                    
                    size_t TextLength;
                    char *Text;
                };


Casey also added the token_type enum

                simple_preprocessor.cpp

                enum token_type
                {
                    Token_Unknown,
                    
                    Token_OpenParen,    
                    Token_CloseParen,    
                    Token_Colon,    
                    Token_Semicolon,    
                    Token_Asterisk,    
                    Token_OpenBracket,    
                    Token_CloseBracket,    
                    Token_OpenBrace,    
                    Token_CloseBrace,

                    Token_String,    
                    Token_Identifier,    

                    Token_EndOfStream,
                };



16:24
now that we have defined the token_type enum and token struct, 
Casey wrote the general structure of our file 
            
pretty much we will keep on getting tokens until there are no tokens left. 
and we will do a switch on the token type 

                simple_preprocessor.cpp

                int main(int ArgCount, char **Args)
                {
                    char *FileContents = ReadEntireFileIntoMemoryAndNullTerminate("handmade_sim_region.h");

                    bool Parsing = true;
                    while(Parsing)
                    {
                        token Token = GetToken();
                        switch(Token.Type)
                        {
                            case Token_EndOfStream:
                                {
                                    Parsing = false;
                                }
                                break;

                            case ...:
                                break;
                            ...

                        }
                    }
                }


17:45
Casey showing us a printf trick, using the "precision" modifier
so the modifier is in the format of ".number", and has slightly different meanings for the different conversion specifiers

https://www.cprogramming.com/tutorial/printf-format-strings.html


also here we have 

"if the width specification is an asterisk '*', an int argument from the argulment list supplies the value.
The width argumet must precede the value thats being formatted in the argument list"

so if you have 
                printf("%0*f", 5, 3);

this outputs 
                00003

https://docs.microsoft.com/en-us/cpp/c-runtime-library/format-specification-syntax-printf-and-wprintf-functions?view=vs-2017
[just super complicated rules in c printf. not worth remembering it]


anyways, so initially Casey has


                token Token = GetToken();
                printf("%d: %s\n", Token.Type, Token.Text)

but printf is null terminated, so this will keep on printing till forever, so we have to specify how many characters to print

                token Token = GetToken();
                printf("%d: %.*s\n", Token.Type, Token.TextLength, Token.Text);


[i dont understand... doesnt really matter. Its just understanding different rules of printf]...

20:19
Next Casey added the tokenizer struct 

                simple_preprocessor.cpp

                struct tokenizer
                {
                    char *At;
                };

pretty much its a pointer to remember where we are at in reading the file 

                simple_preprocessor.cpp

                int main(int ArgCount, char **Args)
                {
                    char *FileContents = ReadEntireFileIntoMemoryAndNullTerminate("handmade_sim_region.h");

                    tokenizer Tokenizer = {};
                    Tokenizer.At = FileContents;

                    bool Parsing = true;
                    while(Parsing)
                    {
                        token Token = GetToken(&Tokenizer);
                        switch(Token.Type)
                        {
                            case Token_EndOfStream:
                                {
                                    Parsing = false;
                                }
                                break;

                            case ...:
                                break;
                            ...

                        }
                    }
                }



20:58
now Casey will write the GetToken(); function 

the first thing we want to do in the lexing process is to eatup any white spaces 
                           

                simple_preprocessor.cpp

                static token GetToken(tokenizer *Tokenizer)
                {
                    EatAllWhitespace(Tokenizer);

                    ...
                    ...

                    return(Token);
                }



21:42
Casey writing the EatAllWhitespace(); function
-   we first want to check it is white space. If it is we skip 

-   then if its a c style comment At[0] == '/' and At[0] == '/'
    we also want to skip 

    for the '// this is a line of comment', we want to skip till we reach end of line

-   also the block comment '/*' and '*/', we want to skip

    when we encouter a '/*', we want to skip till we see a '*/'




                static void EatAllWhitespace(tokenizer *Tokenizer)
                {
                    for(;;)
                    {
                        if(IsWhitespace(Tokenizer->At[0]))
                        {
                            ++Tokenizer->At;
                        }
                        else if((Tokenizer->At[0] == '/') &&
                                (Tokenizer->At[1] == '/'))
                        {
                            Tokenizer->At += 2;
                            while(Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0]))
                            {
                                ++Tokenizer->At;
                            }
                        }
                        else if((Tokenizer->At[0] == '/') &&
                                (Tokenizer->At[1] == '*'))
                        {
                            Tokenizer->At += 2;
                            while(Tokenizer->At[0] &&
                                  !((Tokenizer->At[0] == '*') &&
                                    (Tokenizer->At[1] == '/')))
                            {
                                ++Tokenizer->At;
                            }
                            
                            if(Tokenizer->At[0] == '*')
                            {
                                Tokenizer->At += 2;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }



22:11
here we have the IsWhitespace(); function. pretty much we are checking against all the white space that we know of.

-   ' ' is a regular space 
-   '\t' is a ASCII horizontal tab 
-   '\v' is a ASCII vertical tab 
-   '\f' is ASCII Formfeed

    '\f' means advance downard to the next 'page'. It was commonly used as page separators, but now 
    is also used as section separators. Text editors can use this characters when you 'insert a page break'

https://stackoverflow.com/questions/20298677/what-does-v-and-r-mean-are-they-white-spaces
                simple_preprocessor.cpp

                inline bool IsWhitespace(char C)
                {
                    bool Result = ((C == ' ') ||
                                   (C == '\t') ||
                                   (C == '\v') ||
                                   (C == '\f') ||
                                   IsEndOfLine(C));

                    return(Result);
                }




23:55
back to our GetToken(); function. 
so depending on our current char, we determine what the token_type is 

-   so we first assume that the Token.TextLength is 1 
    then all the single character cases becomes easy .


                simple_preprocessor.cpp

                static token GetToken(tokenizer *Tokenizer)
                {
                    EatAllWhitespace(Tokenizer);

                    token Token = {};
                    Token.TextLength = 1;
                    Token.Text = Tokenizer->At;
                    char C = Tokenizer->At[0];
                    ++Tokenizer->At;

                    switch(C)
                    {
                        case '\0': {Token.Type = Token_EndOfStream;} break;
                           
                        case '(': {Token.Type = Token_OpenParen;} break;
                        case ')': {Token.Type = Token_CloseParen;} break;
                        case ':': {Token.Type = Token_Colon;} break;
                        case ';': {Token.Type = Token_Semicolon;} break;
                        case '*': {Token.Type = Token_Asterisk;} break;
                        case '[': {Token.Type = Token_OpenBracket;} break;
                        case ']': {Token.Type = Token_CloseBracket;} break;
                        case '{': {Token.Type = Token_OpenBrace;} break;
                        case '}': {Token.Type = Token_CloseBrace;} break;
                 

                    }
                }


25:51
we then deal with the more complicated cases 

-   so the first complicate case we will do is reading a string.
    the idea is that if we see a '"' character, we keep on reading till the next '"' character

-   Casey also handled the case for '\\'
    [dont quite understand this case]

                simple_preprocessor.cpp

                static token GetToken(tokenizer *Tokenizer)
                {
                    EatAllWhitespace(Tokenizer);

                    token Token = {};
                    ...
                    Token.Text = Tokenizer->At;
                    char C = Tokenizer->At[0];
                    ++Tokenizer->At;

                    switch(C)
                    {
                        ...
                        ...
                        
                        case '"':
                        {
                            Token.Type = Token_String;

                            Token.Text = Tokenizer->At;
                            
                            while(Tokenizer->At[0] &&
                                  Tokenizer->At[0] != '"')
                            {
                                if((Tokenizer->At[0] == '\\') &&
                                   Tokenizer->At[1])
                                {
                                    ++Tokenizer->At;
                                }                
                                ++Tokenizer->At;
                            }

                            Token.TextLength = Tokenizer->At - Token.Text;
                            if(Tokenizer->At[0] == '"')
                            {
                                ++Tokenizer->At;
                            }
                        } break;


                    }
                }


28:11
Then Casey added the case when our character is alpha numerical 

-   this is the token case of Token_Identifier, so we set "Token.Type = Token_Identifier"


-   also notice that we set the Token.TextLength by doing 
                
                Token.TextLength = Tokenizer->At - Token.Text;


-   full code below

                simple_preprocessor.cpp

                static token GetToken(tokenizer *Tokenizer)
                {
                    EatAllWhitespace(Tokenizer);

                    token Token = {};
                    ...
                    Token.Text = Tokenizer->At;
                    char C = Tokenizer->At[0];
                    ++Tokenizer->At;

                    switch(C)
                    {
                        ...
                        ...
                        
                        case '"':
                        {
                            ...
                            ...
                        } break;


                        default:
                        {
                            if(IsAlpha(C))
                            {
                                Token.Type = Token_Identifier;0
                                
                                while(IsAlpha(Tokenizer->At[0]) ||
                                      IsNumber(Tokenizer->At[0]) ||
                                      (Tokenizer->At[0] == '_'))
                                {
                                    ++Tokenizer->At;
                                }
                                
                                Token.TextLength = Tokenizer->At - Token.Text;                
                            }
                #if 0
                            else if(IsNumeric(C))
                            {
                                ParseNumber();
                            }
                #endif
                            else
                            {
                                Token.Type = Token_Unknown;
                            }
                        } break;        


                    }
                }



44:25
after much debugging,
we seemed to be getting Token_Identifiers.

so now we will be looking for the "introspect" identifier 

                simple_preprocessor.cpp

                int main(int ArgCount, char **Args)
                {
                    char *FileContents = ReadEntireFileIntoMemoryAndNullTerminate("handmade_sim_region.h");

                    tokenizer Tokenizer = {};
                    Tokenizer.At = FileContents;

                    bool Parsing = true;
                    while(Parsing)
                    {
                        token Token = GetToken(&Tokenizer);
                        switch(Token.Type)
                        {
                            ...
                            ...

                            case Token_Identifier:
                            {                
        ---------->             if(TokenEquals(Token, "introspect"))
                                {
                                    ParseIntrospectable(&Tokenizer);
                                }
                            } break;
                                        
                            default:
                            {
                //                printf("%d: %.*s\n", Token.Type, Token.TextLength, Token.Text);
                            } break;
                        }
                    }
                }

51:02
Casey now write the ParseIntrospectable(); function 

-   once we get inside the "introspect" token, the next thing we expect is an OpenParenthesis
    so we first do a RequireToken(Tokenizer, Token_OpenParen); check 

    if the parentheses isnt there, we printout an error message of "Missing parentheses"

-   then we get the "struct" token, that way we know we are parsing a struct

-   full code below:

                simple_preprocessor.cpp

                static void ParseIntrospectable(tokenizer *Tokenizer)
                {
                    if(RequireToken(Tokenizer, Token_OpenParen))
                    {
                        ParseIntrospectionParams(Tokenizer);

                        token TypeToken = GetToken(Tokenizer);
                        if(TokenEquals(TypeToken, "struct"))
                        {
                            ParseStruct(Tokenizer);
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: Introspection is only supported for structs right now :(\n");            
                        }
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: Missing parentheses.\n");
                    }
                }




53:42
in here, we keep on reading till we see a "ClosingParenthesis"

recall the definition looks like: 

                introspect(category:"regular butter") struct sim_entity

-   full code below:

                simple_preprocessor.cpp

                static void ParseIntrospectionParams(tokenizer *Tokenizer)
                {
                    for(;;)
                    {
                        token Token = GetToken(Tokenizer);
                        if((Token.Type == Token_CloseParen) ||
                           (Token.Type == Token_EndOfStream))
                        {
                            break;
                        }
                    }
                }




54:02
               
here we see how we parse a struct 

here essentially, we first find an Token_OpenBrace,

and if we encounter a Token_CloseBrace, that means we finished parsing our struct.
[this does mean that we cant define a struct inside a struct]

then it is a regular Token_Identifier, we call ParseMember();


                simple_preprocessor.cpp

                static void ParseStruct(tokenizer *Tokenizer)
                {
                    token NameToken = GetToken(Tokenizer);
                    if(RequireToken(Tokenizer, Token_OpenBrace))
                    {
                        
                        printf("member_definition MembersOf_%.*s[] = \n", NameToken.TextLength, NameToken.Text);
                        printf("{\n");
                        for(;;)
                        {
                            token MemberToken = GetToken(Tokenizer);
                            if(MemberToken.Type == Token_CloseBrace)
                            {
                                break;
                            }
                            else
                            {
                                ParseMember(Tokenizer, MemberToken);
                            }
                        }
                        printf("}\n");
                    }
                }





55:25
then we have the ParseMember(); function. 

-   notice that we check for Token_Asterisk,
    that means we have a pointer. For now, we are just setting the IsPointer flag. We will do more in the next episode

-   for now, when we see an Token_Identifier, we just print them out 




-   full code below:

                static void ParseMember(tokenizer *Tokenizer, token MemberTypeToken)
                {
                #if 1
                    bool Parsing = true;
                    bool IsPointer = false;
                    while(Parsing)
                    {
                        token Token = GetToken(Tokenizer);
                        switch(Token.Type)
                        {
                            case Token_Asterisk:
                            {
                                IsPointer = true;
                            } break;

                            case Token_Identifier:
                            {
                                printf("{Type_%.*s, \"%.*s\"},\n",
                                       MemberTypeToken.TextLength, MemberTypeToken.Text,
                                       Token.TextLength, Token.Text);                
                            } break;

                            case Token_Semicolon:
                            case Token_EndOfStream:
                            {

                                Parsing = false;
                            } break;
                        }
                    }
                #else
                    token Token = GetToken(Tokenizer);
                    switch(Token.Type)
                    {
                        case Token_Asterisk:
                        {
                            ParseMember(Tokenizer, Token);
                        } break;

                        case Token_Identifier:
                        {
                            printf("DEBUG_VALUE(%.*s);\n", Token.TextLength, Token.Text);                
                        } break;
                    }
                #endif
                }


Q/A 
1:02:52
Someone asked is this a "Recursive Descent Parser"?

Yes. There are a number of ways to write parsers.
To be fair, we just wrote a "Decent Parser"


so if you want to do a "Recursive Decent Parser", you would do something like 



                static void ParseMember(tokenizer *Tokenizer, token MemberTypeToken)
                {
                    token Token = GetToken(Tokenizer);
                    switch(Token.Type)
                    {
                        case Token_Asterisk:
                        {
                            ParseMember(Tokenizer, Token);
                        } break;

                        case Token_Identifier:
                        {
                            printf("DEBUG_VALUE(%.*s);\n", Token.TextLength, Token.Text);                
                        } break;
                    }
                }

this Implementation is just cleaner
typically if you are writing a parser by hand from scratch, you would write a "Recursive Descent Parser"

there are other types of parsers, which are typically for Generation. For example if you are writing a parser generator.



1:12:36
so in a programming language, you would specify the structure of your data, 
(struct or class);

the compiler for your language knows what these things.
It has to know cuz it needs to convert it into working code 

the compiler has the concept of that structure in a semantic form 


C++ lacks a very important feature that most modern languages have.
that feature is to look at that structure within the code.

the structure that the compiler knows about inherently, you should be able to get it during compilation phase 
and you should be able to do things with them. 

in C/C++, there is know way to say: tell me all the members of the sim_entity struct, and print them out

this ability is called introspection.

its also called reflection in some cases 


1:18:39
Any suggestions on speeding on parsing?
There are alot of ways to speed up parsing. But personally this way of doing parsing is many orders of magnitude 
faster than the compiler actually compiles it, so Casey never had to optimize it. The microsoft compiler is so slow 
that our parsing code is never the bottleneck

