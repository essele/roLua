# roLua - a verion of Lua enhanced for small embedded systems

This is standard lua5.4.2 with enhancements that move most of the strings into ROM so that the runtime memory usage is significantly smaller than normal.
This is heavily inspired by eLua (TODO: link) but focusing on making the changes to the standard distribution as minimal as possible.

On a 64bit x86 system the memory usage with all relevant libs loaded went down from over 20K to less than 5K.

More description to follow...
