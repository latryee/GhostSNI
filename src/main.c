#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#include "windivert.h"

#include "cli.h"
#include "injection.h"
#include "packet_parser.h"
#include "packet_utils.h"
#include "tray.h"

#define MAX_PACKET_SIZE 65535
#define MAX_FILTER_LEN  512

volatile int g_running = 1;
HANDLE g_divert_handle = INVALID_HANDLE_VALUE;

static UINT32 g_original_dns_ip = 0;

static BOOL WINAPI ctrl_c_handler(DWORD ctrl_type)
{
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT)
    {
        printf("\n[!] Kapatma sinyali alindi, temiz cikis yapiliyor...\n");
        g_running = 0;

        if (g_divert_handle != INVALID_HANDLE_VALUE)
        {
            WinDivertShutdown(g_divert_handle, WINDIVERT_SHUTDOWN_BOTH);
        }
        return TRUE;
    }
    return FALSE;
}

static int check_admin_privileges(void)
{
    BOOL is_admin = FALSE;
    PSID admin_group = NULL;

    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&nt_authority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0, &admin_group))
    {
        CheckTokenMembership(NULL, admin_group, &is_admin);
        FreeSid(admin_group);
    }
    return is_admin;
}

static void build_filter(const GhostConfig *cfg, char *filter_buf, size_t buf_len)
{
    int dns_active = (cfg->dns_addr[0] != '\0');

    if (cfg->extra_port > 0)
    {
        snprintf(filter_buf, buf_len,
            "(outbound and ("
            "(tcp and (tcp.DstPort == 80 or tcp.DstPort == 443 or "
            "tcp.DstPort == %d) and tcp.PayloadLength > 0) or "
            "(udp and (udp.DstPort == 443 or udp.DstPort == 53))"
            "))%s",
            cfg->extra_port,
            dns_active ? " or (inbound and udp.SrcPort == 53)" : "");
    }
    else
    {
        snprintf(filter_buf, buf_len,
            "(outbound and ("
            "(tcp and (tcp.DstPort == 80 or tcp.DstPort == 443) "
            "and tcp.PayloadLength > 0) or "
            "(udp and (udp.DstPort == 443 or udp.DstPort == 53))"
            "))%s",
            dns_active ? " or (inbound and udp.SrcPort == 53)" : "");
    }
}

int main(int argc, char *argv[])
{
    unsigned char packet[MAX_PACKET_SIZE];
    UINT packet_len = 0;

    WINDIVERT_ADDRESS addr;

    PWINDIVERT_IPHDR   ip_hdr  = NULL;
    PWINDIVERT_TCPHDR  tcp_hdr = NULL;
    PWINDIVERT_UDPHDR  udp_hdr = NULL;
    PVOID              payload = NULL;
    UINT               payload_len = 0;

    char src_ip_str[32];
    char dst_ip_str[32];
    char flags_str[64];

    char filter_buf[MAX_FILTER_LEN];

    unsigned long long pkt_count = 0;

    GhostConfig config;
    ParsedPacket parsed;

    ghost_config_init(&config);

    if (ghost_parse_args(argc, argv, &config) != 0)
    {
        return 1;
    }

    ghost_print_banner(&config);

    if (!check_admin_privileges())
    {
        fprintf(stderr,
            "[HATA] Bu program Yonetici (Administrator) olarak calistirilmalidir!\n"
            "       Sag tikla -> 'Yonetici olarak calistir' secin.\n");
        fprintf(stderr, "\nDevam etmek icin bir tusa basin...\n");
        getchar();
        return 1;
    }
    printf("[+] Yonetici yetkileri dogrulandi.\n");

    SetConsoleCtrlHandler(ctrl_c_handler, TRUE);

    build_filter(&config, filter_buf, sizeof(filter_buf));

    printf("[*] WinDivert handle aciliyor...\n");
    printf("    Filtre: %s\n", filter_buf);

    g_divert_handle = WinDivertOpen(
        filter_buf,
        WINDIVERT_LAYER_NETWORK,
        0,
        0
    );

    if (g_divert_handle == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        fprintf(stderr, "[HATA] WinDivertOpen basarisiz! Hata kodu: %lu\n", err);

        switch (err)
        {
        case ERROR_FILE_NOT_FOUND:
            fprintf(stderr,
                "  -> WinDivert64.sys bulunamadi.\n"
                "     DLL ve SYS dosyalarinin .exe ile ayni dizinde oldugundan emin olun.\n");
            break;
        case ERROR_ACCESS_DENIED:
            fprintf(stderr, "  -> Erisim reddedildi. Yonetici olarak calistirin.\n");
            break;
        case ERROR_INVALID_PARAMETER:
            fprintf(stderr, "  -> Gecersiz filtre ifadesi.\n");
            break;
        default:
            fprintf(stderr, "  -> Bilinmeyen hata. Driver imza durumunu kontrol edin.\n");
            break;
        }
        return 1;
    }

    printf("[+] WinDivert handle basariyla acildi.\n");
    tray_init();
    printf("[*] Outbound TCP + UDP trafik dinleniyor... (Ctrl+C ile durdurun)\n\n");

    while (g_running)
    {
        if (!WinDivertRecv(g_divert_handle, packet, sizeof(packet),
                           &packet_len, &addr))
        {
            DWORD err = GetLastError();
            if (err == ERROR_NO_DATA)
            {
                printf("[*] WinDivert handle kapatildi, dongu sonlaniyor.\n");
                break;
            }
            fprintf(stderr, "[UYARI] WinDivertRecv hatasi: %lu\n", err);
            continue;
        }

        ip_hdr   = NULL;
        tcp_hdr  = NULL;
        udp_hdr  = NULL;
        payload  = NULL;
        payload_len = 0;

        WinDivertHelperParsePacket(
            packet, packet_len,
            &ip_hdr,  NULL, NULL, NULL, NULL,
            &tcp_hdr, &udp_hdr,
            &payload, &payload_len,
            NULL, NULL
        );

        if (ip_hdr == NULL)
        {
            WinDivertHelperCalcChecksums(packet, packet_len, &addr, 0);
            WinDivertSend(g_divert_handle, packet, packet_len, NULL, &addr);
            continue;
        }

        if (!addr.Outbound && udp_hdr != NULL && g_original_dns_ip != 0)
        {
            uint16_t src_port = ntohs(udp_hdr->SrcPort);

            if (src_port == 53 || src_port == (uint16_t)config.dns_port)
            {
                UINT32 redirect_ip = inet_addr(config.dns_addr);

                if (ip_hdr->SrcAddr == redirect_ip)
                {
                    if (config.verbose)
                    {
                        printf("[#%llu] DNS/IN    %s:%d -> *  |  %u byte  "
                               "-> REWRITE SRC to original DNS\n",
                               ++pkt_count, config.dns_addr,
                               ntohs(udp_hdr->SrcPort), payload_len);
                    }

                    ip_hdr->SrcAddr  = g_original_dns_ip;
                    udp_hdr->SrcPort = htons(53);

                    WinDivertHelperCalcChecksums(packet, packet_len, &addr, 0);
                    WinDivertSend(g_divert_handle, packet, packet_len, NULL, &addr);
                    continue;
                }
            }

            WinDivertSend(g_divert_handle, packet, packet_len, NULL, &addr);
            continue;
        }

        if (udp_hdr != NULL)
        {
            uint16_t dst_port = ntohs(udp_hdr->DstPort);
            pkt_count++;

            if (config.block_quic && dst_port == 443)
            {
                if (config.verbose)
                {
                    char s_ip[32], d_ip[32];
                    print_ip_address(ip_hdr->SrcAddr, s_ip, sizeof(s_ip));
                    print_ip_address(ip_hdr->DstAddr, d_ip, sizeof(d_ip));
                    printf("[#%llu] QUIC/UDP  %s:%u -> %s:443  |  %u byte  "
                           "-> DROP (QUIC engellendi)\n",
                           pkt_count, s_ip, ntohs(udp_hdr->SrcPort),
                           d_ip, payload_len);
                }
                continue;
            }

            if (dst_port == 53 && config.dns_addr[0] != '\0')
            {
                UINT32 new_ip = inet_addr(config.dns_addr);

                if (new_ip != INADDR_NONE)
                {
                    g_original_dns_ip = ip_hdr->DstAddr;

                    if (config.verbose)
                    {
                        char old_ip[32];
                        print_ip_address(ip_hdr->DstAddr, old_ip, sizeof(old_ip));
                        printf("[#%llu] DNS/OUT   *:* -> %s:53  |  %u byte  "
                               "-> REDIRECT %s:%d\n",
                               pkt_count, old_ip, payload_len,
                               config.dns_addr, config.dns_port);
                    }

                    ip_hdr->DstAddr  = new_ip;
                    udp_hdr->DstPort = htons((uint16_t)config.dns_port);

                    WinDivertHelperCalcChecksums(packet, packet_len, &addr, 0);
                    WinDivertSend(g_divert_handle, packet, packet_len, NULL, &addr);
                    continue;
                }
            }

            WinDivertHelperCalcChecksums(packet, packet_len, &addr, 0);
            WinDivertSend(g_divert_handle, packet, packet_len, NULL, &addr);
            continue;
        }

        if (tcp_hdr == NULL)
        {
            WinDivertHelperCalcChecksums(packet, packet_len, &addr, 0);
            WinDivertSend(g_divert_handle, packet, packet_len, NULL, &addr);
            continue;
        }

        pkt_count++;

        parse_packet((const uint8_t *)payload, payload_len, &parsed);

        if (parsed.type == PKT_TYPE_HTTP)
        {
            int delta = apply_http_tricks(
                packet, &packet_len, ip_hdr, tcp_hdr,
                payload, payload_len, &config
            );

            if (delta != 0)
            {
                payload_len = (UINT)((int)payload_len + delta);
            }
        }

        if (config.verbose || parsed.type != PKT_TYPE_UNKNOWN)
        {
            print_ip_address(ip_hdr->SrcAddr, src_ip_str, sizeof(src_ip_str));
            print_ip_address(ip_hdr->DstAddr, dst_ip_str, sizeof(dst_ip_str));
            print_tcp_flags(tcp_hdr, flags_str, sizeof(flags_str));

            const char *type_str = get_packet_type_string(parsed.type);

            printf("[#%llu] %s  %s:%u -> %s:%u  |  %s |  %u byte  |  TTL: %u",
                   pkt_count,
                   type_str,
                   src_ip_str, ntohs(tcp_hdr->SrcPort),
                   dst_ip_str, ntohs(tcp_hdr->DstPort),
                   flags_str,
                   payload_len,
                   ip_hdr->TTL);

            if (parsed.hostname[0] != '\0')
            {
                printf("  |  Host: %s", parsed.hostname);
            }

            if (config.verbose && parsed.type == PKT_TYPE_TLS_HELLO &&
                parsed.sni_offset > 0)
            {
                printf("  |  SNI@offset:%u len:%u",
                       parsed.sni_offset, parsed.sni_length);
            }

            printf("\n");
        }

        {
            InjectResult result = inject_packet(
                g_divert_handle,
                packet,
                packet_len,
                &addr,
                ip_hdr,
                tcp_hdr,
                payload,
                payload_len,
                &config,
                &parsed
            );

            if (config.verbose && result == INJECT_OK)
            {
                if (config.do_fake)
                {
                    if (config.ttl_burst)
                        printf("    -> FAKE BURST: TTL=3,4,5,6,7 x5 (www.google.com)\n");
                    else
                        printf("    -> FAKE SENT: TTL=%d (www.google.com)\n",
                               config.fake_ttl);
                }
                if (config.do_fragment && payload_len > (UINT)config.frag_offset)
                {
                    UINT actual_split = (UINT)config.frag_offset;
                    if (parsed.type == PKT_TYPE_TLS_HELLO && parsed.sni_offset > 0)
                        actual_split = parsed.sni_offset;

                    printf("    -> FRAGMENTED: %u + %u byte "
                           "(split@%u %s)\n",
                           actual_split,
                           payload_len - actual_split,
                           actual_split,
                           (parsed.sni_offset > 0 ? "SNI-SPLIT" : "FIXED"));
                }
            }
            else if (result == INJECT_ERROR)
            {
                fprintf(stderr,
                    "[HATA] Paket enjeksiyon hatasi (paket kayboldu)\n");
            }
        }
    }

    printf("\n[*] Toplam yakalanan paket: %llu\n", pkt_count);

    if (g_divert_handle != INVALID_HANDLE_VALUE)
    {
        WinDivertClose(g_divert_handle);
        g_divert_handle = INVALID_HANDLE_VALUE;
        printf("[+] WinDivert handle kapatildi.\n");
    }

    tray_destroy();
    printf("[+] GhostSNI temiz sekilde kapatildi.\n");

    FreeConsole();
    ExitProcess(0);
    return 0;
}
