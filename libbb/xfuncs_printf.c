/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 * Copyright (C) 2006 Rob Landley
 * Copyright (C) 2006 Denys Vlasenko
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
/* We need to have separate xfuncs.c and xfuncs_printf.c because
 * with current linkers, even with section garbage collection,
 * if *.o module references any of XXXprintf functions, you pull in
 * entire printf machinery. Even if you do not use the function
 * which uses XXXprintf.
 *
 * xfuncs.c contains functions (not necessarily xfuncs)
 * which do not pull in printf, directly or indirectly.
 * xfunc_printf.c contains those which do.
 */
#include "libbb.h"

/* All the functions starting with "x" call bb_error_msg_and_die() if they
 * fail, so callers never need to check for errors.  If it returned, it
 * succeeded. */

static inline void FAST_FUNC bb_die_memory_exhausted(void) {
    bb_simple_error_msg_and_die("out of memory");
}

#ifndef DMALLOC
// Die if we can't allocate size bytes of memory.
void *FAST_FUNC xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL && size != 0)
        bb_die_memory_exhausted();
    return ptr;
}

// Die if we can't resize previously allocated memory.  (This returns a pointer
// to the new memory, which may or may not be the same as the old memory.
// It'll copy the contents to a new chunk and free the old one if necessary.)
void *FAST_FUNC xrealloc(void *ptr, size_t size) {
    ptr = realloc(ptr, size);
    if (ptr == NULL && size != 0)
        bb_die_memory_exhausted();
    return ptr;
}
#endif /* DMALLOC */

// Die if we can't allocate and zero size bytes of memory.
void *FAST_FUNC xzalloc(size_t size) {
    void *ptr = xmalloc(size);
    memset(ptr, 0, size);
    return ptr;
}

// Die if we can't copy a string to freshly allocated memory.
char *FAST_FUNC xstrdup(const char *s) {
    char *t;

    if (s == NULL)
        return NULL;

    t = strdup(s);

    if (t == NULL)
        bb_die_memory_exhausted();

    return t;
}

// Die if we can't allocate n+1 bytes (space for the null terminator) and copy
// the (possibly truncated to length n) string into it.
char *FAST_FUNC xstrndup(const char *s, int n) {
    char *t;

    if (ENABLE_DEBUG && s == NULL)
        bb_simple_error_msg_and_die("xstrndup bug");

    t = strndup(s, n);

    if (t == NULL)
        bb_die_memory_exhausted();

    return t;
}

int FAST_FUNC fflush_all(void) {
    return fflush(NULL);
}

int FAST_FUNC bb_putchar(int ch) {
    return putchar(ch);
}