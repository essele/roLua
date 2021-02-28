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

/*
 * Hack for x86 since we don't actually have any read-only memory
 */

extern void *read_only_list[];
extern int is_read_only(void *p);

#endif
