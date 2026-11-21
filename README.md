# PinjamLab Modul 3 — Penanganan Eksepsi & Pemrograman Modular

Proyek praktikum Laboratorium Pemrograman II. Paket ini mempertahankan struktur kode dosen dan melengkapi artefak Lab 1, Lab 2, serta perubahan perilaku Lab 3 dalam C++20 dan Python 3.12.

## Menjalankan pengujian

Dari Git Bash di direktori `Module3`:

```bash
./run_tests.sh
```

Dari PowerShell:

```powershell
bash ./run_tests.sh
```

Script menjalankan enam kelompok pemeriksaan:

1. **Dependency rule** — `scripts/check_dependencies.py` memeriksa C++ includes dan Python imports/calls agar `domain/` tidak bergantung pada `io`, `service`, `app`, file, atau console.
2. **Build C++20** — seluruh test dibangun dengan `-Wall -Wextra -Werror`.
3. **C++ tests** — hirarki exception, `Result<T>`, RAII, handler aplikasi, dan input CSV.
4. **Golden output** — hasil CLI dibandingkan byte demi byte dengan expected output untuk `bad-requests.csv` dan `my-bad-requests.csv`; perbedaan CRLF/LF dinormalisasi.
5. **Python tests** — pengujian padanan untuk Python, termasuk kecocokan `EquipmentUnavailable` sebagai Result.
6. **Defect B / cleanup** — AddressSanitizer mencoba mereproduksi raw-pointer leak dan membuktikan perbaikan `std::unique_ptr`. Jika libasan tidak tersedia, script menggunakan live-object counter sebagai fallback yang dijelaskan di modul.

Berkas bukti yang dihasilkan script berada di `bin/`, termasuk `asan_before.txt` dan `asan_after.txt` bila AddressSanitizer tersedia, atau `leak_before_fallback.txt` dan `leak_after_fallback.txt` bila tidak tersedia.

## Lab 1 — artefak yang dipertahankan

- `LAB1_REPORT.md` — pemetaan kegagalan, tabel mode kegagalan baru, input khusus, dan refleksi.
- `LAB1_4_ARIANE5_REFLECTION.md` — refleksi Ariane 5.
- `sample-data/my-bad-requests.csv` — lima input buatan sendiri.
- `sample-data/my-expected-output.txt` — pesan tepat yang diharapkan untuk lima input tersebut.

Validator menolak field kosong/whitespace, identifier di luar format `LP-NNN` atau `M-NNN`, angka bukan integer, integer di luar rentang 32-bit, dan durasi nonpositif. C++ dan Python menggunakan pesan yang sama.

## Lab 2.1 — dua exception orisinal

Dua subclass baru ditambahkan di kedua bahasa:

- `ReservationConflict`: membawa tag equipment, member pemegang reservasi, dan member yang mencoba meminjam. `Equipment` menyimpan reservasi; `LoanService` mengatur reservasi dan menolak peminjaman oleh member lain. Handler `app` menggunakan data terstruktur untuk membuat pesan.
- `FineCapReached`: membawa loan ID, denda hasil perhitungan dalam sen, dan batas denda dalam sen. `LoanService.calculateFineCents()` / `calculate_fine_cents()` melempar exception bila hasil perhitungan melebihi batas; handler app membentuk pesan dengan menggunakan data exception.

Test memeriksa kedua jalur, getter/atribut konteks, dan handler tingkat aplikasi.

## Lab 2.2 — `Result<T>` from-scratch

`domain/result.hpp` dan `domain/result.py` adalah implementasi `Result` buatan sendiri. Validator mengembalikan Result untuk kegagalan format yang diharapkan. Test membandingkan hasil `Result` dengan versi throwing untuk input yang sama, termasuk pesan kegagalan dan integer overflow. Tidak menggunakan `std::expected`.

## Lab 2.3 — cleanup deterministik

- C++: `TxnGuard` memakai RAII; destructor melakukan rollback jika belum commit.
- Python: `txn_guard()` memakai `@contextmanager`; block `with` melakukan rollback pada exception dan meneruskan kembali exception tersebut.
- Test memastikan commit berhasil dan sumber daya/record pending dibersihkan pada jalur exception.

## Lab 2.4 — error yang ditelan dan leak pada error path

**Defect A:** kode sumber yang diberikan dalam arsip ini sudah memiliki handler bertipe `LoanError` di app; karena itu tidak tepat menyatakan bahwa snapshot awal yang diterima masih memiliki `catch-all` kosong. Paket ini menambahkan regression test untuk sepuluh baris tetap, lima input buatan sendiri, handler khusus `ReservationConflict`, dan memastikan kesalahan file yang tidak dikenal tetap merambat (tidak ditelan). Hasil penolakan harus memiliki pesan per baris dan exit code 1.

**Defect B:** `cpp/tests/leak_demo.cpp` memperlihatkan raw pointer yang kehilangan kepemilikan ketika exception unwind dan versi `std::unique_ptr` yang membersihkan objek secara deterministik. `run_tests.sh` mencoba AddressSanitizer. Jika toolchain tidak memiliki runtime ASan yang berfungsi, script membuktikan jumlah objek hidup melalui destructor/live-object counter, sesuai fallback yang dijelaskan README starter.

## Hasil verifikasi paket

Paket ini diverifikasi di lingkungan Linux dengan GNU g++ 14.2.0, Python 3.13.5, dan pytest 9.0.2. Hasil verifikasi Lab 3: 12 test C++ exception, 4 test RAII, 7 test Result, 5 test app, 31 test Python, dua golden-output check, dependency checker lulus, dan ASan raw-pointer leak terdeteksi sementara jalur `unique_ptr` bersih.

Versi toolchain di atas adalah lingkungan verifikasi paket ini, bukan klaim bahwa pengujian tersebut sudah dijalankan pada komputer mahasiswa. Jalankan `run_tests.sh` kembali di lingkungan Windows Anda sebelum commit/push.


## Lab 3.1 — `EquipmentUnavailable` sebagai `Result`

Keputusan prediksi dicatat terlebih dahulu di `PREDICTION.md`. `LoanService::borrow()` /
`LoanService.borrow()` sekarang mengembalikan `Result<Loan>` untuk kondisi equipment
sudah dipinjam, karena itu adalah outcome yang wajar dalam operasi peminjaman dan
caller app dapat melaporkannya atau menawarkan waiting list. Kegagalan lain seperti
asset/member tidak dikenal, durasi melanggar aturan, quota, dan reservation conflict
tetap memakai exception bertipe.

Adapter `borrowThrowingForComparison()` / `borrow_throwing_for_comparison()` hanya
untuk test pembanding perilaku lama. Jalur produksi app memeriksa `ok()` / `ok`.
Test membandingkan pesan failure Result dengan exception lama dan membandingkan nilai
Loan pada successful borrow.

## Lab 3.2 — empat lapisan dan dependency rule

Struktur awal repository yang diunggah sudah memiliki `domain/`, `service/`, `io/`,
dan `app/`. Karena itu tidak ada pemindahan file god-file yang dapat dibuktikan pada
snapshot aktual. Graf sebelum/sesudah dan alasan keputusan tersebut ada pada
`LAB3_REPORT.md` dan `docs/`. Dependency rule kini diperiksa oleh
`scripts/check_dependencies.py` dan kegagalan pemeriksaan menghentikan `run_tests.sh`.

## Lab 3.3 — tabel perbandingan

`LAB3_REPORT.md` berisi dua tabel wajib: strategi catch-all dibanding penanganan
bertingkat/bertipe, dan mekanisme cleanup C++ RAII dibanding Python `with`.

## Lab 3.4 — reproduce from clean

Ikuti `REPRODUCE_FROM_CLEAN.md`. Rekan harus clone branch `m3-work`, menjalankan
`./run_tests.sh`, dan mereproduksi pesan dari `bad-requests.csv` serta
`my-bad-requests.csv` dengan urutan dan exit code yang sama.
