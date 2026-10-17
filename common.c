#include <ctype.h>
#include <stdlib.h>

#include "common.h"

int parse_percent(const char *s, double *out)
{
	char *end;

	double conv = strtod(s, &end);

	if (end == s) {
		return 0;
	}

	if (*end == '%') {
		end++;
		while (isspace((unsigned char)*end)) {
			end++;
		}
	}

	if (*end != '\0') {
		return 0;
	}

	*out = conv / 100.0;

	return 1;
}
