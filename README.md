# GhostSNI 👻

**DPI Bypass Packet Manipulation Tool for Windows**

GhostSNI, Türkiye ve diğer ülkelerdeki ISS'lerin uyguladığı DPI (Deep Packet Inspection) engellemelerini atlatmak için geliştirilmiş bir araçtır. WinDivert kullanarak TCP/UDP trafiğini gerçek zamanlı olarak manipüle eder.

---

## ✨ Özellikler

- **TCP Fragmentation** — TLS Client Hello'yu SNI noktasından bölerek DPI'ı atlatır
- **Reverse Fragmentation** — Fragment'ları ters sırada göndererek sıralı birleştirme yapan DPI'ı atlatır
- **Fake Packet Injection** — Düşük TTL'li sahte paketlerle DPI cihazını yanıltır
- **TTL Burst Mode** — TTL=3,4,5,6,7 ile 5 ayrı sahte paket göndererek farklı ISS'leri destekler
- **Wrong Checksum** — Sahte paketleri yanlış TCP checksum ile gönderir (DPI görür, sunucu drop eder)
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

`.cmd` dosyasına çift tıkla → CMD otomatik kapanır → sadece tray'de 👻 ikon kalır.
Kapatmak için ikona tıkla → **"Durdur ve Çık"**.

**Biri çalışmazsa sıradakini dene:**

| # | Dosya | Açıklama |
|---|-------|----------|
| ⭐ | `ghostsni_turkey.cmd` | **Ana profil** — Tüm silahlar açık + Yandex DNS |
| 1 | `ghostsni_turkey_alt1.cmd` | Sabit TTL=3 (burst yerine tek TTL) |
| 2 | `ghostsni_turkey_alt2.cmd` | Sabit TTL=5 (daha uzak DPI cihazları) |
| 3 | `ghostsni_turkey_alt3.cmd` | DNS yönlendirme kapalı (DoH kullananlar için) |
| 4 | `ghostsni_turkey_alt4.cmd` | QUIC block kapalı + wrong-chksum kapalı |
| 5 | `ghostsni_turkey_alt5.cmd` | Büyük fragment bölme noktası (4 byte) |
| 6 | `ghostsni_turkey_alt6.cmd` | Minimal mod — sadece frag + HTTP tricks |

### Manuel Kullanım

```bash
GhostSNI.exe -f 2 -e -b --wrong-chksum --reverse-frag -q -p -r -s -m --dns-addr 77.88.8.8 --dns-port 53
```

### Tüm Bayraklar

```
Modset'ler:
  -1              En uyumlu mod (sadece HTTP trick'leri)
  -2              Dengeli mod (frag + HTTP trick'leri)
  -3              Agresif mod (frag + fake + reverse frag)
  -4              Türkiye özel (tüm silahlar + DNS)

DPI Atlatma:
  -f, --frag <n>       TCP fragmentation bölme noktası (byte)
  -e, --fake           Sahte paket enjeksiyonu (fake Client Hello)
  -t, --ttl <n>        Sahte paket TTL değeri (varsayılan: 3)
  -b, --ttl-burst      TTL burst modu (3,4,5,6,7)
  --wrong-chksum       Sahte paketi yanlış TCP checksum ile gönder
  --reverse-frag       Fragment'ları ters sırada gönder
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
      ├─ 1. Sahte paket gönder (düşük TTL / yanlış checksum)
      │     → DPI sahte SNI'yı görür, gerçeği kaçırır
      │     → Sunucu checksum hatalı paketi drop eder
      │
      ├─ 2. Gerçek paketi SNI'dan böl (reverse fragment)
      │     → Fragment 2 önce, Fragment 1 sonra
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

## ⚠️ Önemli Notlar

- **Yönetici yetkisi** gereklidir (WinDivert driver'ı yükler)
- **Windows Defender** ilk çalıştırmada uyarı verebilir — bu normaldir
- Tarayıcınızda **DoH (Secure DNS)** açmanız önerilir:
  - Chrome: `Ayarlar > Gizlilik > Güvenli DNS > Açık`
  - Firefox: `Ayarlar > Ağ > DNS over HTTPS`
- Tüm tarayıcılarla uyumlu — Chrome, Firefox, Edge, Opera, Brave

## 🏗️ Kaynak Koddan Derleme

```bash
cmake -B build -S . -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 📜 Lisans

MIT License

## 🙏 Teşekkürler

- [WinDivert](https://reqrypt.org/windivert.html) — Bastiaan Bijl

