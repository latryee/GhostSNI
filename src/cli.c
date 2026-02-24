#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"

static int parse_int_arg(const char *flag_name, char *argv[],
                         int idx, int argc, int *out_val)
{
    if (idx + 1 >= argc)
    {
        fprintf(stderr, "[HATA] '%s' bayragi bir deger bekliyor.\n", flag_name);
        return -1;
    }

    char *endptr = NULL;
    long val = strtol(argv[idx + 1], &endptr, 10);

    if (endptr == argv[idx + 1] || *endptr != '\0')
    {
        fprintf(stderr, "[HATA] '%s' icin gecersiz deger: '%s'\n",
                flag_name, argv[idx + 1]);
        return -1;
    }

    *out_val = (int)val;
    return 0;
}

static int parse_str_arg(const char *flag_name, char *argv[],
                         int idx, int argc, char *out_buf, size_t buf_len)
{
    if (idx + 1 >= argc)
    {
        fprintf(stderr, "[HATA] '%s' bayragi bir deger bekliyor.\n", flag_name);
        return -1;
    }

    snprintf(out_buf, buf_len, "%s", argv[idx + 1]);
    return 0;
}

static void apply_modeset(GhostConfig *cfg, int modeset)
{
    switch (modeset)
    {
    case 1:
        cfg->do_passive_dpi     = 1;
        cfg->do_host_replace    = 1;
        cfg->do_host_remove_space = 1;
        break;

    case 2:
        cfg->do_passive_dpi     = 1;
        cfg->do_host_replace    = 1;
        cfg->do_host_remove_space = 1;
        cfg->do_fragment        = 1;
        cfg->frag_offset        = 2;
        break;

    case 3:
        cfg->do_passive_dpi     = 1;
        cfg->do_host_replace    = 1;
        cfg->do_host_remove_space = 1;
        cfg->do_fragment        = 1;
        cfg->frag_offset        = 2;
        cfg->do_fake            = 1;
        cfg->fake_ttl           = 3;
        cfg->ttl_burst          = 1;
        cfg->reverse_frag       = 1;
        break;

    case 4:
        cfg->do_passive_dpi     = 1;
        cfg->do_host_replace    = 1;
        cfg->do_host_remove_space = 1;
        cfg->do_host_mix_case   = 1;
        cfg->do_fragment        = 1;
        cfg->frag_offset        = 2;
        cfg->do_fake            = 1;
        cfg->fake_ttl           = 3;
        cfg->ttl_burst          = 1;
        cfg->wrong_chksum       = 1;
        cfg->reverse_frag       = 1;
        cfg->block_quic         = 1;
        snprintf(cfg->dns_addr, MAX_DNS_ADDR_LEN, "77.88.8.8");
        cfg->dns_port           = 53;
        break;

    default:
        fprintf(stderr, "[UYARI] Gecersiz modeset: %d (desteklenen: 1-4)\n",
                modeset);
        break;
    }

    cfg->modeset = modeset;
}

void ghost_config_init(GhostConfig *cfg)
{
    memset(cfg, 0, sizeof(GhostConfig));
    cfg->fake_ttl = 3;
    cfg->ttl_burst = 0;
    cfg->dns_port = 53;
}

int ghost_parse_args(int argc, char *argv[], GhostConfig *cfg)
{
    int i;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            ghost_print_help(argv[0]);
            exit(0);
        }

        else if (strcmp(argv[i], "-1") == 0) { apply_modeset(cfg, 1); }
        else if (strcmp(argv[i], "-2") == 0) { apply_modeset(cfg, 2); }
        else if (strcmp(argv[i], "-3") == 0) { apply_modeset(cfg, 3); }
        else if (strcmp(argv[i], "-4") == 0) { apply_modeset(cfg, 4); }

        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--frag") == 0)
        {
            if (parse_int_arg(argv[i], argv, i, argc, &cfg->frag_offset) != 0)
                return -1;
            cfg->do_fragment = 1;
            i++;
        }
        else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--fake") == 0)
        {
            cfg->do_fake = 1;
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--ttl") == 0)
        {
            if (parse_int_arg(argv[i], argv, i, argc, &cfg->fake_ttl) != 0)
                return -1;
            i++;
        }
        else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--ttl-burst") == 0)
        {
            cfg->ttl_burst = 1;
        }
        else if (strcmp(argv[i], "--wrong-chksum") == 0)
        {
            cfg->wrong_chksum = 1;
        }
        else if (strcmp(argv[i], "--reverse-frag") == 0)
        {
            cfg->reverse_frag = 1;
        }
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--block-quic") == 0)
        {
            cfg->block_quic = 1;
        }

        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--passive-dpi") == 0)
        {
            cfg->do_passive_dpi = 1;
        }
        else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reverse-host") == 0)
        {
            cfg->do_host_replace = 1;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--remove-space") == 0)
        {
            cfg->do_host_remove_space = 1;
        }
        else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mix-host") == 0)
        {
            cfg->do_host_mix_case = 1;
        }

        else if (strcmp(argv[i], "--dns-addr") == 0)
        {
            if (parse_str_arg(argv[i], argv, i, argc,
                              cfg->dns_addr, MAX_DNS_ADDR_LEN) != 0)
                return -1;
            i++;
        }
        else if (strcmp(argv[i], "--dns-port") == 0)
        {
            if (parse_int_arg(argv[i], argv, i, argc, &cfg->dns_port) != 0)
                return -1;
            i++;
        }

        else if (strcmp(argv[i], "--port") == 0)
        {
            if (parse_int_arg(argv[i], argv, i, argc, &cfg->extra_port) != 0)
                return -1;
            i++;
        }

        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            cfg->verbose = 1;
        }
        else if (strcmp(argv[i], "--silent") == 0)
        {
            cfg->silent = 1;
        }

        else
        {
            fprintf(stderr, "[HATA] Bilinmeyen arguman: '%s'\n", argv[i]);
            fprintf(stderr, "       Kullanim bilgisi icin: %s --help\n", argv[0]);
            return -1;
        }
    }

    return 0;
}

void ghost_print_help(const char *program_name)
{
    printf(
    "GhostSNI v%s - DPI Bypass Packet Manipulation Tool\n"
    "Kullanim: %s [BAYRAKLAR]\n"
    "\n"
    "Modset'ler (Hazir Profiller):\n"
    "  -1                   En uyumlu mod (HTTP trick'leri)\n"
    "  -2                   Dengeli mod (frag + HTTP trick'leri)\n"
    "  -3                   Agresif mod (frag + fake paket)\n"
    "  -4                   Turkiye ozel mod (tum silahlar + DNS redir)\n"
    "\n"
    "DPI Atlatma:\n"
    "  -f, --frag <n>       TCP fragmentation (Client Hello bolme noktasi)\n"
    "  -e, --fake           Sahte paket enjeksiyonu (fake Client Hello)\n"
    "  -t, --ttl <n>        Sahte paket TTL degeri (varsayilan: 3)\n"
    "  -q, --block-quic     QUIC/HTTP3 trafiğini engelle\n"
    "  --wrong-chksum       Sahte paket yanlis TCP checksum ile gonder\n"
    "  --reverse-frag       Fragment'lari ters sirada gonder\n"
    "\n"
    "HTTP Manipulasyon:\n"
    "  -p, --passive-dpi    Pasif DPI engelleme (RST/redirect drop)\n"
    "  -r, --reverse-host   Host header case degistir (Host -> hoSt)\n"
    "  -s, --remove-space   Host header bosluk sil (Host: -> Host:)\n"
    "  -m, --mix-host       Host degeri karisik case (example.com -> eXaMpLe.CoM)\n"
    "\n"
    "DNS Yonlendirme:\n"
    "  --dns-addr <ip>      DNS sorgularini bu IP'ye yonlendir\n"
    "  --dns-port <port>    DNS yonlendirme portu (varsayilan: 53)\n"
    "\n"
    "Ag Ayarlari:\n"
    "  --port <port>        Ek TCP port izle (80/443 disinda)\n"
    "\n"
    "Genel:\n"
    "  -v, --verbose        Detayli log ciktisi\n"
    "  -h, --help           Bu yardim mesajini goster\n"
    "\n"
    "Ornekler:\n"
    "  %s -4                                    Turkiye profili (onerilir)\n"
    "  %s -f 2 -e --ttl 3 -p -r -s             Manuel ayarlar\n"
    "  %s -3 --dns-addr 1.1.1.1 --dns-port 1253  Agresif + DNS\n"
    "\n",
    GHOSTSNI_VERSION, program_name, program_name, program_name, program_name);
}

void ghost_print_banner(const GhostConfig *cfg)
{
    printf("=============================================================\n");
    printf("  GhostSNI v%s - DPI Bypass Packet Manipulation Tool\n", GHOSTSNI_VERSION);
    printf("=============================================================\n");

    if (cfg->modeset > 0)
        printf("  Profil  : Modeset -%d\n", cfg->modeset);

    printf("  Ayarlar :\n");

    if (cfg->do_fragment)
        printf("    [+] TCP Fragmentation : offset = %d byte%s\n", cfg->frag_offset,
               cfg->reverse_frag ? " (TERS SIRA)" : "");
    if (cfg->do_fake)
        printf("    [+] Sahte Paket       : TTL = %d%s%s\n",
               cfg->fake_ttl,
               cfg->ttl_burst ? " (BURST: 3,4,5,6,7)" : "",
               cfg->wrong_chksum ? " + YANLIS CHECKSUM" : "");
    if (cfg->block_quic)
        printf("    [+] QUIC Engelleme    : aktif\n");
    if (cfg->do_passive_dpi)
        printf("    [+] Pasif DPI         : aktif\n");
    if (cfg->do_host_replace)
        printf("    [+] Host Degistirme   : aktif (Host -> hoSt)\n");
    if (cfg->do_host_remove_space)
        printf("    [+] Bosluk Silme      : aktif\n");
    if (cfg->do_host_mix_case)
        printf("    [+] Mix Case          : aktif\n");
    if (cfg->dns_addr[0] != '\0')
        printf("    [+] DNS Yonlendirme   : %s:%d\n", cfg->dns_addr, cfg->dns_port);
    if (cfg->extra_port > 0)
        printf("    [+] Ek Port           : %d\n", cfg->extra_port);
    if (cfg->verbose)
        printf("    [+] Verbose Log       : aktif\n");

    if (!cfg->do_fragment && !cfg->do_fake && !cfg->do_passive_dpi &&
        !cfg->do_host_replace && !cfg->do_host_remove_space &&
        !cfg->do_host_mix_case && !cfg->block_quic &&
        cfg->dns_addr[0] == '\0')
    {
        printf("    [-] Sadece dinleme modu (passthrough)\n");
    }

    printf("=============================================================\n\n");
}
