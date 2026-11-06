#include <pulse/pulseaudio.h>
#include <stddef.h>
#include <stdlib.h>

#include "common.h"

static pa_mainloop *ml = NULL;
static action_t action = ACTION_NONE;

static void get_volume_cb(pa_context *ctx, const pa_sink_info *info, int is_last, void *data)
{
	if (is_last < 0) {
		eputs("failed to get sink info\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	if (is_last > 0) {
		pa_mainloop_quit(ml, 0);
		return;
	}

	double vol = (double)pa_cvolume_max(&info->volume) / PA_VOLUME_NORM;
	double percent = vol * 100.0;

	if (info->mute) {
		printf("volume: %.1f%% (muted)\n", percent);
	} else {
		printf("volume: %.1f%%\n", percent);
	}
}

static void context_success_cb(pa_context *ctx, int success, void *data)
{
	if (!success) {
		eputs("failed to change sink state\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	pa_mainloop_quit(ml, 0);
}

static void set_volume_cb(pa_context *ctx, const pa_sink_info *info, int is_last, void *data)
{
	double *passed = data;

	if (is_last < 0) {
		eputs("failed to get sink info\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	if (is_last > 0) {
		return;
	}

	pa_volume_t vol = (pa_volume_t)(*passed * PA_VOLUME_NORM);
	pa_cvolume cv = info->volume;
	pa_cvolume_scale(&cv, vol);

	pa_operation *op = pa_context_set_sink_volume_by_name(ctx, info->name, &cv, context_success_cb, NULL);

	if (op == NULL) {
		eputs("failed to change sink state\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	pa_operation_unref(op);
}

static void change_volume_cb(pa_context *ctx, const pa_sink_info *info, int is_last, void *data)
{
	double *step = data;

	if (is_last < 0) {
		eputs("failed to get sink info\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	if (is_last > 0) {
		return;
	}

	pa_volume_t delta = (pa_volume_t)(*step * PA_VOLUME_NORM + 0.5);
	pa_cvolume cv = info->volume;
	if (action == ACTION_UP) {
		pa_cvolume_inc_clamp(&cv, delta, PA_VOLUME_NORM);
	} else {
		pa_cvolume_dec(&cv, delta);
	}

	pa_operation *op = pa_context_set_sink_volume_by_name(ctx, info->name, &cv, context_success_cb, NULL);

	if (op == NULL) {
		eputs("failed to change sink state\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	pa_operation_unref(op);
}

static void mute_cb(pa_context *ctx, const pa_sink_info *info, int is_last, void *data)
{
	if (is_last < 0) {
		eputs("failed to get sink info\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	if (is_last > 0) {
		return;
	}

	int should_mute = 1;
	if (action == ACTION_UNMUTE) {
		should_mute = 0;
	} else if (action == ACTION_MUTE_TOGGLE) {
		should_mute = !info->mute;
	}

	pa_operation *op = pa_context_set_sink_mute_by_name(ctx, info->name, should_mute, context_success_cb, NULL);

	if (op == NULL) {
		eputs("failed to change sink state\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	pa_operation_unref(op);
}

static void server_info_cb(pa_context *ctx, const pa_server_info *info, void *data)
{
	pa_sink_info_cb_t callback = NULL;

	if (info == NULL) {
		eputs("failed to get server info\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	if (info->default_sink_name == NULL) {
		eputs("failed to get default sink name\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	switch (action) {
	case ACTION_GET:
		callback = get_volume_cb;
		break;
	case ACTION_SET:
		callback = set_volume_cb;
		break;
	case ACTION_UP:
	case ACTION_DOWN:
		callback = change_volume_cb;
		break;
	case ACTION_MUTE:
	case ACTION_UNMUTE:
	case ACTION_MUTE_TOGGLE:
		callback = mute_cb;
		break;
	default:
		eputs("unsupported action");
		pa_mainloop_quit(ml, 1);
		break;
	}

	pa_operation *op = pa_context_get_sink_info_by_name(ctx, info->default_sink_name, callback, data);

	if (op == NULL) {
		eputs("failed to request sink info\n");
		pa_mainloop_quit(ml, 1);
		return;
	}

	pa_operation_unref(op);
}

static void context_state_cb(pa_context *ctx, void *data)
{
	switch (pa_context_get_state(ctx)) {
	case PA_CONTEXT_READY: {
		pa_operation *op = pa_context_get_server_info(ctx, server_info_cb, data);

		if (op == NULL) {
			eputs("failed to request server info\n");
			pa_mainloop_quit(ml, 1);
			break;
		}
		pa_operation_unref(op);
		break;
	}

	case PA_CONTEXT_FAILED:
		eputs("failed to connect to the server\n");
		pa_mainloop_quit(ml, 1);
		break;

	case PA_CONTEXT_TERMINATED:
		pa_mainloop_quit(ml, 0);
		break;

	default:
		break;
	}
}

int pulseaudio_handle_action(action_t a, void *data)
{
	ml = pa_mainloop_new();
	action = a;

	int ret = EXIT_FAILURE;
	pa_context *ctx = NULL;

	if (ml == NULL) {
		eputs("failed to create main loop\n");
		goto quit;
	}

	ctx = pa_context_new(pa_mainloop_get_api(ml), "volumectl");
	if (ctx == NULL) {
		eputs("failed to get context\n");
		goto quit;
	}

	pa_context_set_state_callback(ctx, context_state_cb, data);

	if (pa_context_connect(ctx, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
		eputs("failed to connect to the server\n");
		goto quit;
	}

	if (pa_mainloop_run(ml, &ret) < 0) {
		eputs("failed to run main loop\n");
		ret = EXIT_FAILURE;
	}

	pa_context_disconnect(ctx);

quit:
	if (ctx != NULL) {
		pa_context_unref(ctx);
		ctx = NULL;
	}

	if (ml != NULL) {
		pa_mainloop_free(ml);
		ml = NULL;
	}

	return ret;
}
