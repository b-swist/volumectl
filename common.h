#ifndef VOLUMECTL_COMMON_H
#define VOLUMECTL_COMMON_H

#include <ctype.h>
#include <stdio.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define eputs(s) fputs(s, stderr)

#define LENGTH(A) (sizeof(A) / sizeof(A[0]))

typedef enum action {
	ACTION_NONE,
	ACTION_GET,
	ACTION_SET,
	ACTION_UP,
	ACTION_DOWN,
	ACTION_MUTE,
	ACTION_UNMUTE,
	ACTION_MUTE_TOGGLE,
} action_t;

int parse_percent(const char *s, double *out);

#endif // VOLUMECTL_COMMON_H
