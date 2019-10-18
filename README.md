# TODO
Write a tutorial.

# (part of the tutorial)
ISO messages can be moved, copied or passed by reference_wrapper (std::ref) if move semantics arent implemented for your ISO message class.

# Design decissions
* I don't use exceptions because on a Coretex-M target enabling them increased the binary size by 13kB (around 10% increase). I use error codes (?) in favour of a error handler only because cpp-core-guidelines doest that.

