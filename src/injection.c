#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "windivert.h"
#include "injection.h"

static const char FAKE_HTTP_PAYLOAD[] =
    "GET / HTTP/1.1\r\n"
    "Host: www.google.com\r\n"
    "\r\n";

#define FAKE_HTTP_PAYLOAD_LEN  (sizeof(FAKE_HTTP_PAYLOAD) - 1)

static const uint8_t FAKE_TLS_CLIENT_HELLO[] = {
    0x16,
    0x03, 0x01,
    0x00, 0x65,

    0x01,
    0x00, 0x00, 0x61,

    0x03, 0x03,

    0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB,
    0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB,
    0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB,
    0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB,

    0x00,

    0x00, 0x02,
    0x00, 0x2F,

    0x01,
    0x00,

    0x00, 0x36,

    0x00, 0x00,
    0x00, 0x13,
    0x00, 0x11,
    0x00,
    0x00, 0x0E,
    0x77, 0x77, 0x77, 0x2E,
    0x67, 0x6F, 0x6F, 0x67,
    0x6C, 0x65, 0x2E, 0x63,
    0x6F, 0x6D,

    0x00, 0x2B,
    0x00, 0x05,
    0x04,
    0x03, 0x03,
    0x03, 0x01,

    0xFF, 0x01,
    0x00, 0x01,
    0x00,

    0x00, 0x0B,
    0x00, 0x02,
    0x01,
    0x00,

    0x00, 0x17,
    0x00, 0x00,

    0x00, 0x15,
    0x00, 0x00
};

#define FAKE_TLS_PAYLOAD_LEN  sizeof(FAKE_TLS_CLIENT_HELLO)

static InjectResult send_original(HANDLE handle, unsigned char *packet,
                                  UINT packet_len, WINDIVERT_ADDRESS *addr)
{
    WinDivertHelperCalcChecksums(packet, packet_len, addr, 0);

    if (!WinDivertSend(handle, packet, packet_len, NULL, addr))
    {
        fprintf(stderr, "[UYARI] WinDivertSend hatasi (passthrough): %lu\n",
                GetLastError());
        return INJECT_ERROR;
    }

    return INJECT_PASSTHROUGH;
}

static int find_host_header(const uint8_t *payload, UINT payload_len)
{
    if (payload_len < 5)
        return -1;

    for (UINT i = 0; i <= payload_len - 5; i++)
    {
        uint8_t c0 = payload[i]     | 0x20;
        uint8_t c1 = payload[i + 1] | 0x20;
        uint8_t c2 = payload[i + 2] | 0x20;
        uint8_t c3 = payload[i + 3] | 0x20;

        if (c0 == 'h' && c1 == 'o' && c2 == 's' && c3 == 't' &&
            payload[i + 4] == ':')
        {
            return (int)i;
        }
    }

    return -1;
}

static int find_host_value_start(const uint8_t *payload, UINT payload_len,
                                  int host_colon_pos)
{
    int pos = host_colon_pos + 1;

    while (pos < (int)payload_len && payload[pos] == ' ')
        pos++;

    if (pos >= (int)payload_len)
        return -1;

    return pos;
}

static int find_host_value_end(const uint8_t *payload, UINT payload_len,
                                int value_start)
{
    for (int i = value_start; i < (int)payload_len; i++)
    {
        if (payload[i] == '\r' || payload[i] == '\n')
            return i;
    }
    return (int)payload_len;
}

int apply_http_tricks(
    unsigned char      *packet,
    UINT               *packet_len,
    PWINDIVERT_IPHDR    ip_hdr,
    PWINDIVERT_TCPHDR   tcp_hdr,
    PVOID               payload,
    UINT                payload_len,
    const GhostConfig  *config)
{
    if (!config->do_host_replace && !config->do_host_mix_case &&
        !config->do_host_remove_space)
    {
        return 0;
    }

    (void)tcp_hdr;
    (void)packet;

    uint8_t *p = (uint8_t *)payload;
    int delta = 0;

    int host_pos = find_host_header(p, payload_len);

    if (host_pos < 0)
    {
        return 0;
    }

    if (config->do_host_replace)
    {
        p[host_pos]     = 'h';
        p[host_pos + 1] = 'o';
        p[host_pos + 2] = 'S';
        p[host_pos + 3] = 't';
    }

    if (config->do_host_mix_case)
    {
        int val_start = find_host_value_start(p, payload_len, host_pos + 4);
        int val_end   = find_host_value_end(p, payload_len,
                                            val_start >= 0 ? val_start : 0);

        if (val_start >= 0)
        {
            int toggle = 0;
            for (int i = val_start; i < val_end; i++)
            {
                uint8_t ch = p[i];

                if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
                {
                    if (toggle)
                        p[i] = ch & ~0x20;
                    else
                        p[i] = ch | 0x20;

                    toggle = !toggle;
                }
            }
        }
    }

    if (config->do_host_remove_space)
    {
        int colon_pos = host_pos + 4;

        if (colon_pos + 1 < (int)payload_len && p[colon_pos + 1] == ' ')
        {
            int space_pos = colon_pos + 1;

            UINT bytes_to_move = payload_len - (UINT)space_pos - 1;

            if (bytes_to_move > 0)
            {
                memmove(p + space_pos, p + space_pos + 1, bytes_to_move);
            }

            uint16_t old_len = ntohs(ip_hdr->Length);
            ip_hdr->Length = htons(old_len - 1);

            *packet_len = *packet_len - 1;

            delta = -1;
        }
    }

    return delta;
}

static int send_fake_packet(
    HANDLE              handle,
    unsigned char      *orig_packet,
    WINDIVERT_ADDRESS  *addr,
    PWINDIVERT_IPHDR    ip_hdr,
    PWINDIVERT_TCPHDR   tcp_hdr,
    const GhostConfig  *config,
    PacketType          pkt_type)
{
    unsigned char fake_buf[FRAGMENT_BUF_SIZE];

    UINT ip_hdr_len  = (UINT)(ip_hdr->HdrLength) * 4;
    UINT tcp_hdr_len = (UINT)(tcp_hdr->HdrLength) * 4;
    UINT headers_total = ip_hdr_len + tcp_hdr_len;

    const void *fake_payload;
    UINT fake_payload_len;

    if (pkt_type == PKT_TYPE_HTTP)
    {
        fake_payload     = FAKE_HTTP_PAYLOAD;
        fake_payload_len = (UINT)FAKE_HTTP_PAYLOAD_LEN;
    }
    else
    {
        fake_payload     = FAKE_TLS_CLIENT_HELLO;
        fake_payload_len = (UINT)FAKE_TLS_PAYLOAD_LEN;
    }

    UINT fake_total = headers_total + fake_payload_len;

    if (fake_total > FRAGMENT_BUF_SIZE)
    {
        fprintf(stderr, "[UYARI] Sahte paket tamponu yetersiz (%u byte)\n",
                fake_total);
        return -1;
    }

    memcpy(fake_buf, orig_packet, headers_total);
    memcpy(fake_buf + headers_total, fake_payload, fake_payload_len);

    PWINDIVERT_IPHDR fake_ip = (PWINDIVERT_IPHDR)fake_buf;
    fake_ip->Length = htons((uint16_t)fake_total);

    fake_ip->TTL = (uint8_t)config->fake_ttl;

    if (config->ttl_burst)
    {
        static const uint8_t burst_ttls[] = { 3, 4, 5, 6, 7 };
        int burst_count = sizeof(burst_ttls) / sizeof(burst_ttls[0]);

        for (int b = 0; b < burst_count; b++)
        {
            fake_ip->TTL = burst_ttls[b];

            PWINDIVERT_TCPHDR ftcp = (PWINDIVERT_TCPHDR)(fake_buf + ip_hdr_len);
            fake_ip->Checksum  = 0;
            ftcp->Checksum     = 0;
            WinDivertHelperCalcChecksums(fake_buf, fake_total, addr, 0);

            if (config->wrong_chksum)
                ftcp->Checksum ^= 0xFFFF;

            if (!WinDivertSend(handle, fake_buf, fake_total, NULL, addr))
            {
                fprintf(stderr, "[UYARI] Burst fake TTL=%d basarisiz: %lu\n",
                        burst_ttls[b], GetLastError());
            }
        }
        return 0;
    }

    fake_ip->TTL = (uint8_t)config->fake_ttl;

    PWINDIVERT_TCPHDR fake_tcp = (PWINDIVERT_TCPHDR)(fake_buf + ip_hdr_len);
    fake_ip->Checksum  = 0;
    fake_tcp->Checksum = 0;

    WinDivertHelperCalcChecksums(fake_buf, fake_total, addr, 0);

    if (config->wrong_chksum)
        fake_tcp->Checksum ^= 0xFFFF;

    if (!WinDivertSend(handle, fake_buf, fake_total, NULL, addr))
    {
        fprintf(stderr, "[HATA] Sahte paket gonderimi basarisiz: %lu\n",
                GetLastError());
        return -1;
    }

    return 0;
}

static int send_real_fragmented(
    HANDLE              handle,
    unsigned char      *packet,
    WINDIVERT_ADDRESS  *addr,
    PWINDIVERT_IPHDR    ip_hdr,
    PWINDIVERT_TCPHDR   tcp_hdr,
    PVOID               payload,
    UINT                payload_len_val,
    UINT                split_pos,
    int                 reverse_frag)
{
    UINT ip_hdr_len    = (UINT)(ip_hdr->HdrLength) * 4;
    UINT tcp_hdr_len   = (UINT)(tcp_hdr->HdrLength) * 4;
    UINT headers_total = ip_hdr_len + tcp_hdr_len;
    UINT part1_len     = split_pos;
    UINT part2_len     = payload_len_val - split_pos;
    uint32_t orig_seq  = ntohl(tcp_hdr->SeqNum);

    unsigned char frag1[FRAGMENT_BUF_SIZE];
    UINT frag1_total = headers_total + part1_len;
    memcpy(frag1, packet, headers_total);
    memcpy(frag1 + headers_total, payload, part1_len);
    PWINDIVERT_IPHDR  frag1_ip  = (PWINDIVERT_IPHDR)frag1;
    PWINDIVERT_TCPHDR frag1_tcp = (PWINDIVERT_TCPHDR)(frag1 + ip_hdr_len);
    frag1_ip->Length    = htons((uint16_t)frag1_total);
    frag1_ip->Checksum  = 0;
    frag1_tcp->Checksum = 0;
    WinDivertHelperCalcChecksums(frag1, frag1_total, addr, 0);

    unsigned char frag2[FRAGMENT_BUF_SIZE];
    UINT frag2_total = headers_total + part2_len;
    memcpy(frag2, packet, headers_total);
    memcpy(frag2 + headers_total, (uint8_t *)payload + split_pos, part2_len);
    PWINDIVERT_IPHDR  frag2_ip  = (PWINDIVERT_IPHDR)frag2;
    PWINDIVERT_TCPHDR frag2_tcp = (PWINDIVERT_TCPHDR)(frag2 + ip_hdr_len);
    frag2_ip->Length    = htons((uint16_t)frag2_total);
    frag2_tcp->SeqNum   = htonl(orig_seq + split_pos);
    frag2_ip->Checksum  = 0;
    frag2_tcp->Checksum = 0;
    WinDivertHelperCalcChecksums(frag2, frag2_total, addr, 0);

    unsigned char *first_frag  = reverse_frag ? frag2 : frag1;
    UINT           first_len   = reverse_frag ? frag2_total : frag1_total;
    unsigned char *second_frag = reverse_frag ? frag1 : frag2;
    UINT           second_len  = reverse_frag ? frag1_total : frag2_total;

    if (!WinDivertSend(handle, first_frag, first_len, NULL, addr))
    {
        fprintf(stderr, "[HATA] Fragment %s gonderimi basarisiz: %lu\n",
                reverse_frag ? "2 (ilk)" : "1", GetLastError());
        return -1;
    }
    if (!WinDivertSend(handle, second_frag, second_len, NULL, addr))
    {
        fprintf(stderr, "[HATA] Fragment %s gonderimi basarisiz: %lu\n",
                reverse_frag ? "1 (son)" : "2", GetLastError());
        return -1;
    }

    return 0;
}

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
    const ParsedPacket *parsed)
{
    if (!config->do_fake && !config->do_fragment)
    {
        return send_original(handle, packet, packet_len, addr);
    }

    if (parsed->type == PKT_TYPE_UNKNOWN)
    {
        return send_original(handle, packet, packet_len, addr);
    }

    if (tcp_hdr->Fin || tcp_hdr->Rst || tcp_hdr->Syn)
    {
        return send_original(handle, packet, packet_len, addr);
    }

    if (payload == NULL || payload_len_val == 0)
    {
        return send_original(handle, packet, packet_len, addr);
    }

    int did_fake = 0;

    if (config->do_fake)
    {
        if (send_fake_packet(handle, packet, addr, ip_hdr, tcp_hdr,
                             config, parsed->type) == 0)
        {
            did_fake = 1;
        }
        else
        {
            fprintf(stderr,
                "[UYARI] Sahte paket gonderilemedi, gercek paket devam ediyor.\n");
        }
    }

    UINT split_pos = (UINT)config->frag_offset;

    if (parsed->type == PKT_TYPE_TLS_HELLO && parsed->sni_offset > 0)
    {
        split_pos = parsed->sni_offset;
    }

    int can_fragment = config->do_fragment
                    && payload_len_val > split_pos
                    && split_pos > 0;

    if (can_fragment)
    {
        if (send_real_fragmented(handle, packet, addr, ip_hdr, tcp_hdr,
                                 payload, payload_len_val, split_pos,
                                 config->reverse_frag) != 0)
        {
            return INJECT_ERROR;
        }
    }
    else
    {
        WinDivertHelperCalcChecksums(packet, packet_len, addr, 0);

        if (!WinDivertSend(handle, packet, packet_len, NULL, addr))
        {
            fprintf(stderr, "[HATA] Gercek paket gonderimi basarisiz: %lu\n",
                    GetLastError());
            return INJECT_ERROR;
        }
    }

    return (did_fake || can_fragment) ? INJECT_OK : INJECT_PASSTHROUGH;
}
