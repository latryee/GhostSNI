#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "packet_parser.h"

static inline uint16_t read_uint16_be(const uint8_t *buf)
{
    return (uint16_t)((buf[0] << 8) | buf[1]);
}

static inline uint32_t read_uint24_be(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[2];
}

static int extract_sni_from_client_hello(const uint8_t *payload,
                                         uint32_t payload_len,
                                         ParsedPacket *result)
{
    uint32_t pos = 0;

    if (payload_len < 5)
        return 0;

    pos = 5;

    if (pos + 4 > payload_len)
        return 0;

    pos = 9;

    if (pos + 2 > payload_len)
        return 0;
    pos += 2;

    if (pos + 32 > payload_len)
        return 0;
    pos += 32;

    if (pos + 1 > payload_len)
        return 0;

    uint8_t session_id_len = payload[pos];
    pos += 1;

    if (pos + session_id_len > payload_len)
        return 0;
    pos += session_id_len;

    if (pos + 2 > payload_len)
        return 0;

    uint16_t cipher_suites_len = read_uint16_be(payload + pos);
    pos += 2;

    if (pos + cipher_suites_len > payload_len)
        return 0;
    pos += cipher_suites_len;

    if (pos + 1 > payload_len)
        return 0;

    uint8_t comp_methods_len = payload[pos];
    pos += 1;

    if (pos + comp_methods_len > payload_len)
        return 0;
    pos += comp_methods_len;

    if (pos + 2 > payload_len)
        return 0;

    uint16_t extensions_total_len = read_uint16_be(payload + pos);
    pos += 2;

    uint32_t extensions_end = pos + extensions_total_len;
    if (extensions_end > payload_len)
        extensions_end = payload_len;

    while (pos + 4 <= extensions_end)
    {
        uint16_t ext_type   = read_uint16_be(payload + pos);
        uint16_t ext_length = read_uint16_be(payload + pos + 2);
        pos += 4;

        if (pos + ext_length > extensions_end)
            break;

        if (ext_type == 0x0000)
        {
            if (ext_length < 5)
            {
                pos += ext_length;
                continue;
            }

            uint32_t sni_data_start = pos;

            pos += 2;

            uint8_t name_type = payload[pos];
            pos += 1;

            if (name_type != 0x00)
            {
                pos = sni_data_start + ext_length;
                continue;
            }

            if (pos + 2 > extensions_end)
                break;

            uint16_t name_len = read_uint16_be(payload + pos);
            pos += 2;

            if (pos + name_len > extensions_end)
                break;

            uint16_t copy_len = name_len;
            if (copy_len >= MAX_HOSTNAME_LEN)
                copy_len = MAX_HOSTNAME_LEN - 1;

            memcpy(result->hostname, payload + pos, copy_len);
            result->hostname[copy_len] = '\0';

            result->sni_offset = pos;
            result->sni_length = name_len;

            return 1;
        }

        pos += ext_length;
    }

    return 0;
}

static int extract_http_host(const uint8_t *payload, uint32_t payload_len,
                             ParsedPacket *result)
{
    const char *patterns[] = {
        "\r\nHost: ",
        "\r\nhost: ",
        "\r\nHost:",
        "\r\nhost:",
        NULL
    };

    const size_t pattern_extra[] = { 8, 8, 7, 7 };

    const uint8_t *found = NULL;
    uint32_t host_start = 0;
    int p;

    for (p = 0; patterns[p] != NULL; p++)
    {
        size_t pat_len = pattern_extra[p];
        uint32_t i;

        for (i = 0; i + pat_len <= payload_len; i++)
        {
            if (memcmp(payload + i, patterns[p], pat_len) == 0)
            {
                found = payload + i + pat_len;
                host_start = i + (uint32_t)pat_len;
                break;
            }
        }

        if (found != NULL)
            break;
    }

    if (found == NULL)
        return 0;

    while (host_start < payload_len &&
           (payload[host_start] == ' ' || payload[host_start] == '\t'))
    {
        host_start++;
    }

    uint32_t host_end = host_start;
    while (host_end < payload_len &&
           payload[host_end] != '\r' &&
           payload[host_end] != '\n' &&
           payload[host_end] != ':' &&
           payload[host_end] != ' ')
    {
        host_end++;
    }

    uint32_t host_len = host_end - host_start;
    if (host_len == 0 || host_len >= MAX_HOSTNAME_LEN)
        return 0;

    memcpy(result->hostname, payload + host_start, host_len);
    result->hostname[host_len] = '\0';

    return 1;
}

static int is_http_request(const uint8_t *payload, uint32_t payload_len)
{
    if (payload == NULL || payload_len < 4)
        return 0;

    if (memcmp(payload, "GET ",  4) == 0) return 1;
    if (memcmp(payload, "POST",  4) == 0) return 1;
    if (memcmp(payload, "HEAD",  4) == 0) return 1;
    if (payload_len >= 3 && memcmp(payload, "PUT",     3) == 0) return 1;
    if (payload_len >= 6 && memcmp(payload, "DELETE",  6) == 0) return 1;
    if (payload_len >= 7 && memcmp(payload, "OPTIONS", 7) == 0) return 1;
    if (payload_len >= 7 && memcmp(payload, "CONNECT", 7) == 0) return 1;
    if (payload_len >= 5 && memcmp(payload, "PATCH",   5) == 0) return 1;

    return 0;
}

static int is_tls_client_hello(const uint8_t *payload, uint32_t payload_len)
{
    if (payload == NULL || payload_len < 6)
        return 0;

    return (payload[0] == 0x16 &&
            payload[1] == 0x03 &&
            payload[5] == 0x01);
}

void parse_packet(const uint8_t *payload, uint32_t payload_len,
                  ParsedPacket *result)
{
    memset(result, 0, sizeof(ParsedPacket));

    if (payload == NULL || payload_len == 0)
    {
        result->type = PKT_TYPE_UNKNOWN;
        return;
    }

    if (is_tls_client_hello(payload, payload_len))
    {
        result->type = PKT_TYPE_TLS_HELLO;

        if (!extract_sni_from_client_hello(payload, payload_len, result))
        {
            result->hostname[0] = '\0';
        }
        return;
    }

    if (is_http_request(payload, payload_len))
    {
        result->type = PKT_TYPE_HTTP;

        if (!extract_http_host(payload, payload_len, result))
        {
            result->hostname[0] = '\0';
        }
        return;
    }

    result->type = PKT_TYPE_UNKNOWN;
}

const char* get_packet_type_string(PacketType type)
{
    switch (type)
    {
    case PKT_TYPE_TLS_HELLO: return "[TLS CLIENT HELLO]";
    case PKT_TYPE_HTTP:      return "[HTTP REQUEST]";
    case PKT_TYPE_UNKNOWN:   return "[TCP DATA]";
    default:                 return "[???]";
    }
}
