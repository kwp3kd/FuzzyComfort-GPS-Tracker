# 📡 FuzzyComfort-GPS Tracker

Repositori ini berisi dokumentasi kode sumber untuk proyek mata kuliah **Internet of Things (IoT)**: **Sistem Monitoring Suhu, Kelembapan Udara, dan Geolokasi Berbasis Mobile**.

Proyek ini menggunakan mikrokontroler **ESP32** yang terintegrasi dengan sensor **DHT22** dan modul **GPS Neo-6M**. Data yang terbaca dikirimkan secara *real-time* ke *cloud* **ThingSpeak** dan divisualisasikan melalui *dashboard* web kustom.

---

## 📂 Struktur Direktori

Repositori ini dibagi menjadi dua bagian utama:

*   📁 **`ESP32_Code/`**: Berisi *source code* (file `.ino`) yang di-*upload* menggunakan Arduino IDE ke board ESP32.
*   📁 **`Web_Dashboard/`**: Berisi file antarmuka web (`index.html`) untuk memvisualisasikan data secara spasial dan grafik.

---

## 🛠️ 1. Penjelasan Kode Arduino IDE (ESP32)

Folder `ESP32_Code` berisi program utama untuk membaca dan mengirimkan data.

**Fitur dalam kode ESP32:**
*   **Pembacaan Sensor & Validasi:** Membaca data dari sensor DHT22 (Suhu dan Kelembapan) dan GPS Neo-6M (Latitude dan Longitude). Terdapat mekanisme validasi (NaN) agar data *error* tidak terkirim ke *cloud*.
*   **Logika Fuzzy:** Menghitung *Skor Fuzzy* (tingkat kenyamanan lingkungan) berdasarkan nilai suhu dan kelembapan yang terbaca.
*   **Tampilan OLED:** Menampilkan data secara *real-time* pada layar OLED 0.91 inci.
*   **Pengiriman ke Cloud:** Mengirimkan data (Latitude, Longitude, Kelembapan, Suhu, Skor Fuzzy) ke ThingSpeak (Field 1-5) setiap 20 detik menggunakan koneksi WiFi.

**Cara Menjalankan:**
1. Buka file `.ino` menggunakan Arduino IDE.
2. Pastikan *Board Manager* ESP32 dan seluruh *Library* (DHT, TinyGPSPlus, ThingSpeak, Adafruit SSD1306/GFX) sudah terpasang.
3. Sesuaikan `SSID`, `Password WiFi`, `Channel ID`, dan `Write API Key` ThingSpeak milik Anda di dalam kode.
4. *Upload* kode ke board ESP32.

---

## 🗺️ 2. Penjelasan Web Dashboard (HTML)

Folder `Web_Dashboard` berisi *source code* berbasis HTML, CSS (Bootstrap), dan JavaScript (Leaflet.js & Chart.js).

**Fitur dalam Dashboard Web:**
*   **Peta Interaktif (Timeline):** Menampilkan rute perjalanan alat secara visual menggunakan peta OpenStreetMap dan Leaflet.js.
*   **Segmentasi Sesi:** Memisahkan data perjalanan berdasarkan hari dan waktu (jika ada jeda terlalu lama, sesi baru akan dibuat).
*   **Sinkronisasi Data:** Menarik data langsung dari API ThingSpeak secara otomatis (setiap 15 detik).
*   **Grafik & Filter:** Menampilkan grafik tren suhu, kelembapan, dan skor fuzzy dari sesi yang dipilih. Terdapat filter bulan dan tanggal untuk mempermudah pencarian data histori.
*   **Indikator Warna:** Titik pada peta dan label status diwarnai berdasarkan kategori skor kenyamanan (Nyaman, Cukup, Kurang, Buruk).

**Cara Menjalankan:**
1. Buka file `index.html` menggunakan *browser* (Google Chrome, Edge, atau Firefox).
2. *Dashboard* akan otomatis memuat data dari ThingSpeak Channel yang sudah dikonfigurasi di dalam kode (pastikan `CHANNEL_ID` dan `READ_API_KEY` sudah sesuai).
3. *(Opsional)* Anda dapat mengaktifkan fitur **GitHub Pages** pada repositori ini agar *dashboard* web dapat diakses secara *online* melalui tautan.

---
*Dibuat untuk memenuhi tugas proyek mata kuliah Internet of Things (IoT).*
