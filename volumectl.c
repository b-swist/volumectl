#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "pulseaudio.h"

typedef struct command {
	const char *name;
	action_t action;
	bool needs_arg;
} command_t;

static const command_t commands[] = {
    {"get",         ACTION_GET,         false},
    {"set",         ACTION_SET,         true },
    {"up",          ACTION_UP,          true },
    {"down",        ACTION_DOWN,        true },
    {"mute",        ACTION_MUTE,        false},
    {"unmute",      ACTION_UNMUTE,      false},
    {"mute-toggle", ACTION_MUTE_TOGGLE, false},
};

int main(int argc, char **argv)
{
	if (argc < 2) {
		eprintf("%s: no command provided\n", argv[0]);
		return EXIT_FAILURE;
	}

	const command_t *cmd = NULL;

	for (size_t i = 0; i < LENGTH(commands); i++) {
		if (strcmp(argv[1], commands[i].name) == 0) {
			cmd = &commands[i];
			break;
		}
	}

	if (cmd == NULL) {
		eprintf("%s: unknown command provided: %s\n", argv[0], argv[1]);
		return EXIT_FAILURE;
	}

	void *data = NULL;
	double percent = 0.0;
	if (cmd->needs_arg) {
		if (argc < 3) {
			eprintf("%s: missing argument\n", argv[0]);
			return EXIT_FAILURE;
		}

		if (!parse_percent(argv[2], &percent)) {
			eprintf("%s: invalid argument: %s\n", argv[0], argv[2]);
			return EXIT_FAILURE;
		}

		data = &percent;
	}

	if (argc > 2 + cmd->needs_arg) {
		eprintf("%s: too many arguments provided\n", argv[0]);
		return EXIT_FAILURE;
	}

	return pulseaudio_handle_action(cmd->action, data);
}
