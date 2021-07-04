An attempt was made to isolate the api between the laters.


schedule format
-----

Picks what timeslots and how many words to send, nothing else.



arbiter js
----
Wakes up on a timer as best as possible in js scripting world.  Asks schedule what to do.  remembers.  asks and tells cpp when to wake up and what to send



arbiter c++
----
watches tun data and tells up layer how many words there are





arbiter js pulls packets out of tun but does not add the hash in
they are sent to txschedule with re-pulls out of "tun" but adds the hashes this time
right before they are sent into airpacket and over the air




Seq
====
Js chooses seq
gets loaded into zmq packet
tx schedule unpacks this packet and "hack" assigns into airtx
pullandprep then calls air packet to "choose" the next seq, but it was hacked

