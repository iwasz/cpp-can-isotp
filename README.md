# TODO
Write a tutorial.

# Using
## Dependencies
Run-time dependencies (libraries bundled with the code in deps directory as git submodules)
* [etl](https://www.etlcpp.com/map.html) - etl_profile.h is required to be available somewhere in your include path. You can copy one from ```test/example``` for starters.
* [GSL](https://github.com/microsoft/GSL)

Unit tests
* fmt


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

# TODOs, changes

- [x] Handle ISO messages that are constrained to less than maximum 4095B allowed by the ISO document.
- [x] Test when ISO message is size-constrained.
- [x] Test etl::vector ISO messages.
- [x] Use valgrind in the unit-test binary from time to time.
- [ ] Maybe optimize, but not so important.
- [x] It is possible to define a TP object without a default address and then use its send method also without an address. This way you are sending a message into oblivion. Have it sorted out.
- [ ] Get rid of all warinigs and c-tidy issues.
- [x] Check if separationTime and blockSize received from a peer is taken into account during sending (check both sides of communication BS is faulty for sure, no flow frame is sent other than first one).
- [x] blockSize is hardcoede to 8 for testiung purposes. Revert to 0.
- [ ] If errors occur during multi frame message receiving, the isoMessage should be removed (eventually, probably some timeouts are mentioned in the ISO). Now it is not possible to receive second message if first has failed to be received entirely.
- [ ] Check if retyurn value from sendFrame is taken into account .
- [x] Implement all types of addressing.
- [ ] Use some beter means of unit testing. Test time dependent calls, maybe use some clever unit testing library like trompeleoleil for mocking.
- [x] Test crosswise connected objects. They should be able to speak to each other.
- [ ] Test this library with python-can-isotp.
- [ ] Add blocking API.
- [ ] Test this api with std::threads.
- [x] Implement FF parameters : BS and STime
- [x] verify with the ISO pdf whether everything is implemented, and what has to be implemented.
- [ ] Redesign API - I (as a user) hate being forced to provide dozens of arguyments upon construction that I don't really care about and do not use them. In this particular case my biggest concern is the create function and callbacks that it takes.
- [x] implement all enums that can be found in the ISO document.
- [x] encapsulate more functionality into CanFrameWrapper. Like gettype, get length, getSerialnumber  etc.
- [ ] Include a note about N_As and NAr timeouts in the README.md. User shall check if sending a single CAN frame took les than 1000ms + 50%. He should return false in that case, true otherwise.
- [ ] address all TODOs in the code.
- [x] get rid of homeberew list, use etl.
- [x] Test instantiation and usage with other CanFrame type
- [ ] Test instantiation and usage with other IsoMessage type
- [ ] Test flow control.
- [ ] Communication services (page 3):
  - [x] N_USData.request (address, message)
  - [ ] N_USData.confirm (address, result) <- request (this above an only this) completed successfully or not.
  - [ ] N_USData_FF.indication (address, LENGTH) <- callback that first frame was received. It tells what is the lenghth of expected message
  - [ ] N_USData.indication (address, Message, result) - after Sf or after multi-can-message. Indicates that new data has arrived.
  - [x] N_ChangeParameter.request (faddress, key, value) - requests a parameter change in peer or locally, I'm not sure.
  - [x] N_ChangeParameter.confirm  (address, key, result).
  - [x] N_ChangeParameter.request and N_ChangeParameter.confirm are optional. Fixed values may be used instead, and this is the way to go I think. They are in fact hardcoded to 0 (best performance) in sendFlowFrame (see comment).
- [x] Parameters (fixed or changeable) are : STmin and BS. Both hardcoded to 0.
- [x] 5.2.3 and 5.2.4 when to send an indication and ff indication.
- [x] enum N_Result
- [ ] Flow control during transmission (chapter 6.3 pages 12, 13, 14).
- [x] Reading BS and STmin from FC frame received aftrer sending FF.
- [x] Waiting for FC between blocks of size BS CAN frames.
- [x] Reading this incoming FC and deciding what to do next.
- [x] If CTS, resume normally,
- [x] WAIT - wait (how much to wait?),
- [x] If WAIT is received more than N_WFTmax times then fail (this also means, that FC can be received few times ina row).
- [x] OVFLW - what to do then?
- [x] Delay of STmin between CFs
- [x] Protocol data units : create some factory and polymorphic types berived from CanFrameWrapper
- [x] Timing
- [x] Addressing
  - [x] Address information is included in every CAN frame whether FF, CF, FC or SF
  - [x] normal (uses arbitration ID, no constraints on the value)
     - [x] 11b
     - [x] 29b
  - [x] normal fixed (uses arbitration ID, further requirements as to how to encode this address into the arbitration ID). 29b only
     - [x] physical
     - [x] functional
  -  [x] extended. Like normal, but first data byte contains targetAddress, which is absent in arbitration ID.
     - [x] 11b
     - [x] 29b
  -  [x] mixed
     - [ ] 11
     - [ ] 29
  - [x] physical
  - [x] functional
- [ ] Unexpected N_PDU
-  [ ] implent (if not imlenmented already)
-  [ ] test
- [x] Get rid of dynamic allocation, because there is one.
- [ ] Get rid of non English comments.
