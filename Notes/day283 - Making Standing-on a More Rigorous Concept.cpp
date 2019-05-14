Handmade Hero 283 - Making Standing-on a More Rigorous Concept

Summary:
Make Entity store actual traversable points instead of just positions

Keyword:
entity movement code


19:27
Casey discussing how he wants resolve two people jumping into the same standing point


40:50
Casey mentions that he now wants to track the actual traversal points instead of just 
"MovementFrom" and "MovementTo" in the entity struct


so Casey added a traversable_reference struct 

                struct traversable_reference
                {
                    entity_reference Entity;
                    u32 TraversableIndex;
                };


now entity struct is 

                handmade_entity.h

                struct entity
                {
                    ...
                    traversable_reference MovementFrom;
                    traversable_reference MovementTo;
                    ...
                };

54:58
Casey made the entity remember which traversable point it is standing on 
so it renamed MovementFrom to StandingOn;

                handmade_entity.h

                struct entity
                {
                    ...
                    traversable_reference StandingOn;
                    traversable_reference MovementTo;
                    ...
                };
