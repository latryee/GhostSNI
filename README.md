# GhostSNI 👻

**DPI Bypass Packet Manipulation Tool for Windows**

GhostSNI, Türkiye ve diğer ülkelerdeki ISS'lerin uyguladığı DPI (Deep Packet Inspection) engellemelerini atlatmak için geliştirilmiş bir araçtır. WinDivert kullanarak TCP/UDP trafiğini gerçek zamanlı olarak manipüle eder.

---

## ✨ Özellikler

- **TCP Fragmentation** — TLS Client Hello'yu SNI noktasından bölerek DPI'ı atlatır
- **Fake Packet Injection** — Düşük TTL'li sahte paketlerle DPI cihazını yanıltır
- **TTL Burst Mode** — TTL=3,4,5,6,7 ile 5 ayrı sahte paket göndererek farklı ISS'leri destekler
- **HTTP Host Tricks** — Header case değiştirme, boşluk silme, mix case
- **QUIC Engelleme** — UDP 443 trafiğini drop eder, tarayıcıyı TLS'e zorlar
- **DNS Yönlendirme** — DNS sorgularını alternatif sunucuya yönlendirir
- **System Tray** — Yeşil hayalet ikon 👻 ile arka planda sessiz çalışır
- **Sıfır Bağımlılık** — Sadece WinDivert gerektirir, ek kurulum yok

## 📦 Kurulum

1. [Releases](../../releases) sayfasından son sürümü indirin
2. ZIP'i bir klasöre çıkarın
3. İstediğiniz `.cmd` dosyasına **sağ tıklayın → Yönetici olarak çalıştır**

> **Not:** Windows Defender uyarı verebilir — bu WinDivert driver'ından kaynaklanan false positive'dir.

## 🚀 Kullanım

### Profiller — Çalışanı Bulana Kadar Dene

`.cmd` dosyasına sağ tıkla → Yönetici olarak çalıştır → GhostSNI arka planda başlar.
Kapatmak için tray'deki hayalet ikona tıkla → **"Durdur ve Çık"**.

**Biri çalışmazsa sıradakini dene:**

| # | Dosya | Açıklama | Oyun Uyumlu |
|---|-------|----------|:-----------:|
| ⭐ | `ghostsni_turkey.cmd` | **Ana profil** — Tüm özellikler + Yandex DNS | ❌ |
| 1 | `ghostsni_turkey_alt1.cmd` | Sabit TTL=3 + Yandex DNS | ❌ |
| 2 | `ghostsni_turkey_alt2.cmd` | Sabit TTL=5 + Yandex DNS | ❌ |
| 3 | `ghostsni_turkey_alt3.cmd` | DNS kapalı + QUIC kapalı | ✅ |
| 4 | `ghostsni_turkey_alt4.cmd` | Sabit TTL=3 + DNS kapalı + QUIC kapalı | ✅ |
| 5 | `ghostsni_turkey_alt5.cmd` | Büyük fragment (4 byte) + oyun uyumlu | ✅ |
| 6 | `ghostsni_turkey_alt6.cmd` | Sadece frag + HTTP tricks (fake paket yok) | ✅ |
| 7 | `ghostsni_turkey_alt7.cmd` | **Minimal** — sadece frag + host tricks (en güvenli) | ✅ |
| 8 | `ghostsni_turkey_alt8.cmd` | Frag + fake (passive DPI kapalı) | ✅ |
| 9 | `ghostsni_turkey_alt9.cmd` | Modeset 1 — En uyumlu (sadece HTTP tricks) | ✅ |
| 10 | `ghostsni_turkey_alt10.cmd` | Modeset 2 — Dengeli (frag + HTTP tricks) | ✅ |
| 11 | `ghostsni_turkey_alt11.cmd` | Cloudflare DNS (1.1.1.1) + tüm özellikler | ❌ |

> 💡 **Discord/oyun sorunları mı var?** → Alt3, Alt6 veya Alt7'yi dene (QUIC+DNS kapalı, en güvenli).

### Manuel Kullanım

```bash
GhostSNI.exe -f 2 -e -b -q -p -r -s -m --dns-addr 77.88.8.8 --dns-port 53
```

### Tüm Bayraklar

```
Modset'ler:
  -1              En uyumlu mod (sadece HTTP trick'leri)
  -2              Dengeli mod (frag + HTTP trick'leri)
  -3              Agresif mod (frag + fake paket)
  -4              Türkiye özel (tüm silahlar + DNS)

DPI Atlatma:
  -f, --frag <n>       TCP fragmentation bölme noktası (byte)
  -e, --fake           Sahte paket enjeksiyonu (fake Client Hello)
  -t, --ttl <n>        Sahte paket TTL değeri (varsayılan: 3)
  -b, --ttl-burst      TTL burst modu (3,4,5,6,7)
  -q, --block-quic     QUIC/HTTP3 trafiğini engelle

HTTP Manipülasyon:
  -p, --passive-dpi    Pasif DPI engelleme (RST/redirect drop)
  -r, --reverse-host   Host header case değiştir (Host → hoSt)
  -s, --remove-space   Host header boşluk sil
  -m, --mix-host       Hostname karışık case (eXaMpLe.CoM)

DNS:
  --dns-addr <ip>      DNS yönlendirme IP'si
  --dns-port <port>    DNS yönlendirme portu (varsayılan: 53)

Genel:
  --port <port>    Ek TCP port izle
  -v, --verbose    Detaylı log çıktısı
  -h, --help       Yardım
```

## 🔧 Nasıl Çalışır?

```
  Tarayıcı → HTTPS isteği (TLS Client Hello)
      │
      ▼
  WinDivert paket yakalar
      │
      ├─ 1. Sahte paket gönder (düşük TTL)
      │     → DPI sahte SNI'yı görür, gerçeği kaçırır
      │     → Sunucu TTL=0 olan paketi drop eder
      │
      ├─ 2. Gerçek paketi SNI'dan böl (fragmentation)
      │     → DPI iki parçayı birleştiremez
      │
      └─ Sunucuya ulaşır ✓
```

## 🖥️ Windows Servisi (Opsiyonel)

Windows açılınca otomatik başlatmak için:

```bash
service_install.cmd    (sağ tıkla → Yönetici olarak çalıştır)
service_remove.cmd     (kaldırmak için)
```

## ⚠️ Sorun Giderme

| Sorun | Çözüm |
|-------|-------|
| CMD anında kapanıyor | Sağ tıkla → **Yönetici olarak çalıştır** |
| "GhostSNI.exe bulunamadi" | `bin\` veya `build\` klasöründe exe olmalı |
| Discord açılmıyor | Alt3, Alt6 veya Alt7 kullan (QUIC+DNS kapalı) |
| Oyunlarda bağlantı hatası | Alt3-Alt10 arası dene (oyun uyumlu profiller) |
| Windows Defender uyarısı | False positive — izin ver |
| Hiçbir profil çalışmıyor | DoH (Secure DNS) açık mı kontrol et |

## ⚠️ Önemli Notlar

- **Yönetici yetkisi** gereklidir (WinDivert driver'ı yükler)
- **Windows Defender** ilk çalıştırmada uyarı verebilir — bu normaldir
- Tarayıcınızda **DoH (Secure DNS)** açmanız önerilir:
  - Chrome: `Ayarlar > Gizlilik > Güvenli DNS > Açık`
  - Firefox: `Ayarlar > Ağ > DNS over HTTPS`
- Tüm tarayıcılarla uyumlu — Chrome, Firefox, Edge, Opera, Brave
- **QUIC nedir?** UDP 443 üzerinden çalışan yeni nesil bir protokol. Discord ve bazı oyunlar kullanır. Engellemek (`-q`) tarayıcıyı eski TLS'e zorlar ama Discord/oyunları bozabilir.

## 🏗️ Kaynak Koddan Derleme

```bash
cmake -B build -S . -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 📜 Lisans

MIT License

## 🙏 Teşekkürler

- [WinDivert](https://reqrypt.org/windivert.html) — Bastiaan Bijl
- [GoodbyeDPI](https://github.com/ValdikSS/GoodbyeDPI) — ValdikSS (ilham kaynağı)
