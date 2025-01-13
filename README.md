# What it is
This library implements [ISO 15765-2](https://en.wikipedia.org/wiki/ISO_15765-2) transport layer known also as CAN bus ISO TP or simply *Transport Protocol*. It was developed with microcontrollers in mind, but was also tested on a Linux box (although on a Linux system there is a [better option](https://github.com/hartkopp/can-isotp)). 

## Notes
Whenever I refer to some cryptic paragraph numer in this document or in the source code I have **ISO 15765-2 First edition (2004-10-15)** on mind. This PDF id widely available in the Net though I'm not sure about lagality of this stuff, so I'm not including it in the repo. Normally an ISO document like that costs around $150.

This library is influenced by and was tested against [python-can-isotp](https://github.com/pylessard/python-can-isotp).

# Using the library
## Building
This library is header only, but you can build unit-tests and other simple examples, which can help you understand how to use the library in case of some trouble. This is a possible scenario of acheiving this:

```sh
git clone --recurse-submodules git@github.com:iwasz/cpp-can-isotp.git
mkdir -p cpp-can-isotp/build
cd cpp-can-isotp/build
cmake -GNinja ..
ninja

# Run the tests
test/unit-test/unit-test
```
In case of trouble with updating submodules, refer to [this stack overflow question](https://stackoverflow.com/questions/1030169/easy-way-to-pull-latest-of-all-git-submodules) (like I do everytime I deal with this stuff :D).

## Dependencies
All dependencies are provided as git submodules:

Run-time dependencies (libraries bundled with the code in deps directory as git submodules)
* [etl](https://www.etlcpp.com/map.html) - etl_profile.h is required to be available somewhere in your include path. You can copy one from ```test/example``` for starters.
* [GSL](https://github.com/microsoft/GSL)
* C++17 (```if constexpr```).

Unit tests
* [Catch2](https://github.com/catchorg/Catch2)
* [fmt](https://github.com/fmtlib/fmt)

## Using in your project
Just

```cpp
#include "TransportProtocol.h"
```

## Instantiating
First you have to instantiate the ```TransportProtocol``` class which encapsulates the protocol state. TransportProtocol is a class template and can be customized depending on underlying CAN-bus implementation, memory constraints, error (exception) handling sheme and time related stuff. This makes the API of TransportProtocol a little bit verbose, but also enables one to use it on a *normal* computer (see ```socket-test``` for Linux example) as well as on microcontrollers which this library was meant for at the first place. 

```cpp
#include "LinuxCanFrame.h"
#include "TransportProtocol.h"

/// ...

using namespace tp;
int socketFd = createSocket (); // (1)

auto tp = create<can_frame> ( // (2)
         Address{0x789ABC, 0x123456}, // (3)
         [] (auto const &iso) { fmt::print ("Message size : {}\n", iso.size ()); }, // (4)
         [socketFd] (auto const &frame) { 
               if (!sendSocket (socketFd, frame)) { // (5)
                        fmt::print ("Error\n");
                        return false;
               }

               return true;
         });

listenSocket (socketFd, [&tp] (auto const &frame) { tp.onCanNewFrame (frame); }); // (6)
```
The code you see above is more or less all that's needed for **receiving** ISO-TP messages. In (1) we somehow connect to the underlying CAN-bus subsystem and then, using ```socketFd``` we are able to send and receive raw CAN-frames (see examples). 

## Callbacks
Callback is the second parameter to ```create``` function, and it can have 3 different forms. The simplest (called *simple* througout this document and the source code) is :

```cpp
void indication (tp::IsoMessage const &msg) { /* ... */ }

// Example usage:
auto tp = tp::create<can_frame> (tp::Address{0x789ABC, 0x123456}, indication, socketSend);
```

This one implements ```N_USData.indication``` (see par. 5.2.4)  and gets called whan new ISO message was successfulty assembled, or in case of an error to inform the user, that current ISO message could not be assembled fully. In the latter case, the ```msg``` argument will be empty, but no other further information on the error will be available. Of course name ```indication``` is used here as an example, and even a lambda can be used (in fact in unit tests lambdas are used almost exclusively).

The next one is called an *advanced* callback, because it has more parameters:

```cpp
void indication (tp::Address const &a, tp::IsoMessage const &msg, tp::Result res) { /* ... */ }
```

Meaning of the parameters is :

1. Address ```a``` is the address of a peer who sent the message. Depending on the addressing scheme ```a.getTxId ()``` or ```a.getTargetAddress ()``` will be of interest to the user. See paragraph on addresses.
1. Message ```msg``` is the ISO message (in case of a success) or empty if there was an error.
1. Result ```res``` will have value ```Result::N_OK``` in case of a success or something other if case of a failed transmission.

Lastly there is *advancedMethod* or *full* callback (again, those are the terms used in the unit tests and througout the comments in the source) which not only implements ```N_USData.indication``` but also ```N_USData.confirm``` and N_USData_FF.indication:

```cpp
class FullCallback {
public:
        void indication (tp::Address const &address, std::vector<uint8_t> const &isoMessage, tp::Result result) {}
        void confirm (tp::Address const &address, tp::Result result) {}
        void firstFrameIndication (tp::Address const &address, uint16_t len) {}
};

// Example usage:
auto tp = tp::create<can_frame> (tp::Address{0x789ABC, 0x123456}, FullCallback (), socketSend);
```

# Addressing
Addressing is somewhat vaguely described in the 2004 ISO document I have, so the best idea I had (after long head scratching) was to mimic the python-can-isotp library which I test my library against. In this API an address has a total of 5 numeric values representing various addresses, and another two types (target address type N_TAtype and the Mtype which stands for **TODO I forgot**). These numeric properties of an address object are:
* rxId
* txId
* sourceAddress
* targetAddress
* networkAddressExtension

The important thing to note here is that not all of them are used at the same time but rather, depending on addressing encoder (i.e. one of the addressing modes) used, a subset is used. 

# Platform speciffic remarks
## Arduino
I successfully ported and tested this library to Arduino (see examples directory). Currently only [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) CAN implementation is supported. Be sure to experiment with separation time (use ```TransportProtocol::setSeparationTime```) to be sure that your board can keep up with receiving fast CAN frames bursts.

# (part of the tutorial)
ISO messages can be moved, copied or passed by reference_wrapper (std::ref) if move semantics arent implemented for your ISO message class.

# Design decissions
* I don't use exceptions because on a Coretex-M target enabling them increased the binary size by 13kB (around 10% increase). I use error codes (?) in favour of a error handler only because cpp-core-guidelines doest that.

# TODOs, changes
- [x] NO! Even when sending we must be able to receive a flow control frame. make specialization for void (and/or) 0-sized isoMessages. Such an implementation would be able to send only.
- [ ] Describe (in this README) various callback options!
- [ ] Once again rethink ```TransportProtocol::send``` interface. Previously it had pass-by-value agrgument, now I switched (switched back?) to universal-reference. 
- [ ] Make section here in the README about passing IsoMessages to ```TransportProtocol::send```. Show lvl, rvr (use std::move explicitly to make a point), and std::ref.
- [ ] Test errorneus sequences of canFrames during assemblying segemnted messages. Make an unit test of that. For example on slower receivers some can frames can be lost, and this fact should be detected and reported to the user. 
- [ ] Add Stm32 example.
- [ ] Move ```example``` from test to root, rename to ```examples```. Add Ardiono example with ino extension.
- [ ] Extend Linux example, implement CAN interface properly using boost::asio.
- [ ] Add Arduino example 
- [ ] Add an introduction to TP addressing in this README.
- [x] Handle ISO messages that are constrained to less than maximum 4095B allowed by the ISO document.
- [x] Test when ISO message is size-constrained.
- [x] Test etl::vector ISO messages.
- [x] Use valgrind in the unit-test binary from time to time.
- [ ] Maybe optimize, but not so important.
- [x] It is possible to define a TP object without a default address and then use its send method also without an address. This way you are sending a message into oblivion. Have it sorted out.
- [ ] Get rid of all warinigs and c-tidy issues.
- [x] Check if separationTime and blockSize received from a peer is taken into account during sending (check both sides of communication BS is faulty for sure, no flow frame is sent other than first one).
- [x] blockSize is hardcoede to 8 for testiung purposes. Revert to 0.
- [ ] If errors occur during multi frame message receiving, the isoMessage should be removed (eventually. Probably some timeouts are mentioned in the ISO). Now it is not possible to receive second message if first has failed to be received entirely.
- [x] Check if retyurn value from sendFrame is taken into account .
- [x] Implement all types of addressing.
- [ ] Use some better means of unit testing. Test time dependent calls, maybe use some clever unit testing library like trompeleoleil for mocking.
- [x] Test crosswise connected objects. They should be able to speak to each other.
- [x] Test this library with python-can-isotp.
- [ ] Add blocking API.
- [ ] Test this api with std::threads.
- [x] Implement FF parameters : BS and STime
- [x] verify with the ISO pdf whether everything is implemented, and what has to be implemented.
- [ ] Redesign API - I (as a user) hate being forced to provide dozens of arguyments upon construction that I don't really care about and do not use them. In this particular case my biggest concern is the create function and callbacks that it takes.
- [x] Implement all enums that can be found in the ISO document.
- [x] ~~Encapsulate more functionality into CanFrameWrapper. Like gettype, get length, getSerialnumber  etc.~~
- [ ] (?) Include a note about N_As and NAr timeouts in the README.md. User shall check if sending a single CAN frame took les than 1000ms + 50%. He should return false in that case, true otherwise.
- [ ] Address all TODOs in the code.
- [x] Get rid of homeberew list, use etl.
- [x] Test instantiation and usage with other CanFrame type
- [x] Test instantiation and usage with other IsoMessage type
- [x] Test flow control.
- [ ] Communication services (page 3):
  - [x] N_USData.request (address, message)
  - [x] N_USData.confirm (address, result) <- request (this above an only this) completed successfully or not.
  - [X] N_USData_FF.indication (address, LENGTH) <- callback that first frame was received. It tells what is the lenghth of expected message
  - [x] N_USData.indication (address, Message, result) - after Sf or after multi-can-message. Indicates that new data has arrived.
  - [x] N_ChangeParameter.request (faddress, key, value) - requests a parameter change in peer or locally, I'm not sure.
  - [x] N_ChangeParameter.confirm  (address, key, result).
  - [x] N_ChangeParameter.request and N_ChangeParameter.confirm are optional. Fixed values may be used instead, and this is the way to go I think. They are in fact hardcoded to 0 (best performance) in sendFlowFrame (see comment).
- [x] Parameters (fixed or changeable) are : STmin and BS. Both hardcoded to 0.
- [x] 5.2.3 and 5.2.4 when to send an indication and ff indication.
- [x] enum N_Result
- [X] Flow control during transmission (chapter 6.3 pages 12, 13, 14).
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
     - [x] 11
     - [x] 29
  - [x] physical
  - [x] functional
- [ ] Unexpected N_PDU
-  [ ] implent (if not imlenmented already)
-  [ ] test
- [x] Get rid of dynamic allocation, because there is one.
- [ ] Get rid of non English comments.
- [ ] Finish this TODO
- [ ] Calls like this : ```indication (*theirAddress, {}, Result::N_UNEXP_PDU);``` are potentially inefficient if IsoMessageT internally allocates on the stack (etl::vector for example). Possible solution would be to pass by pointer and pass nullptr in such cases, but that would be inconvenient for the end user.
- [ ] ~~Problem with sequence numbers. Sometimes python can-isotp library would dump the following error message (sequence numbers are various)~~ This was a problem on the other part of the connection. Python app has had synchronisation problems:

INFO:root:Preparing the telemetry...
WARNING:root:IsoTp error happened : WrongSequenceNumberError - Received a ConsecutiveFrame with wrong SequenceNumber. Expecting 0x2, Received 0x3
WARNING:root:IsoTp error happened : UnexpectedConsecutiveFrameError - Received a ConsecutiveFrame while reception was idle. Ignoring
WARNING:isotp:Received a ConsecutiveFrame with wrong SequenceNumber. Expecting 0x2, Received 0x3
WARNING:isotp:Received a ConsecutiveFrame while reception was idle. Ignoring
WARNING:root:Device thermometer_1 is UNSTABLE
ERROR:root:Exception in prepareTelemetry task
Traceback (most recent call last):
  File "/home/iwasz/workspace/nbox/nbox-raspi/src/telemetry.py", line 82, in prepareTelemetry
    await aggregateStore()
  File "/home/iwasz/workspace/nbox/nbox-raspi/src/engine.py", line 34, in aggregateStore
    cachedValues = await cacheValuesForDevice(
  File "/home/iwasz/workspace/nbox/nbox-raspi/src/engine.py", line 187, in cacheValuesForDevice
    value = await device.request(deviceName, propertyName)
  File "/home/iwasz/workspace/nbox/nbox-raspi/src/device/device.py", line 55, in request
    return await getattr(self, command)()
  File "/home/iwasz/workspace/nbox/nbox-raspi/src/device/external/oneWireSensors.py", line 70, in getTemperature
    data = await onewire.readBytes(self.port, 9)
  File "/home/iwasz/workspace/nbox/nbox-raspi/src/core/onewire.py", line 68, in readBytes
    decoded = await messagePack.measurementsRequest(
  File "/home/iwasz/workspace/nbox/nbox-raspi/src/core/messagePack.py", line 49, in measurementsRequest
    return await asyncio.wait_for(
  File "/usr/lib/python3.8/asyncio/tasks.py", line 490, in wait_for
    raise exceptions.TimeoutError()
asyncio.exceptions.TimeoutError
INFO:root:Sending current telemetry...

# License
[MIT](https://opensource.org/licenses/MIT)

Copyright 2021 Łukasz Iwaszkiewicz

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.