#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>

// gcc binpack3.c -o binpack3.bin -O3 -march=native -Wall

#define OBPP_TIMEOUT 1
#define OBPP_SEARCH_COMPLETE 0
#define OBPP_PACKING_FOUND -1

typedef struct {
  uint32_t *itemsizes; // itemsizes[0] ... itemsizes[itemscount-1]
  uint32_t *bins; // bins[0] ... bins[itemscount-1]
  uint32_t *binsums; // bins[0] ... bins[binscount-1]
  uint32_t *totminbinws; // totminbinws[0] ... totminbinws[itemscount-1]
  uint64_t mstimeout; // 0 or 50 <= mstimeout <= 1,000,000,000,000
  uint32_t optimalbinscount; // 0 if not yet known, -1 if optimality unknowable
  uint32_t binscount; 
  uint32_t wastedspace;
  uint32_t bincapacity; // 1 <= bincapacity <= 1000000
  uint32_t itemscount; // 1 <= itemscount <= 1000
  uint32_t sumitems; 
  uint32_t item; 
  _Bool solved;
} obpp_args_t;


void obpp_sortitems(obpp_args_t packing) {
  _Bool sorted = false;
  uint32_t temp;
  uint32_t i;
  while(!sorted) {
    sorted = true;
    for (i=0; i<packing.itemscount-1; i++) {
      if (packing.itemsizes[i] < packing.itemsizes[i+1]) {
        temp = packing.itemsizes[i];
        packing.itemsizes[i] = packing.itemsizes[i+1];
        packing.itemsizes[i+1] = temp;
        sorted = false;
      }
    }
  }
}

_Bool obpp_additem(obpp_args_t *packing, uint32_t itemsize) {
  if (packing->item >= packing->itemscount) return false;
  if (itemsize == 0) return false;
  if (itemsize > packing->bincapacity) return false;
  packing->itemsizes[packing->item] = itemsize;
  packing->sumitems += itemsize;
  packing->item++;
  if (packing->item == packing->itemscount) obpp_sortitems(*packing);
  return true;
}

_Bool obpp_free(obpp_args_t *packing) {
  if (packing->binsums == NULL) return false;
  if (packing->totminbinws == NULL) return false;
  if (packing->itemsizes == NULL) return false;
  if (packing->bins == NULL) return false;
  free(packing->binsums);
  packing->binsums = NULL;
  free(packing->totminbinws);
  packing->totminbinws = NULL;
  free(packing->itemsizes);
  packing->itemsizes = NULL;
  free(packing->bins);
  packing->bins = NULL;
  packing->solved = false;
  return true;
}

_Bool obpp_init(obpp_args_t *packing, uint64_t mstimeout, uint32_t bincapacity, uint32_t itemscount) {
  // Can use on same obpp_args_t struct after obpp_free().
  // Calling obpp_init() twice or more without obpp_free() will cause memory leaks.
  if (itemscount == 0) return false;
  if (itemscount > 1000) return false;
  if (bincapacity == 0) return false;
  if (bincapacity > 1000000) return false;
  if (mstimeout > 1000000000000ULL) return false;
  if ((mstimeout < 50) && (mstimeout != 0)) return false;
  packing->binsums = calloc(itemscount, sizeof(uint32_t));
  assert(packing->binsums != NULL);
  packing->totminbinws = calloc(itemscount, sizeof(uint32_t));
  assert(packing->totminbinws != NULL);
  packing->itemsizes = calloc(itemscount, sizeof(uint32_t));
  assert(packing->itemsizes != NULL);
  packing->bins = calloc(itemscount, sizeof(uint32_t));
  assert(packing->bins != NULL);
  packing->mstimeout = mstimeout;
  packing->optimalbinscount = 0;
  packing->binscount = 0;
  packing->wastedspace = 0;
  packing->bincapacity = bincapacity;
  packing->itemscount = itemscount;
  packing->sumitems = 0;
  packing->item = 0;
  packing->solved = false;
  return true;
}

_Bool obpp_printpacking(obpp_args_t packing) {
  if (!packing.solved) return false;
  uint32_t binsum;
  if (packing.optimalbinscount != packing.binscount) printf("Potentially Sub-Optimal Packing!\n");
  //for (uint32_t j=0; j<packing.itemscount; j++) printf("%u ", packing.bins[j]);
  //printf("\n");
  for (uint32_t i=0; i<packing.binscount; i++) {
    printf("[");
    binsum = 0;
    for (uint32_t j=0; j<packing.itemscount; j++) {
      if (packing.bins[j] == i) {
        if (binsum > 0) printf(" ");
        printf("%i", packing.itemsizes[j]);
        binsum += packing.itemsizes[j];
      }      
    }
    printf("] (Wasted = %u)\n", packing.bincapacity - binsum);
  }
  return true;
}

_Bool obpp_searchconcluded(obpp_args_t packing) {
  // Assumes that if the bins of the first binscount-1 items are as below, the search is over.
  // 0 1 ... binscount-4 binscount-3 binscount-1
  // This is because the last case of the first binscount items to process is
  // 0 1 ... binscount-4 binscount-3 binscount-2 binscount-1.
  // Cases where any of the first binscount items share a bin will have already been processed.
  int32_t i;
  for (i=0; (i+2)<packing.binscount; i++) {
    if (packing.bins[i] < i) return false;
  }
  if (packing.bins[i] > i) return true;
  return false;
}

uint64_t obpp_gettimems(void) {
  struct timeval thistime;
  gettimeofday(&thistime, NULL);
  return thistime.tv_sec*1000ULL + thistime.tv_usec/1000ULL;
}


int obpp_nextpacking(obpp_args_t packing) {
  uint64_t starttimems, currenttimems;
  uint32_t smallestitem = packing.itemsizes[packing.itemscount - 1];
  starttimems = obpp_gettimems();
  uint32_t i, istart, item, iteration;
  if (packing.item == packing.itemscount) {
    item = packing.item-1;
  } else {
    item = packing.item;
  }
  _Bool backtrack;
  int32_t binwastedspace, minbinwastedspace, totminbinwastedspace;
  iteration = 0;
  while (true) {
    if (packing.bins[item] != 0xffffffff) {
      packing.binsums[packing.bins[item]] -= packing.itemsizes[item];
      istart = packing.bins[item]+1;
      packing.bins[item] = 0xffffffff;
    } else {
      istart = 0;
    }
    if (packing.itemsizes[item] == packing.itemsizes[item-1]) {
      istart = (istart < packing.bins[item-1] ? packing.bins[item-1] : istart);
    }
    backtrack = true;
    iteration++;
    if ((iteration & 0xfffff) == 0) {
      iteration = 0;
      if (packing.mstimeout > 0) {      
        currenttimems = obpp_gettimems();
        if ((currenttimems - starttimems) > packing.mstimeout) return OBPP_TIMEOUT;
      }
    }
    totminbinwastedspace = packing.totminbinws[item-1];
    for (i=istart; i<packing.binscount; i++) {
      binwastedspace = (int32_t)(packing.bincapacity - packing.binsums[i]) - (int32_t)packing.itemsizes[item];
      if (binwastedspace >= 0) {
        minbinwastedspace = (binwastedspace < smallestitem ? binwastedspace : 0);
        if (totminbinwastedspace + minbinwastedspace <= packing.wastedspace) {
          packing.binsums[i] += packing.itemsizes[item];
          packing.bins[item] = i;
          packing.totminbinws[item] = totminbinwastedspace + minbinwastedspace;
          if ((item + 2) == packing.binscount) {
            if ((item + 1) == i) {
              if (obpp_searchconcluded(packing)) return OBPP_SEARCH_COMPLETE;
            }
          }
          item += 1;
          if (item == packing.itemscount) return OBPP_PACKING_FOUND;
          backtrack = false;
          packing.bins[item] = 0xffffffff;
          break;
        }
      }
    }    
    if (backtrack) item -= 1;
    if (item == 0) return OBPP_SEARCH_COMPLETE;
  }
}

void obpp_randomtask(uint32_t bincapacity, uint32_t minitemsize, uint32_t maxitemsize, uint32_t itemscount) {
  printf("%u ", bincapacity);
  for(int i=0; i<itemscount; i++) {
    printf("%u ", minitemsize + (rand() % (maxitemsize - minitemsize + 1)));
  }
}

_Bool obpp_packbins(obpp_args_t *packing, _Bool report) {
  if (packing->binscount == 0) packing->binscount = (packing->bincapacity - 1 + packing->sumitems)/packing->bincapacity;
  uint32_t distinctitemsizes, i;
  int res;
  if (report) {
    distinctitemsizes = 1;
    for (i=1; i<packing->itemscount; i++) {
      distinctitemsizes += (packing->itemsizes[i] != packing->itemsizes[i-1]);
    }
    printf("Packing %u items with %u distinct sizes.\n", packing->itemscount, distinctitemsizes);
  }
  while (true) {
    packing->wastedspace = packing->binscount*packing->bincapacity - packing->sumitems;
    if (report) printf("Looking for a solution with %u bins. (Wasted Space = %u)\n", packing->binscount, packing->wastedspace);  
    if (!packing->solved) {   
      packing->item = 0;
      memset(packing->bins, 0xff, packing->itemscount*sizeof(uint32_t));
      memset(packing->binsums, 0, packing->itemscount*sizeof(uint32_t));
      memset(packing->totminbinws, 0, packing->itemscount*sizeof(uint32_t));
      i = 0;
      while ((i < packing->binscount) && (packing->item < packing->itemscount)) {
        if (packing->binsums[i] + packing->itemsizes[packing->item] <= packing->bincapacity) {
          packing->binsums[i] += packing->itemsizes[packing->item];
          packing->bins[packing->item] = i;
          packing->item++;
        } else {
          i++;
        }
      }
      if (packing->item >= packing->itemscount) {
        packing->solved = true;
        if (packing->optimalbinscount == 0) packing->optimalbinscount = packing->binscount;
        return true;
      }
    }
    res = obpp_nextpacking(*packing); 
    if (res == OBPP_TIMEOUT) {
      if (report) printf("Search Timed Out!\n");  
      if (packing->optimalbinscount == 0) packing->optimalbinscount = 0xffffffff;      
      packing->solved = false; 
    }  
    if (res == OBPP_PACKING_FOUND) {
      packing->solved = true;      
      packing->item = packing->itemscount;
      if (packing->optimalbinscount == 0) packing->optimalbinscount = packing->binscount;
      return true;
    }
    packing->binscount++;
  }
}


int main(int argc, char**argv) {
  obpp_args_t packing;
  srand(time(0));
  //obpp_randomtask(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
  //exit(0);
  uint32_t i, bincapacity;
  uint32_t element, itemscount;
  uint64_t mstimeout;
  _Bool validargs = false;
  if (argc > 1) {
    mstimeout = atol(argv[1]);
    if ((mstimeout == 0) || ((mstimeout >= 50ULL) && (mstimeout <= 1000000000000ULL))) {
      if (argc > 2) {
        bincapacity = atoi(argv[2]);
        if (bincapacity <= 1000000) {
          itemscount = argc-3;
          if (itemscount <= 1000) {
            if (obpp_init(&packing, mstimeout, bincapacity, itemscount)) {
              for (i=3; i<argc; i++) {
                element = atoi(argv[i]);
                if ((element == 0) || (element > bincapacity) || !obpp_additem(&packing, element)) {
                  obpp_free(&packing);
                  break;
                }
              }
              if (i == argc) validargs = true;
            }
          }
        }
      }
    }
  } 
  if (!validargs) {
    printf("This program attempts to solve the offline, optimal 1D bin packing problem. It finds the minimum number of bins b into which n items (1 <= n <= 1000) can be packed without overflowing the bincapacity (1 <= bincapacity <= 1000000). The itemsizes must be positive and not more than the bincapacity. A minimum of mstimeout milliseconds (0 or 50 <= mstimeout <= 1,000,000,000,000) are allowed to find a fit to b bins, after which the search is promoted to finding a fit to an incremented number of bins. \nIf mstimeout = 0, no timeout is imposed, and only an optimal solution will be returned before program termination. Searches with large numbers of items/bins can be prohibitively time consuming, especially when searching for a tight fit (not much wasted space), and are not guaranteed to find solutions in a reasonable time with the inital guesses for the bin count resulting in an exhaustive search of the space. It is guaranteed however, that when a solution is found, it is optimal or is marked as 'Potentially Sub-Optimal'.\n");
    printf("Usage:- %s mstimeout bincapacity itemsize1 ... itemsizen\n", argv[0]);
    printf("e.g. %s 0 10 5 4 3 2 1 2 3 4 5\n", argv[0]);
    printf("Copyright:- Simon Goater Feb 2024.\n");
    exit(0);
  }
  obpp_packbins(&packing, true);
  printf("%u bins are required.\n", packing.binscount);
  obpp_printpacking(packing);
  obpp_free(&packing);
}
/*
time ./binpack3.bin 0 200 23 24 25 36 47 58 32 34 45 43 45 56 67 54
Packing 14 items with 13 distinct values.
Looking for a solution with 3 bins. (Wasted Space = 11)
3 bins are required.
[67 58 47 25] (Wasted = 3)
[56 54 45 45] (Wasted = 0)
[43 36 34 32 24 23] (Wasted = 8)

real	0m0.002s
user	0m0.002s
sys	0m0.000s


time ./binpack3.bin 0 200 23 24 25 36 47 58 32 34 45 43 45 56 67 54 23 13 24 45 36 71 21 33
Packing 22 items with 17 distinct values.
Looking for a solution with 5 bins. (Wasted Space = 145)
5 bins are required.
[71 67 58] (Wasted = 4)
[56 54 47] (Wasted = 43)
[45 45 45 43] (Wasted = 22)
[36 36 34 33 32 25] (Wasted = 4)
[24 24 23 23 21 13] (Wasted = 72)

real	0m0.002s
user	0m0.002s
sys	0m0.000s


time ./binpack3.bin 0 362 123 222 211 158 211 231 158 34 109 56 47 135 106 162 103 99 85 131 86 117 78 85 113 42 55 69 125 78 231 155
Packing 30 items with 25 distinct values.
Looking for a solution with 10 bins. (Wasted Space = 5)
Looking for a solution with 11 bins. (Wasted Space = 367)
11 bins are required.
[231 86 42] (Wasted = 3)
[231 85 34] (Wasted = 12)
[222 85 55] (Wasted = 0)
[211 78 69] (Wasted = 4)
[211 78 56] (Wasted = 17)
[162 158] (Wasted = 42)
[158 155 47] (Wasted = 2)
[135 131] (Wasted = 96)
[125 123] (Wasted = 114)
[117 113 109] (Wasted = 23)
[106 103 99] (Wasted = 54)

real	0m1.566s
user	0m1.560s
sys	0m0.004s


time ./binpack3.bin 0 600 123 222 251 158 211 231 158 34 109 56 147 135 106 162 103 99 85 31 86 117 178 85 113 42 55 69 125 78 231
Packing 29 items with 26 distinct values.
Looking for a solution with 6 bins. (Wasted Space = 0)
6 bins are required.
[251 222 85 42] (Wasted = 0)
[231 211 158] (Wasted = 0)
[231 178 135 56] (Wasted = 0)
[162 158 147 78 55] (Wasted = 0)
[125 123 113 109 99 31] (Wasted = 0)
[117 106 103 86 85 69 34] (Wasted = 0)

real	30m36.296s
user	30m27.990s
sys	0m1.079s

time ./binpack3.bin 0 1000 198 286 165 155 109 289 287 171 246 176 146 254 271 148 170 206 287 239 243 281 198 162 116 196 270 121 155 145 163 185 289 211 120 103 266 129 243 253 150 188 279 197 141 250 195 162 155 181 100 248 112 148 261 128 245 230 300 300 226 163 134 164 224 155 167 190 134 109 293 135 147 
Packing 71 items with 58 distinct sizes.
Looking for a solution with 14 bins. (Wasted Space = 137)
14 bins are required.
[300 300 293 103] (Wasted = 4)
[289 289 287 135] (Wasted = 0)
[287 286 281 146] (Wasted = 0)
[279 271 270 155] (Wasted = 25)
[266 261 254 116 100] (Wasted = 3)
[253 250 248 246] (Wasted = 3)
[245 243 243 239] (Wasted = 30)
[230 226 224 211 109] (Wasted = 0)
[206 198 198 197 196] (Wasted = 5)
[195 190 188 185 181] (Wasted = 61)
[176 171 170 167 165 150] (Wasted = 1)
[164 163 163 155 121 120 112] (Wasted = 2)
[162 162 155 155 129 128 109] (Wasted = 0)
[148 148 147 145 141 134 134] (Wasted = 3)

real	0m28.705s
user	0m28.655s
sys	0m0.000s

time ./binpack3.bin 000 500 188 202 271 166 269 245 228 111 197 125 119 128 163 216 249 238 172 123 296 130 132 155 254 154 257 137 115 116 114  219 152 126 189 219 296 133 297 257 230 273 276 258 135 142 
Packing 44 items with 41 distinct sizes.
Looking for a solution with 17 bins. (Wasted Space = 28)
Looking for a solution with 18 bins. (Wasted Space = 528)
18 bins are required.
[297 189] (Wasted = 14)
[296 188] (Wasted = 16)
[296 172] (Wasted = 32)
[276 166] (Wasted = 58)
[273 163] (Wasted = 64)
[271 155] (Wasted = 74)
[269 154] (Wasted = 77)
[258 128 114] (Wasted = 0)
[257 132 111] (Wasted = 0)
[257 126 116] (Wasted = 1)
[254 130 115] (Wasted = 1)
[249 245] (Wasted = 6)
[238 230] (Wasted = 32)
[228 219] (Wasted = 53)
[219 216] (Wasted = 65)
[202 152 142] (Wasted = 4)
[197 137 135] (Wasted = 31)
[133 125 123 119] (Wasted = 0)

real	80m7.924s
user	79m46.528s
sys	0m2.622s



time ./binpack3.bin 0 865 202 120 188 202 271 166 269 245 228 211 197 189 119 131 163 216 149 238 172 123 296 130 66 137 191 86 44 165 222 54
Packing 30 items with 29 distinct sizes.
Looking for a solution with 6 bins. (Wasted Space = 0)
6 bins are required.
[296 271 188 66 44] (Wasted = 0)
[269 245 228 123] (Wasted = 0)
[238 222 216 189] (Wasted = 0)
[211 202 202 131 119] (Wasted = 0)
[197 191 172 165 86 54] (Wasted = 0)
[166 163 149 137 130 120] (Wasted = 0)

real	280m14.493s
user	279m40.117s
sys	0m5.182s
*/

