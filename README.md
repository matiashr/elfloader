elfloader
=========

This is an attempt to implement a elfloader for chibios( or actually any OS/osless)
for ARM. Im using a stm32F4 discovery board to develop this.


To build a dynamic linked program, one needs to have a library to link against,
this could be accomplished by compiling chibios as a lib, and linking it with the application.

But for simplicity (or stupidity) ive created a sample lib (shared library with dummy functions, intention is only to have the symbols)
I then link the application with with this lib.
During loading, I will then resolv the symbols to their correct locations, thus the "dummy" library will never be used.
This could ofcourse be done better later.

in the loader the resolver corrently uses hardcoded symbols, pstr(), listTasks(), this means that only these symbols will resolv,
later a real symboltable should be included in the OS.

Currently it seems that a static binary can be loaded and called, only tested one example, it's in nonlibexample directory
this example will load and can be executed, but this is ofcourse not enough as static linking would case a very large binary
