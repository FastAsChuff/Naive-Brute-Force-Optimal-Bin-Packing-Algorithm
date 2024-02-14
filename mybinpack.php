<?php
// Motivated by https://stackoverflow.com/questions/77973738/how-to-solve-this-interview-task
// Also see https://stackoverflow.com/questions/19941706/bin-packing-algorithm-in-need-of-speeding-up
// and https://stackoverflow.com/questions/47402590/simple-non-trivial-bin-packing-instance
/* 

Naive Brute Force Optimal Bin Packing Algorithm.
================================================
   
   This algorithm finds an optimal way of packing n items (positive integers) 
   in an array into b bins using a naive algorithm.
      
     b = ceil(sumitems/bincapacity) // Known lower bound for b.
          
     do 
     
       initialise packing array
     
       // Search for a valid way to pack all n items into b bins
       
       do 
       
         check packing array is valid (no bins overflow)
     
         If valid packing found, print the packing array and terminate.
       
         next packing array
         
         if packing arrays exhausted, break
       
       loop
     
       b++
     
     loop
     
   The method for producing the next packing array is simply by counting digit by digit from zero up to b-1 
   meaning there are b^n distinct packing arrays for a given b and n. This means that to search the whole space,
   which it must do to prove that there is no valid solution with b bins, has an approximate time complexity of O(b^n).
   In practice, if a solution exists, it will probably be found long before the whole space is searched, 
   but if none exists, the whole space must be searched before searching for a solution with an incremented b.
   
   Typical whole space search time is of the order of b^n us on an old Core2 Duo 2.1GHz laptop. The catastrophic time
   complexity means that it is easy to create problems that would have runtimes longer than the age of the universe. 
   However, runtimes for small b and n may be acceptable. 
   
   It is recommended that items which must be put in their own bins, where itemsize + minitemsize > bincapacity, are excluded from the array. To reduce runtimes, try removng the k smallest elements, without reducing the number of starting bins, then try to fit them manually into an optimal bin packing solution of the remaining n-k items.
*/

function initpacking($itemcount) {
  $packing = array();
  for ($i=0; $i<$itemcount; $i++) {
    $packing[$i] = 0;
  }
  return $packing;
}

function checkpacking($bincount, $itemcount, $itemsizes, $packing, $bincapacity) {
  $bins = array();
  for($i=0; $i<$bincount; $i++) {
    $bins[$i] = 0;
  }
  for($i=0; $i<$itemcount; $i++) {
    $bins[$packing[$i]] += $itemsizes[$i];
    if ($bins[$packing[$i]] > $bincapacity) return false;
  }
  return true;
}

function nextpacking($itemcount, $bincount, $packing) {
  $packingix = 0;
  while ($packingix < $itemcount) {
    if ($packing[$packingix] < $bincount-1) {
      $packing[$packingix]++;
      return $packing;
    }
    $packing[$packingix] = 0;
    $packingix++;
  }
  return NULL;
}

function printpacking($bincount, $itemcount, $itemsizes, $packing) {
  $sumarray = array();
  for ($i=0; $i<$bincount; $i++) {
    $sum = 0;
    echo "[";
    for ($j=0; $j<$itemcount; $j++) {
      if ($packing[$j] == $i) {
        echo $itemsizes[$j]." ";
        $sum += $itemsizes[$j];
      }
    }
    echo "]\n";
    $sumarray[] = $sum;
  }
  echo "Bin Totals ";
  for ($i=0; $i<$bincount; $i++) {
    echo $sumarray[$i]." ";
  }
  echo "\n";
}

function packbins($itemsizes, $bincapacity) {
  $sumitemsizes = array_sum($itemsizes);
  $itemcount = count($itemsizes);
  $bincount = ceil($sumitemsizes/$bincapacity);
  $sum = 0;
  for ($i=0; $i<$itemcount; $i++) {
    echo $itemsizes[$i]." ";
    $sum += $itemsizes[$i];
  }
  echo "\nSum = $sum\n";
  while (1) {
    echo "Looking for bin packing solution with $bincount bins of max. capacity $bincapacity.\n";
    $packing = initpacking($itemcount);
    while (!($goodpacking = checkpacking($bincount, $itemcount, $itemsizes, $packing, $bincapacity))) {
      if (($packing = nextpacking($itemcount, $bincount, $packing)) == NULL) break;
    }
    if ($goodpacking) {
      printpacking($bincount, $itemcount, $itemsizes, $packing);
      echo "$bincount bins are required.\n";
      return $packing;
    }
    $bincount++;
  }
}

//$itemsizes = [10, 7, 7, 5, 5]; // 2 (time = 47ms)
//$itemsizes = [12, 7, 6, 2, 2]; // 2 (time = 47ms)
//$itemsizes = [16, 2, 2, 18, 18, 4]; // 3 (time = 47ms)
//$itemsizes = [13, 5, 6, 11, 15, 11, 19, 12, 7, 18]; // 6->7 (time = 75s)
$itemsizes = [13, 5, 6, 11, 15, 11, 12, 7]; // 4->5 (time = 80ms)
$bincapacity = 20;
$packing = packbins($itemsizes, $bincapacity);
?>
