/* vi: set sw=4 ts=4: */
/*
 * Busybox main internal header file
 *
 * Based in part on code from sash, Copyright (c) 1999 by David I. Bell
 * Permission has been granted to redistribute this code under GPL.
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
#ifndef LIBBB_H
#define LIBBB_H 1

#define _GNU_SOURCE /* See feature_test_macros(7) */
#include "platform.h"
#include "autoconf.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <paths.h>
#if defined __UCLIBC__ /* TODO: and glibc? */
/* use inlined versions of these: */
#define sigfillset(s) __sigfillset(s)
#define sigemptyset(s) __sigemptyset(s)
#define sigisemptyset(s) __sigisemptyset(s)
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
/* There are two incompatible basename's, let's not use them! */
/* See the dirname/basename man page for details */
#include <libgen.h> /* dirname,basename */
#undef basename
#define basename dont_use_basename
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#if !defined(major) || defined(__GLIBC__)
#include <sys/sysmacros.h>
#endif
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <sys/param.h>
#include <pwd.h>
#include <grp.h>
#include <arpa/inet.h>

/* Some useful definitions */
#undef FALSE
#define FALSE ((int)0)
#undef TRUE
#define TRUE ((int)1)
#undef SKIP
#define SKIP ((int)2)

/* Macros for min/max.  */
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define ARRAY_SIZE(x) ((unsigned)(sizeof(x) / sizeof((x)[0])))

#define vi_main main
#define applet_name "vi"
#define BB_VER "1.33.2"

/* "Keycodes" that report an escape sequence.
 * We use something which fits into signed char,
 * yet doesn't represent any valid Unicode character.
 * Also, -1 is reserved for error indication and we don't use it. */
enum {
    KEYCODE_UP = -2,
    KEYCODE_DOWN = -3,
    KEYCODE_RIGHT = -4,
    KEYCODE_LEFT = -5,
    KEYCODE_HOME = -6,
    KEYCODE_END = -7,
    KEYCODE_INSERT = -8,
    KEYCODE_DELETE = -9,
    KEYCODE_PAGEUP = -10,
    KEYCODE_PAGEDOWN = -11,
    KEYCODE_BACKSPACE = -12, /* Used only if Alt/Ctrl/Shifted */
    KEYCODE_D = -13,         /* Used only if Alted */
#if 0
	KEYCODE_FUN1      = ,
	KEYCODE_FUN2      = ,
	KEYCODE_FUN3      = ,
	KEYCODE_FUN4      = ,
	KEYCODE_FUN5      = ,
	KEYCODE_FUN6      = ,
	KEYCODE_FUN7      = ,
	KEYCODE_FUN8      = ,
	KEYCODE_FUN9      = ,
	KEYCODE_FUN10     = ,
	KEYCODE_FUN11     = ,
	KEYCODE_FUN12     = ,
#endif
    /* ^^^^^ Be sure that last defined value is small enough.
	 * Current read_key() code allows going up to -32 (0xfff..fffe0).
	 * This gives three upper bits in LSB to play with:
	 * KEYCODE_foo values are 0xfff..fffXX, lowest XX bits are: scavvvvv,
	 * s=0 if SHIFT, c=0 if CTRL, a=0 if ALT,
	 * vvvvv bits are the same for same key regardless of "shift bits".
	 */
    // KEYCODE_SHIFT_...   = KEYCODE_...   & ~0x80,
    KEYCODE_CTRL_RIGHT = KEYCODE_RIGHT & ~0x40,
    KEYCODE_CTRL_LEFT = KEYCODE_LEFT & ~0x40,
    KEYCODE_ALT_RIGHT = KEYCODE_RIGHT & ~0x20,
    KEYCODE_ALT_LEFT = KEYCODE_LEFT & ~0x20,
    KEYCODE_ALT_BACKSPACE = KEYCODE_BACKSPACE & ~0x20,
    KEYCODE_ALT_D = KEYCODE_D & ~0x20,

    KEYCODE_CURSOR_POS = -0x100, /* 0xfff..fff00 */
    /* How long is the longest ESC sequence we know?
	 * We want it big enough to be able to contain
	 * cursor position sequence "ESC [ 9999 ; 9999 R"
	 */
    KEYCODE_BUFFER_SIZE = 16
};

/* This struct is deliberately not defined. */
/* See docs/keep_data_small.txt */
struct globals;
/* '*const' ptr makes gcc optimize code much better.
 * Magic prevents ptr_to_globals from going into rodata.
 * If you want to assign a value, use SET_PTR_TO_GLOBALS(x) */
extern struct globals *const ptr_to_globals;
#if defined(__clang_major__) && __clang_major__ >= 9
/* Clang/llvm drops assignment to "constant" storage. Silently.
 * Needs serious convincing to not eliminate the store.
 */
static ALWAYS_INLINE void *not_const_pp(const void *p) {
    void *pp;
    __asm__ __volatile__("# forget that p points to const" : /*outputs*/ "=r"(pp) : /*inputs*/ "0"(p));
    return pp;
}
#else
static ALWAYS_INLINE void *not_const_pp(const void *p) {
    return (void *)p;
}
#endif

/* At least gcc 3.4.6 on mipsel system needs optimization barrier */
#define barrier() __asm__ __volatile__("" ::: "memory")
#define SET_PTR_TO_GLOBALS(x)                                              \
    do {                                                                   \
        (*(struct globals **)not_const_pp(&ptr_to_globals)) = (void *)(x); \
        barrier();                                                         \
    } while (0)

#define FREE_PTR_TO_GLOBALS()          \
    do {                               \
        if (ENABLE_FEATURE_CLEAN_UP) { \
            free(ptr_to_globals);      \
        }                              \
    } while (0)

#ifdef HAVE_PRINTF_PERCENTM
#define STRERROR_FMT "%m"
#define STRERROR_ERRNO /*nothing*/
#else
#define STRERROR_FMT "%s"
#define STRERROR_ERRNO , strerror(errno)
#endif

/* We need to export XXX_main from libbusybox
 * only if we build "individual" binaries
 */
#if ENABLE_FEATURE_INDIVIDUAL
#define MAIN_EXTERNALLY_VISIBLE EXTERNALLY_VISIBLE
#else
#define MAIN_EXTERNALLY_VISIBLE
#endif

// verror_msg.c
enum {
    LOGMODE_NONE = 0,
    LOGMODE_STDIO = (1 << 0),
    LOGMODE_SYSLOG = (1 << 1) * ENABLE_FEATURE_SYSLOG,
    LOGMODE_BOTH = LOGMODE_SYSLOG + LOGMODE_STDIO,
};
void bb_verror_msg(const char *s, va_list p, const char *strerr) FAST_FUNC;
void bb_error_msg(const char *s, ...) FAST_FUNC;
void bb_error_msg_and_die(const char *s, ...) FAST_FUNC;
void bb_simple_error_msg(const char *s) FAST_FUNC;
void bb_simple_error_msg_and_die(const char *s) FAST_FUNC;
void xfunc_die(void) FAST_FUNC;

// xfuncs.c
#define TERMIOS_CLEAR_ISIG (1 << 0)
#define TERMIOS_RAW_CRNL_INPUT (1 << 1)
#define TERMIOS_RAW_CRNL_OUTPUT (1 << 2)
#define TERMIOS_RAW_CRNL (TERMIOS_RAW_CRNL_INPUT | TERMIOS_RAW_CRNL_OUTPUT)
#define TERMIOS_RAW_INPUT (1 << 3)
ssize_t safe_write(int fd, const void *buf, size_t count) FAST_FUNC;
ssize_t full_write(int fd, const void *buf, size_t len) FAST_FUNC;
int get_terminal_width_height(int fd, unsigned *width, unsigned *height) FAST_FUNC;
int get_terminal_width(int fd) FAST_FUNC;
int tcsetattr_stdin_TCSANOW(const struct termios *tp) FAST_FUNC;
int get_termios_and_make_raw(int fd, struct termios *newterm, struct termios *oldterm, int flags) FAST_FUNC;
int set_termios_to_raw(int fd, struct termios *oldterm, int flags) FAST_FUNC;
int safe_poll(struct pollfd *ufds, nfds_t nfds, int timeout) FAST_FUNC;

// xfuncs_printf.c
#ifdef DMALLOC
#include <dmalloc.h>
#endif
void *xmalloc(size_t size) FAST_FUNC RETURNS_MALLOC;
void *xzalloc(size_t size) FAST_FUNC RETURNS_MALLOC;
void *xrealloc(void *old, size_t size) FAST_FUNC;
char *xstrdup(const char *s) FAST_FUNC RETURNS_MALLOC;
char *xstrndup(const char *s, int n) FAST_FUNC RETURNS_MALLOC;
int fflush_all(void) FAST_FUNC;
int bb_putchar(int ch) FAST_FUNC;

// read.c
ssize_t safe_read(int fd, void *buf, size_t count) FAST_FUNC;
ssize_t full_read(int fd, void *buf, size_t len) FAST_FUNC;

// read_key.c
int64_t read_key(int fd, char *buffer, int timeout) FAST_FUNC;
char *last_char_is(const char *s, int c) FAST_FUNC;
char *skip_whitespace(const char *s) FAST_FUNC;
char *skip_non_whitespace(const char *s) FAST_FUNC;

#endif
