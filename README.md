# TODO
Write a tutorial.

# Addressing
Addressing is somewhat vaguely described in the 2004 ISO document I have, so the best idea I had (after long gead scratching) was to mimic the python-can-isotp library which I test my library against. In this API an address has a total of 5 numeric values representing various addresses, and another two types (target address type N_TAtype and the Mtype which stands for **TODO I forgot**). These numeric properties of an address object are:
* rxId
* txId
* sourceAddress
* targetAddress
* networkAddressExtension

The important thing to note here is that not all of them are used at the same time but rather, depending on addressing encoder (i.e. one of the addressing modes) used, a subset is used. 



# (part of the tutorial)
ISO messages can be moved, copied or passed by reference_wrapper (std::ref) if move semantics arent implemented for your ISO message class.

# Design decissions
* I don't use exceptions because on a Coretex-M target enabling them increased the binary size by 13kB (around 10% increase). I use error codes (?) in favour of a error handler only because cpp-core-guidelines doest that.

