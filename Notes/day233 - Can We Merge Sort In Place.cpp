Handmade Hero Day 233 - Can We Merge Sort In Place?

Summary:
implemented mergesort

Keyword:
mergesort

5:32
Casey trying to implement MergeSort();

                handmade_render_group.cpp

                internal void MergeSort(u32 Count, tile_sort_entry *First, tile_sort_entry *Temp)
                {
                    if(Count == 1)
                    {
                        // NOTE(casey): No work to do.
                    }
                    else if(Count == 2)
                    {
                        tile_sort_entry *EntryA = First;
                        tile_sort_entry *EntryB = First + 1;
                        if(EntryA->SortKey > EntryB->SortKey)
                        {
                            Swap(EntryA, EntryB);
                        }
                    }
                    else
                    {
                        u32 Half0 = Count / 2;
                        u32 Half1 = Count - Half0;

                        Assert(Half0 >= 1);
                        Assert(Half1 >= 1);

                        tile_sort_entry *InHalf0 = First;
                        tile_sort_entry *InHalf1 = First + Half0;
                        tile_sort_entry *End = First + Count;

                        MergeSort(Half0, InHalf0, Temp);
                        MergeSort(Half1, InHalf1, Temp);

                        tile_sort_entry *ReadHalf0 = InHalf0;
                        tile_sort_entry *ReadHalf1 = InHalf1;

                        tile_sort_entry *Out = Temp;
                        for(u32 Index = 0;
                            Index < Count;
                            ++Index)
                        {
                            if(ReadHalf0 == InHalf1)
                            {
                                *Out++ = *ReadHalf1++;
                            }
                            else if(ReadHalf1 == End)
                            {
                                *Out++ = *ReadHalf0++;
                            }
                            else if(ReadHalf0->SortKey < ReadHalf1->SortKey)
                            {
                                *Out++ = *ReadHalf0++;
                            }
                            else
                            {
                                *Out++ = *ReadHalf1++;
                            }            
                        }
                        Assert(Out == (Temp + Count));
                        Assert(ReadHalf0 == InHalf1);
                        Assert(ReadHalf1 == End);
                            
                        // TODO(casey): Not really necessary if we ping-pong
                        for(u32 Index = 0;
                            Index < Count;
                            ++Index)
                        {
                            First[Index] = Temp[Index];
                        }
                        
                #if 0
                        tile_sort_entry *ReadHalf0 = First;
                        tile_sort_entry *ReadHalf1 = First + Half0;
                        tile_sort_entry *End = First + Count;

                        // NOTE(casey): Step 1 - Find the first out-of-order pair
                        while((ReadHalf0 != ReadHalf1) &&
                              (ReadHalf0->SortKey < ReadHalf1->SortKey))
                        {
                            ++ReadHalf0;
                        }

                        // NOTE(casey): Step 2 - Swap as many Half1 items in as necessary
                        if(ReadHalf0 != ReadHalf1)
                        {
                            tile_sort_entry CompareWith = *ReadHalf0;
                            while((ReadHalf1 != End) && (ReadHalf1->SortKey < CompareWith.SortKey))
                            {
                                Swap(ReadHalf0++, ReadHalf1++);
                            }

                            ReadHalf1 = InHalf1;
                        }
                #endif
                    }
                }


pretty much spent the whole episode trying to figure out if you can do merge sort inplace.

turns out you cant.
