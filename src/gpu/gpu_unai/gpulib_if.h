#ifndef GPULIB_H
#define GPULIB_H

#include <stdint.h>

extern const unsigned char cmd_lengths[256];

int renderer_init(void);

void renderer_finish(void);

void renderer_notify_res_change(void);

int do_cmd_list(unsigned int *list, int list_len, int *last_cmd);

void renderer_sync_ecmds(uint32_t *ecmds);

void renderer_update_caches(int x, int y, int w, int h);

void renderer_flush_queues(void);

void renderer_set_interlace(int enable, int is_odd);

void renderer_set_config(const struct gpulib_config_t *config);

#endif
