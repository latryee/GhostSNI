#ifndef PACKET_PARSER_H
#define PACKET_PARSER_H

#include <stdint.h>

#define MAX_HOSTNAME_LEN    256

typedef enum {
    PKT_TYPE_UNKNOWN     = 0,
    PKT_TYPE_TLS_HELLO   = 1,
    PKT_TYPE_HTTP        = 2
} PacketType;

typedef struct {
    PacketType  type;
    char        hostname[MAX_HOSTNAME_LEN];
    uint32_t    sni_offset;
    uint32_t    sni_length;
} ParsedPacket;

void parse_packet(const uint8_t *payload, uint32_t payload_len,
                  ParsedPacket *result);

const char* get_packet_type_string(PacketType type);

#endif
