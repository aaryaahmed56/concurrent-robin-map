# concurrent-robin-map

A C++ library implementing a hash map designed with concurrent robin hood hashing, as detailed in
a [paper](https://arxiv.org/pdf/1911.03028.pdf) by Kelley, Pearlmutter, and Maguire, while
maintaining as much faithfulness as possible to the `std::unordered_map` interface.

## References

<table style="border:0px">
<tr>
    <td valign="top"><a name="ref-brown-2015"></a>[KPM2018]</td>
    <td>Robert Kelley, Barak A. Pearlmutter, and Phil Maguire.
    <a href=https://arxiv.org/pdf/1809.04339.pdf>
    Concurrent Robin Hood Hashing</a>.
    In <i>22nd International Conference on Principles of Distributed Systems, 2018.</td>
</tr>
</table>
