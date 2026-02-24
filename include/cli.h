#ifndef CLI_H
#define CLI_H

#include <stdint.h>

#define MAX_DNS_ADDR_LEN    64
#define GHOSTSNI_VERSION    "0.3.0"

typedef struct {
    int     do_fragment;
    int     frag_offset;
    int     do_fake;
    int     fake_ttl;
    int     ttl_burst;
    int     wrong_chksum;
    int     reverse_frag;
    int     block_quic;
    int     do_passive_dpi;
    int     do_host_replace;
    int     do_host_remove_space;
    int     do_host_mix_case;
    char    dns_addr[MAX_DNS_ADDR_LEN];
    int     dns_port;
    int     extra_port;
    int     verbose;
    int     silent;
    int     modeset;
} GhostConfig;

void ghost_config_init(GhostConfig *cfg);
int  ghost_parse_args(int argc, char *argv[], GhostConfig *cfg);
void ghost_print_help(const char *program_name);
void ghost_print_banner(const GhostConfig *cfg);

#endif
