# sprotoc

## Synopsis

``` sh
sudo aptitude install libprotoc-dev

git clone https://github.com/frobnitzem/sprotoc
cd sprotoc
PROTO=$PWD
make
make tests

cd $MY_SRCDIR
sprotoc --c_out=stubs=test:. [defs.proto]
gcc -I$PROTO/include $PROTO/sprotoc/*.c -o test test.c
```

## The Why

  This is a lightweight C-code generator for Google protocol buffers.
It was created out of frustration with the requirement for using
auto-generated structs to hold pb-related metadata (`len..., has...`, etc.).
Other than default values, it doesn't support most field property
tags or other bells and whistles.

  The generated code only makes use of memory allocation (malloc) when
determining the size of the objects it is serializing.
For everything else, it creates temporary structures
to serialize from (to parse into) on the stack.
This allows you to use your own struct data definitions
and only send lightweight pointers back and forth with protocol
buffers.  It also means there is only a single copy (or none at all)
between the protocol buffer output / input and your actual data.

## The How

  The top-level calls are generated as, e.g.
``` c
uint8_t *[package]_[message]_to_string(
      size_t *len, MY_[package]_[message] *a)
```
and
``` c
MY_[package]_[message] *read_[package]_[message](
      const uint8_t **buf, ssize_t sz, void *info);
```
(info is optional, and passed for your use)

  To use this serialization method effectively requires writing
reader and writer functions for each message type
(`protord..., and protowr...`).
The writer has access to both your structure and protobuf's
generic structure (with variable names / types
auto-generated from the .proto file).  The reader
has access to protobuf's auto-generated structure and a user
pointer (used for malloc purposes).  It should return
a pointer to your structure.

  Of course, the messages can hold other messages.  To serialize
these, you need only fill that field of the protobuf structure
with a pointer to your own child structure (`MY_...`).
The library arranges the rest.

  On the read-side, the library parses children first so that
all the protobuf messages you read out already have your parsed
output data filled in.  Unfortunately, it is not currently
possible to skip parsing parts of a message.

### Lazy Byte Serialization
  One convention that deserves comment is the method of serializing 
fields with type `bytes`.  These are serialized by filling in
the protobuf structure with the function,
``` c
  typedef void (*lazy_writer)(SWriter *print, void *buf, size_t len);
```
The writer is then only called when writes are actually needed.

the SWriter is defined in sprotoc.h, and is used like:
  print->write(print->stream, buf, len);

## Features

1. A complete example serialize / de-serialize program can
be generated for you.  To do this, pass the stubs
parameter to sprotoc:
``` sh
sprotoc --c_out=stubs=stub:. test1.proto
```
Building your copy functions by modifying these stubs
will save you from having to look up _any_ of the naming conventions.

2. This implementation is as close to zero-copy as anyone could
reasonably want.  Only the numbers in each message are copied into
stack-space - not heap space.  Strings / bytes / sub-messages
are kept as lightweight pointers everywhere.  The trade-off
is that repeated sub-fields are limited to MAX_REPEATED
elements.  You define MAX_REPEATED in your own [prefix].pb-c.h.

