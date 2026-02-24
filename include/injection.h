#ifndef INJECTION_H
#define INJECTION_H

#include <windows.h>
#include "windivert.h"
#include "cli.h"
#include "packet_parser.h"

#define FRAGMENT_BUF_SIZE 65535

typedef enum {
    INJECT_OK           = 0,
    INJECT_PASSTHROUGH  = 1,
    INJECT_ERROR        = -1
} InjectResult;

InjectResult inject_packet(
    HANDLE              handle,
    unsigned char      *packet,
    UINT                packet_len,
    WINDIVERT_ADDRESS  *addr,
    PWINDIVERT_IPHDR    ip_hdr,
    PWINDIVERT_TCPHDR   tcp_hdr,
    PVOID               payload,
    UINT                payload_len_val,
    const GhostConfig  *config,
    const ParsedPacket *parsed
);

int apply_http_tricks(
    unsigned char      *packet,
    UINT               *packet_len,
    PWINDIVERT_IPHDR    ip_hdr,
    PWINDIVERT_TCPHDR   tcp_hdr,
    PVOID               payload,
    UINT                payload_len,
    const GhostConfig  *config
);

#endif
