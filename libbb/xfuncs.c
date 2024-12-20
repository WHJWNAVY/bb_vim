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
 *
 * TODO: move xmalloc() and xatonum() here.
 */
#include "libbb.h"

ssize_t FAST_FUNC safe_write(int fd, const void *buf, size_t count) {
    ssize_t n;

    for (;;) {
        n = write(fd, buf, count);
        if (n >= 0 || errno != EINTR)
            break;
        /* Some callers set errno=0, are upset when they see EINTR.
		 * Returning EINTR is wrong since we retry write(),
		 * the "error" was transient.
		 */
        errno = 0;
        /* repeat the write() */
    }

    return n;
}

/*
 * Write all of the supplied buffer out to a file.
 * This does multiple writes as necessary.
 * Returns the amount written, or -1 if error was seen
 * on the very first write.
 */
ssize_t FAST_FUNC full_write(int fd, const void *buf, size_t len) {
    ssize_t cc;
    ssize_t total;

    total = 0;

    while (len) {
        cc = safe_write(fd, buf, len);

        if (cc < 0) {
            if (total) {
                /* we already wrote some! */
                /* user can do another write to know the error code */
                return total;
            }
            return cc; /* write() returns -1 on failure. */
        }

        total += cc;
        buf = ((const char *)buf) + cc;
        len -= cc;
    }

    return total;
}

static int wh_helper(int value, int def_val, const char *env_name, int *err) {
    /* Envvars override even if "value" from ioctl is valid (>0).
	 * Rationale: it's impossible to guess what user wants.
	 * For example: "man CMD | ...": should "man" format output
	 * to stdout's width? stdin's width? /dev/tty's width? 80 chars?
	 * We _cant_ know it. If "..." saves text for e.g. email,
	 * then it's probably 80 chars.
	 * If "..." is, say, "grep -v DISCARD | $PAGER", then user
	 * would prefer his tty's width to be used!
	 *
	 * Since we don't know, at least allow user to do this:
	 * "COLUMNS=80 man CMD | ..."
	 */
    char *s = getenv(env_name);
    if (s) {
        value = atoi(s);
        /* If LINES/COLUMNS are set, pretend that there is
		 * no error getting w/h, this prevents some ugly
		 * cursor tricks by our callers */
        *err = 0;
    }

    if (value <= 1 || value >= 30000)
        value = def_val;
    return value;
}

/* It is perfectly ok to pass in a NULL for either width or for
 * height, in which case that value will not be set.  */
int FAST_FUNC get_terminal_width_height(int fd, unsigned *width, unsigned *height) {
    struct winsize win;
    int err;
    int close_me = -1;

    if (fd == -1) {
        if (isatty(STDOUT_FILENO))
            fd = STDOUT_FILENO;
        else if (isatty(STDERR_FILENO))
            fd = STDERR_FILENO;
        else if (isatty(STDIN_FILENO))
            fd = STDIN_FILENO;
        else
            close_me = fd = open("/dev/tty", O_RDONLY);
    }

    win.ws_row = 0;
    win.ws_col = 0;
    /* I've seen ioctl returning 0, but row/col is (still?) 0.
	 * We treat that as an error too.  */
    err = ioctl(fd, TIOCGWINSZ, &win) != 0 || win.ws_row == 0;
    if (height)
        *height = wh_helper(win.ws_row, 24, "LINES", &err);
    if (width)
        *width = wh_helper(win.ws_col, 80, "COLUMNS", &err);

    if (close_me >= 0)
        close(close_me);

    return err;
}
int FAST_FUNC get_terminal_width(int fd) {
    unsigned width;
    get_terminal_width_height(fd, &width, NULL);
    return width;
}

int FAST_FUNC tcsetattr_stdin_TCSANOW(const struct termios *tp) {
    return tcsetattr(STDIN_FILENO, TCSANOW, tp);
}

int FAST_FUNC get_termios_and_make_raw(int fd, struct termios *newterm, struct termios *oldterm, int flags) {
    //TODO: slattach, shell read might be adapted to use this too: grep for "tcsetattr", "[VTIME] = 0"
    int r;

    memset(oldterm, 0, sizeof(*oldterm)); /* paranoia */
    r = tcgetattr(fd, oldterm);
    *newterm = *oldterm;

    /* Turn off buffered input (ICANON)
	 * Turn off echoing (ECHO)
	 * and separate echoing of newline (ECHONL, normally off anyway)
	 */
    newterm->c_lflag &= ~(ICANON | ECHO | ECHONL);
    if (flags & TERMIOS_CLEAR_ISIG) {
        /* dont recognize INT/QUIT/SUSP chars */
        newterm->c_lflag &= ~ISIG;
    }
    /* reads will block only if < 1 char is available */
    newterm->c_cc[VMIN] = 1;
    /* no timeout (reads block forever) */
    newterm->c_cc[VTIME] = 0;
    /* IXON, IXOFF, and IXANY:
 * IXOFF=1: sw flow control is enabled on input queue:
 * tty transmits a STOP char when input queue is close to full
 * and transmits a START char when input queue is nearly empty.
 * IXON=1: sw flow control is enabled on output queue:
 * tty will stop sending if STOP char is received,
 * and resume sending if START is received, or if any char
 * is received and IXANY=1.
 */
    if (flags & TERMIOS_RAW_CRNL_INPUT) {
        /* IXON=0: XON/XOFF chars are treated as normal chars (why we do this?) */
        /* dont convert CR to NL on input */
        newterm->c_iflag &= ~(IXON | ICRNL);
    }
    if (flags & TERMIOS_RAW_CRNL_OUTPUT) {
        /* dont convert NL to CR+NL on output */
        newterm->c_oflag &= ~(ONLCR);
        /* Maybe clear more c_oflag bits? Usually, only OPOST and ONLCR are set.
		 * OPOST  Enable output processing (reqd for OLCUC and *NL* bits to work)
		 * OLCUC  Map lowercase characters to uppercase on output.
		 * OCRNL  Map CR to NL on output.
		 * ONOCR  Don't output CR at column 0.
		 * ONLRET Don't output CR.
		 */
    }
    if (flags & TERMIOS_RAW_INPUT) {
#ifndef IMAXBEL
#define IMAXBEL 0
#endif
#ifndef IUCLC
#define IUCLC 0
#endif
#ifndef IXANY
#define IXANY 0
#endif
        /* IXOFF=0: disable sending XON/XOFF if input buf is full
		 * IXON=0: input XON/XOFF chars are not special
		 * BRKINT=0: dont send SIGINT on break
		 * IMAXBEL=0: dont echo BEL on input line too long
		 * INLCR,ICRNL,IUCLC: dont convert anything on input
		 */
        newterm->c_iflag &= ~(IXOFF | IXON | IXANY | BRKINT | INLCR | ICRNL | IUCLC | IMAXBEL);
    }
    return r;
}

int FAST_FUNC set_termios_to_raw(int fd, struct termios *oldterm, int flags) {
    struct termios newterm;

    get_termios_and_make_raw(fd, &newterm, oldterm, flags);
    return tcsetattr(fd, TCSANOW, &newterm);
}

/* Wrapper which restarts poll on EINTR or ENOMEM.
 * On other errors does perror("poll") and returns.
 * Warning! May take longer than timeout_ms to return! */
int FAST_FUNC safe_poll(struct pollfd *ufds, nfds_t nfds, int timeout) {
    while (1) {
        int n = poll(ufds, nfds, timeout);
        if (n >= 0)
            return n;
        /* Make sure we inch towards completion */
        if (timeout > 0)
            timeout--;
        /* E.g. strace causes poll to return this */
        if (errno == EINTR)
            continue;
        /* Kernel is very low on memory. Retry. */
        /* I doubt many callers would handle this correctly! */
        if (errno == ENOMEM)
            continue;
        bb_simple_error_msg("poll");
        return n;
    }
}
