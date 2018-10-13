Handmade Hero Day 091 - Bases Part II

Summary:
introduced the concept of transformation among coordinate systems
nothing too complicated. Essentially things you already know

Keyword:
renderer

pretty much talking about transformation among coordinate systems 



32:55

added the render_entry_coordinate_system render entry type 

				struct render_entry_coordinate_system
				{
				    render_group_entry_header Header;
				    v2 Origin;
				    v2 XAxis;
				    v2 YAxis;
				    v4 Color;

				    v2 Points[16];
				};

I do that too in my games :)






34:58
added a time variable in game_state to test rotations

				struct game_state
				{
					...
					...

				    real32 Time;
				};






				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{ 

					...
					...

				    GameState->Time += Input->dtForFrame;
				    real32 Angle = GameState->Time;

				    ...
				    ...
				}




38:52
added the CoordinateSystem draw call in handmade_render_group.h


				handmade_render_group.h

				inline render_entry_coordinate_system *
				CoordinateSystem(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color)
				{
				    render_entry_coordinate_system *Entry = PushRenderElement(Group, render_entry_coordinate_system);
				    if(Entry)
				    {
				        Entry->Origin = Origin;
				        Entry->XAxis = XAxis;
				        Entry->YAxis = YAxis;
				        Entry->Color = Color;
				    }

				    return(Entry);
				}









43:28
added the render_entry_coordinate_system case in RenderToOutput 
essentially, we are just drawing three lines

	            case RenderGroupEntryType_render_entry_coordinate_system:
	            {
	                render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Header;
	                
	                v2 Dim = {2, 2};
	                v2 P = Entry->Origin;
	                DrawRectangle(OutputTarget, P - Dim, P + Dim, Entry->Color.r, Entry->Color.g, Entry->Color.b);

	                P = Entry->Origin + Entry->XAxis;
	                DrawRectangle(OutputTarget, P - Dim, P + Dim, Entry->Color.r, Entry->Color.g, Entry->Color.b);
	                
	                P = Entry->Origin + Entry->YAxis;
	                DrawRectangle(OutputTarget, P - Dim, P + Dim, Entry->Color.r, Entry->Color.g, Entry->Color.b);

	                for(uint32 PIndex = 0;
	                    PIndex < ArrayCount(Entry->Points);
	                    ++PIndex)
	                {
	                    v2 P = Entry->Points[PIndex];
	                    P = Entry->Origin + P.x*Entry->XAxis + P.y*Entry->YAxis;
	                    DrawRectangle(OutputTarget, P - Dim, P + Dim, Entry->Color.r, Entry->Color.g, Entry->Color.b);
	                }
	                
	                BaseAddress += sizeof(*Entry);
	            } break;





46:58

starting to make rotating x and y axis 


			    GameState->Time += Input->dtForFrame;
			    real32 Angle = GameState->Time;

			    // TODO(casey): Let's add a perp operator!!!
			    v2 Origin = ScreenCenter + 10.0f*V2(Sin(Angle), 0.0f);
			    v2 XAxis = (100.0f + 25.0f*Cos(4.2f*Angle))*V2(Cos(Angle), Sin(Angle));
			    v2 YAxis = (100.0f + 50.0f*Sin(3.9f*Angle))*V2(Cos(Angle + 1.0f), Sin(Angle + 1.0f));
			    uint32 PIndex = 0;
			    render_entry_coordinate_system *C = CoordinateSystem(RenderGroup, Origin, XAxis, YAxis, V4(0.5f+0.5f*Sin(Angle), 0.5f+0.5f*Sin(2.9f*Angle), 0.5f+0.5f*Cos(9.9f*Angle), 1));
			    for(real32 Y = 0.0f; Y < 1.0f; Y += 0.25f)
			    {
			        for(real32 X = 0.0f; X < 1.0f; X += 0.25f)
			        {
			            C->Points[PIndex++] = V2(X, Y);
			        }
			    }
