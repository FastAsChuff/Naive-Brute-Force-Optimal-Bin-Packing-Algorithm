# Naive-Brute-Force-Optimal-Bin-Packing-Algorithm

mybinpack.php
=============
 
Naive Brute Force Optimal Bin Packing Algorithm. Clearly optimal results only from this code. It becomes inefficient quickly however, and so for more diffucult problems, use the following.

binpack3.c
==========

This program attempts to solve the offline, optimal 1D bin packing problem. It finds the minimum number of bins b into which n items (1 <= n <= 1000) can be packed without overflowing the bincapacity (1 <= bincapacity <= 1000000). The itemsizes must be positive and not more than the bincapacity. A minimum of mstimeout milliseconds (0 or 50 <= mstimeout <= 1,000,000,000,000) are allowed to find a fit to b bins, after which the search is promoted to finding a fit to an incremented number of bins. 

If mstimeout = 0, no timeout is imposed, and only an optimal solution will be returned before program termination. Searches with large numbers of items/bins can be prohibitively time consuming, especially when searching for a tight fit (not much wasted space), and are not guaranteed to find solutions in a reasonable time with the inital guesses for the bin count resulting in an exhaustive search of the space. It is guaranteed however, that when a solution is found, it is optimal or is marked as 'Potentially Sub-Optimal'.

Usage:- ./binpack3.bin mstimeout bincapacity itemsize1 ... itemsizen

e.g. ./binpack3.bin 0 10 5 4 3 2 1 2 3 4 5

Copyright:- Simon Goater Feb 2024.

