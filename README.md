# roLua - a verion of Lua enhanced for small embedded systems

This is standard lua5.4.2 with enhancements that move most of the strings into ROM so that the runtime memory usage is significantly smaller than normal.
This is heavily inspired by eLua (TODO: link) but focusing on making the changes to the standard distribution as minimal as possible.

On a 64bit x86 system the memory usage with all relevant libs loaded went down from over 20K to less than 5K.

More description to follow...



Running pairs on a read-only table...

We catch this is lua_next and just act as though there are no items in the table ... why would you need to do this anyway?

Running pairs on the globable table...

So you only see the non-read-only items in the global table, it's just too awkward to have pairs work through the different mechanisms in a consistent way. Again, this is for embedded use, why do you need to see all the real globals?

Changing a read-only global...

So this is a little counter intuitive ... you can set the value of "print" to something else for example, and it will create a new entry in the globals table (and use memory), but it won't have any effect and any future globals lookup will favour the read-only one.

Setting a field in a read-only table...

If it's an existing field, then luaV_fastget is used, which returns a static TValue entry, this is then quickly followed by a luaV_finishfastset() which just updates that TValue (which is irrelevant, so this silently fails ... which is good.

If it's a new field, then luaV_fastget doesn't work, so we drop back to luaV_finishset() .. so here we catch any read only table and just silently exit.

Setting a metatable on a read-only table...

Obviously can't be done ... so we catch it in lapi.c (setmetatable) and just silently exit
