Test Air Packet 1
====
* Modulates, emulate HiggsToHiggs and then demod
  * With Reed Solomon
* Test vectorJoin, vectorSplit
*

Test Air Packet 2
====
* Modulates, emulate HiggsToHiggs and then demod Reed Solomon


Test Air Packet 3
====
* Tests "recovery"  This means that we load a packet off disk with a known bad block
* Per packet hash's are read, and this function tries to scan foward to find the next valid hash



Test Air Packet 7
====
* test AirPacket's header
* test new_subcarrier_data_load() which seems to be a dead function



Test Interleaver
=====
* Test code length 255
* Test some word / size converting functions
* Test code length 8
* Test random code length
* Test random code length with random "stream" length:
  * "stream" interface uses
  * interleave()
  * deinterleave()