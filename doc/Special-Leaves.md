# Computation of the special leaves in the combinatorial type prime counting algorithms

The combinatorial type prime counting algorithms
([Lagarias-Miller-Odlyzko](https://www.ams.org/journals/mcom/1985-44-170/S0025-5718-1985-0777285-5/S0025-5718-1985-0777285-5.pdf),
[Deleglise-Rivat](https://www.ams.org/journals/mcom/1996-65-213/S0025-5718-96-00674-6/S0025-5718-96-00674-6.pdf),
[Gourdon](http://numbers.computation.free.fr/Constants/Primes/Pix/piNalgorithm.ps))
consist of many formulas and the formula that usually
takes longest to compute and is by far the most difficult to implement is the formula
of the so-called special leaves (sometimes called hard special leaves).
[primecount's implementation](https://github.com/kimwalisch/primecount/blob/master/src/deleglise-rivat/S2_hard.cpp)
of the special leaves formula is different from the algorithms that have
been described in any of the combinatorial prime counting papers so far. This document
describes the history of how primecount's implementation came to be and it describes
a practical improvement for the computation of the special leaves that I found in
February 2020 that I consider important.

Implementing the special leaves formula requires use of a prime sieve. The algorithm
is basically a modified version of the well known
[segmented sieve of Eratosthenes](https://en.wikipedia.org/wiki/Sieve_of_Eratosthenes)
which consists of 2 main parts that are executed alternately:

1) Sieve out primes and multiples of primes.
2) Count the number of unsieved elements in the sieve array.

Since there is a large number of leaves for which we have to count the number of
unsieved elements in the sieve array Lagarias-Miller-Odlyzko have suggested using
a [binary indexed tree](https://en.wikipedia.org/wiki/Fenwick_tree) data structure
(a.k.a. Fenwick tree) to speedup counting.
For any number n the binary indexed tree allows to count the number of unsieved
elements ≤ n using only O(log(n)) operations. However the binary indexed tree
must also be updated whilst sieving which slows down the sieving part of the
algorithm by a factor of O(log(n)) operations. All more recent papers about the
combinatorial type prime counting algorithms that I am aware of have also suggested
using the binary indexed tree data structure for counting the number of unsieved
elements in the sieve array.

```C++
// Count the number of unsieved elements <= pos in the
// sieve array using a binary indexed tree.
// Runtime: O(log n).
//
int count(const int* tree, int pos)
{
  int sum = tree[pos++];
  while (pos &= pos - 1)
      sum += tree[pos - 1];
  return sum;
}
```

Despite the theoretical benefits of the binary indexed tree data structure it
has two significant practical drawbacks:
it uses a lot of memory since each thread needs to allocate its own binary indexed
tree and more importantly it is horribly slow because all memory accesses are non
sequential which CPUs are very bad at. For this reason many programmers that have
implemented any of the combinatorial prime counting algorithms
([Christian Bau 2003](http://cs.swan.ac.uk/~csoliver/ok-sat-library/OKplatform/ExternalSources/sources/NumberTheory/ChristianBau/),
[Dana Jacobsen 2013](https://github.com/danaj/Math-Prime-Util), 
[James F. King 2014](https://github.com/jfkingiii/meissel-lehmer),
[Kim Walisch 2014](https://github.com/kimwalisch/primecount)) have avoided using
the binary indexed tree and implemented something else. The method that has turned
out to perform best so far is to get rid of the binary indexed tree data structure
(which speeds up the sieving part of the algorithm by a factor of log(n)) and
count the number of unsieved elements by simply iterating over the sieve array.
There are many known optimizations that can be used to speedup counting e.g.:

* Using the [POPCNT instruction](https://en.wikipedia.org/wiki/SSE4#POPCNT_and_LZCNT)
  in combination with a bit sieve array allows to count many unsieved sieve
  elements using a single instruction. This improves the runtime complexity by a
  large constant factor.
* One can keep track of the total number of unsieved elements that are
  currently in the sieve array as the total number of unsieved elements is
  [used frequently](https://github.com/kimwalisch/primecount/blob/v5.3/src/lmo/pi_lmo5.cpp#L107)
  (once per sieving prime for each segment).
* We can 
  [batch count the number of unsieved elements](https://github.com/kimwalisch/primecount/blob/v5.3/src/lmo/pi_lmo2.cpp#L65)
  in the sieve array for many
  consecutive leaves i.e. instead of starting to count from the beginning of
  the sieve array for each leaf we resume counting from the last sieve index of
  the previous leaf.

This alternative method with the 3 optimizations described above was used in
primecount up to version 5.3 (January 2020). According to my own benchmarks
this method runs up to 3x faster than an implementation that uses the binary
indexed tree data structure. But there is a problem: all of the alternative algorithms that avoid using the
binary indexed tree data structure have a worse runtime complexity. But why are
they faster then? Mainly for two reasons: most memory accesses in the alternative
algorithms are sequential and the ability to count many unsieved elements using
a single instruction improves the runtime complexity by a large constant factor,
in primecount this constant factor is 240. OK, so despite the worse runtime
complexity everything is great?! Unfortunately not, primecount's implementation of
the hard special leaves formula which used the alternative method described above
starts scaling badly above 10^23. For record computations this became a serious
issue e.g. I had initially expected the computation of PrimePi(10^28) to take
about 8 months, however it ended up taking 2.5 years!

The scaling issue is caused by our change to count the number of unsieved elements
by simply iterating over the sieve array. When there are many consecutive leaves
that are close to each other then simply iterating over the sieve array for counting
the number of unsieved elements works great. However when there are very few
consecutive leaves in the current segment our method becomes inefficient. In order
to compute the special leaves we need to sieve up to some limit z. Since we are
using a modified version of the segmented sieve of Eratosthenes the size of the
sieve array will be O(sqrt(z)). This means that if e.g. there is a single leaf in the
current segment we will use O(sqrt(z)) operations to count the number of unsieved
elements in the sieve array (whereas the binary indexed tree would have used only
O(log(z)) operations). This is too much, this deteriorates the runtime complexity
of the algorithm.

So now that we have identified the problem we can think about whether it is possible
to further improve counting of our alternative algorithm by more than a constant factor.
It turns out this is possible and even relatively simple to implement: **We add a
counters array to our sieving algorithm**. The counters array has a size of O(z^(1/4))
and each element of the counters array contains the current count of unsieved elements
in the sieve array for the interval [i * z^(1/4), (i + 1) * z^(1/4)[. Similar to the
algorithm with the binary indexed tree data structure this counters array must
be updated whilst sieving i.e. whenever an element is crossed-off for the first
time in the sieve array we need to decrement the corresponding counter element.
However since we only need to decrement at most 1 counter when crossing of an
element in the sieve array this does not deteriorate the sieving runtime complexity
of the algorithm (unlike the binary indexed tree which deteriorates sieving by a
factor of log(z)). I have to give credit to Christian Bau here who already used
such a counters array back in 2003 however he chose a size of O(n) which does
not improve the runtime complexity.

```C++
// Sieve out a bit from the sieve array and update the
// counters array if the bit was previously 1
is_bit = (sieve[i] >> bit_index) & 1;
total_count -= is_bit;
counters[i >> counters_dist_log2] -= is_bit;
sieve[i] &= ~(1 << bit_index);
```

Now whenever we need to count the number of unsieved elements in the sieve array
we can quickly iterate over the new counters array and sum the counts. We do this
until we are close (≤ O(z^(1/4))) to the limit up to which we need to count.
Once we are close we switch to our old counting method: we simply iterate
over the sieve array and count the number of unsieved elements using the POPCNT
instruction. With this modification we improve the runtime complexity for counting
the number of unsieved elements for a single leaf from O(z^(1/2)) to O(z^(1/4)).

```C++
/// Count 1 bits inside [0, stop]
uint64_t Sieve::count(uint64_t stop)
{
  uint64_t start = prev_stop_ + 1;
  prev_stop_ = stop;

  // Quickly count the number of unsieved elements (in
  // the sieve array) up to a value that is close to
  // the stop number i.e. (stop - value) <= counters_dist.
  // We do this using the counters array, each element
  // of the counters array contains the number of
  // unsieved elements in the interval:
  // [i * counters_dist, (i + 1) * counters_dist[.
  while (counters_stop_ <= stop)
  {
    start = counters_stop_;
    counters_stop_ += counters_dist_;
    counters_count_ += counters_[counters_i_++];
    count_ = counters_count_;
  }

  // Here the remaining distance is relatively small i.e.
  // (stop - start) <= counters_dist, hence we simply
  // count the remaining number of unsieved elements by
  // linearly iterating over the sieve array.
  count_ += count(start, stop);
  return count_;
}
```

Initially when I found this improvement I thought it would fix my particular
scaling issue only up to some large threshold above which the alternative method
would become inefficient again due to its worse runtime complexity. I thought that
the alternative counting method had a runtime complexity of about (number of
special leaves * O(z^(1/4)) since counting the number of unsieved elements for a
single leaf is O(z^(1/4)). However when I measured the average number of
count operations per leaf the number was much lower than expected. It turns out
that batch counting the number of unsieved elements for many consecutive leaves
improves the runtime complexity by more than a constant factor. My measurements
using primecount indicate that the average number of count operations per leaf
grows about as fast as the alpha tuning factor which grows like O(log(x)^3). For
this reason I believe that the improved counting method described above has an
average runtime complexity of O(log(x)^3) per leaf. If this is true then the runtime
complexity of our alternative algorithm would not be worse anymore than the runtime
complexity of the original algorithm with the binary indexed tree. This would
be meaningful as I am not aware of any other algorithm that has the same runtime
complexity as the algorithm with the binary indexed tree and because the binary
indexed tree is practically a performance nightmare. When I implemented the above
alternative counting method in primecount it completely fixed the severe scaling
issue in the computation of the special leaves that had been present in primecount
since the very beginning. Below 10^20 there are no performance improvements,
however above 10^20, the higher you go the more efficient the new method becomes
compared to primecount's old implementation. At 10^25 the new method is already
2x faster. Note that the new method works best with the Deleglise-Rivat and Gourdon
variants of the combinatorial prime counting algorithm as the average distance
between consecutive special leaves is relatively large in those algorithms. In
the Lagarias-Miller-Odlyzko algorithm the average distance between consecutive
special leaves is much smaller so there the new counting method will not improve
performance in practice.

But this is not the end! So far we have focused on improving counting for the case
when there are very few leaves per segment which are far away from each other.
Generally there is a very large number of leaves that are very close to each other
at the beginning of the sieving algorithm and gradually as we sieve up the leaves
become sparser and the distance between the leaves increases. So what we can do is
**start with more but shorter counters and gradually decrease the number of counters
but increase their distance**. We can update the counters size and distance
e.g. at the start of each new segment as the counters have to be reinitialized at
each new segment anyway. The ideal counter distance for the next segment is
```sqrt(average_leaf_distance)```. The only method to accurately calculate the
average leaf distance that I am currently aware of is to sum all leaf distances at
runtime and divide by their number. Since the average leaf distance increases very
slowly we can calculate the average leaf distance of the previous segment and use
that for the next segment. My measurements using primecount indicate that adaptively
resizing the counters further improves counting by more than a constant factor.
Another option that works quite well in practice is to approximately calculate the
average leaf distance using ```average_leaf_distance = sqrt(segment_low)```.

```C++
// Each element of the counters array contains the current
// number of unsieved elements in the interval:
// [i * counters_dist, (i + 1) * counters_dist[.
// Ideally each element of the counters array should
// represent an interval of size:
// min(sqrt(average_leaf_dist), sqrt(segment_size)).
// Also the counter distance should be regularly adjusted
// whilst sieving.
//
void Sieve::allocate_counters(uint64_t segment_low)
{
  uint64_t average_leaf_dist = sqrt(segment_low);
  counters_dist_ = sqrt(average_leaf_dist);
  counters_dist_ = nearest_power_of_2(counters_dist_);
  counters_dist_log2_ = log2(counters_dist_);

  uint64_t counters_size = (sieve_size_ / counters_dist_) + 1;
  counters_.resize(counters_size);
}
```

There are still a few open questions to which I have no answers yet. The most
important one being: What's the runtime complexity of this alternative algorithm?
Unfortunately it is not easy to answer this question as the algorithm
depends on many optimizations all of which improve the runtime complexity by a
tiny factor.
