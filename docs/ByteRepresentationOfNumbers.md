Byte Representation Of Numbers
======


IShort
===
Width is uint32_t.  it means two int16_t packed in.

```
0xrrrriiii
```


* IShortToComplexMulti x3
* ishort_to_double
* ishort_to_parts
* ishort_to_parts_swapped
* ishort_to_angle
* angle_to_ishort
* gain_ishort
* gain_ishort_vector
* complexToIShortMulti x3
* complexConjMult
* complexMult
* riscvATAN


cf64
====
Complex double precision floats.  Each double is 64 bits.  Order is
* r,i

So a vector would look like:
* r,i,r,i,r,i,r,i  ...




Double
===
Usually pairs of double precision floats.  