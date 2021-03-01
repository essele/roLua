/**
 * Specific include file to support the small number of platform specific
 * needs.
 *
 * Currently:
 *
 * 1. Be able to identify whether an item (void *) is in read-only memory
 *    or not.
 *
 */

#ifndef platform_h
#define platform_h

#include "src/lapi.h"
#include "src/lauxlib.h"

/*
 * Hack for x86 since we don't actually have any read-only memory
 */

extern void *read_only_list[];
int is_read_only(void *p);

int read_only_lookup(StkId ra, TValue *t, TValue *f);

int global_lookup(StkId ra, char *key);

void set_baselib(const luaL_Reg *lib, unsigned int count);

TString *read_only_string(const char *str, size_t len);

#endif
