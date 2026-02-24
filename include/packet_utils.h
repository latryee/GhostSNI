#ifndef PACKET_UTILS_H
#define PACKET_UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <winsock2.h>

static inline void print_ip_address(UINT32 addr, char *buf, size_t buf_len)
{
    UINT32 ip = ntohl(addr);
    snprintf(buf, buf_len, "%u.%u.%u.%u",
             (ip >> 24) & 0xFF,
             (ip >> 16) & 0xFF,
             (ip >> 8)  & 0xFF,
             ip & 0xFF);
}

static inline void print_tcp_flags(const WINDIVERT_TCPHDR *tcp_hdr,
                                   char *buf, size_t buf_len)
{
    snprintf(buf, buf_len, "%s%s%s%s%s%s",
             tcp_hdr->Syn ? "SYN " : "",
             tcp_hdr->Ack ? "ACK " : "",
             tcp_hdr->Fin ? "FIN " : "",
             tcp_hdr->Rst ? "RST " : "",
             tcp_hdr->Psh ? "PSH " : "",
             tcp_hdr->Urg ? "URG " : "");
}

static inline int is_http_request(const uint8_t *payload, uint32_t payload_len)
{
    if (payload == NULL || payload_len < 4)
        return 0;

    if (memcmp(payload, "GET ",     4) == 0) return 1;
    if (memcmp(payload, "POST",     4) == 0) return 1;
    if (memcmp(payload, "HEAD",     4) == 0) return 1;
    if (payload_len >= 3 && memcmp(payload, "PUT",  3) == 0) return 1;
    if (payload_len >= 6 && memcmp(payload, "DELETE", 6) == 0) return 1;
    if (payload_len >= 7 && memcmp(payload, "OPTIONS", 7) == 0) return 1;
    if (payload_len >= 7 && memcmp(payload, "CONNECT", 7) == 0) return 1;
    if (payload_len >= 5 && memcmp(payload, "PATCH", 5) == 0) return 1;

    return 0;
}

static inline int is_tls_client_hello(const uint8_t *payload,
                                      uint32_t payload_len)
{
    if (payload == NULL || payload_len < 6)
        return 0;

    if (payload[0] == 0x16 &&
        payload[1] == 0x03 &&
        payload[5] == 0x01)
    {
        return 1;
    }

    return 0;
}

static inline const char* get_packet_type_label(const uint8_t *payload,
                                                uint32_t payload_len)
{
    if (is_tls_client_hello(payload, payload_len))
        return "[TLS CLIENT HELLO]";
    if (is_http_request(payload, payload_len))
        return "[HTTP REQUEST]";
    return "[TCP DATA]";
}

#endif
