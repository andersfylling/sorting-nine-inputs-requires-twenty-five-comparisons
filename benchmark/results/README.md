These files were generated before the metrics types were updated to make more sense for this software.

Note that the "cores" refers to the number of (threads - 1), and is therefore accounted for in the python script.

The prolog data is not accurate to the second, but given that some of the runs take minutes, I do not consider this to be significant enough to throw out the data.
The reason I don't consider the data accurate (the durations) is that I simply measured the time between file updates. The prolog code creates a final file for the generated and pruned filters.

The prolog program was given to me by an author of the paper, and I ran the configuration to use a chunk 5,000. The same size as the segments used in the c++ code.

Given that I have not explicitly asked not been given the right to share the prolog code; I will simply not. I recommend asking the authors for a copy if you want to test my results.

I did not bother with N9, as it will probably take days to run.