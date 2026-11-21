# PinjamLab Modul 3 — Penanganan Eksepsi & Pemrograman Modular

Proyek praktikum **Laboratorium Pemrograman II** untuk Modul 3. Pengerjaan dilakukan pada branch `m3-work` dengan bahasa C++20 dan Python 3.12. Tag `m3-start` dipertahankan sebagai penanda baseline dari starter dosen.

## Tujuan proyek

Menerapkan penanganan exception bertipe, membedakan exception dari kegagalan yang lebih tepat dikembalikan sebagai `Result`, memastikan cleanup berjalan pada jalur normal maupun exception, serta menjaga dependency rule pada arsitektur `domain / service / io / app`.

## Toolchain pengembangan

Toolchain yang dilaporkan tersedia pada komputer pengembangan Windows:

| Komponen | Versi |
|---|---|
| Git | 2.54.0.windows.1 |
| Python | 3.12.0 |
| pytest | 9.1.1 |
| CMake | 4.4.3 |
| g++ (MSYS2) | 16.2.0 |

## Menjalankan pengujian

Jalankan dari direktori proyek `Module3`.

**PowerShell:**

```powershell
& "C:\Program Files\Git\bin\bash.exe" ./run_tests.sh
```

**Git Bash:**

```bash
./run_tests.sh
```

Pemeriksaan sintaks Python secara terpisah:

```powershell
python -m compileall py/pinjamlab
```

`run_tests.sh` merupakan titik masuk pengujian terpadu. Script menjalankan pemeriksaan dependency rule, build dan test C++20, pemeriksaan golden output untuk CSV tetap dan CSV buatan sendiri, test Python, serta pemeriksaan cleanup/leak yang didukung oleh toolchain. Bila AddressSanitizer tidak tersedia atau tidak dapat dijalankan pada toolchain, gunakan fallback live-object counter yang disediakan proyek dan catat kondisi tersebut.

## Struktur proyek

```text
Module3/
├── cpp/
│   ├── include/pinjamlab/
│   │   ├── domain/       # Entitas, LoanError, Result
│   │   ├── service/      # LoanService, TxnGuard
│   │   ├── io/           # CSV reader, validator, report writer
│   │   └── app/          # Runner dan seed data
│   └── tests/            # Test C++
├── py/pinjamlab/
│   ├── domain/
│   ├── service/
│   ├── io/
│   └── app/
├── py/tests/             # Test Python
├── sample-data/
│   ├── bad-requests.csv
│   ├── expected-output.txt
│   ├── my-bad-requests.csv
│   └── my-expected-output.txt
├── scripts/
│   └── check_dependencies.py
├── docs/                 # Graf dependency sebelum/sesudah
├── evidence/             # Catatan/bukti pemeriksaan proyek
├── run_tests.sh
├── CMakeLists.txt
└── README.md
```

## Ringkasan pengerjaan Lab 1–3

### Lab 1 — Analisis kegagalan, input khusus, dan insiden

1. **Pemetaan handler:** menginventarisasi sumber `throw`/`raise`, blok `try`/`catch`/`except`, dan menilai apakah lapisan yang menangkap dapat bertindak. Snapshot starter dosen yang diterima sudah memiliki struktur modular; analisis dilakukan pada kode aktual, bukan mengklaim memecah satu god file yang tidak ditemukan di snapshot tersebut.
2. **Mode kegagalan tambahan:** merancang validasi field yang hanya berisi whitespace, format identifier yang tidak kanonis (`LP-NNN` dan `M-NNN`), dan nilai integer di luar rentang 32-bit. Ketiga kasus ditangani pada boundary validator sebagai kegagalan input yang diharapkan.
3. **Bring-your-own-input:** menambahkan lima baris pada `sample-data/my-bad-requests.csv` dan pesan target pada `sample-data/my-expected-output.txt`.
4. **Refleksi Ariane 5:** membahas penggunaan ulang asumsi, konversi di luar rentang, kebijakan shutdown, dan risiko common-mode failure pada sistem redundan.

Artefak utama: `LAB1_REPORT.md`, `LAB1_4_ARIANE5_REFLECTION.md`, `sample-data/my-bad-requests.csv`, dan `sample-data/my-expected-output.txt`.

### Lab 2 — Hirarki exception, Result, cleanup, dan error yang ditelan

1. **Exception orisinal:** `ReservationConflict` membawa tag equipment, member pemegang reservasi, dan member peminta. `FineCapReached` membawa loan ID, nilai denda hasil perhitungan, dan batas denda. Data terstruktur dipakai oleh handler aplikasi.
2. **Result from-scratch:** C++ menggunakan `Result<T>` buatan sendiri tanpa `std::expected`; Python menyediakan padanannya. Test membandingkan hasil `Result` dengan jalur throwing pada input yang sama.
3. **Cleanup:** C++ menggunakan `TxnGuard` berbasis RAII; Python menggunakan `txn_guard` berbasis `@contextmanager`. Keduanya melakukan rollback ketika terjadi exception dan commit ketika operasi berhasil.
4. **Diagnosis error dan leak:** pengujian aplikasi memastikan error baris dilaporkan dan error file yang bukan bagian keluarga `LoanError` tidak ditelan. Demonstrasi Defect B membandingkan raw pointer dengan `std::unique_ptr`; fallback live-object counter tersedia bila sanitizer tidak didukung toolchain.

Artefak utama: `LAB2_REPORT.md`, class exception pada `domain`, implementasi `Result`, `TxnGuard`, `txn_guard`, serta test C++ dan Python.

### Lab 3 — Prediksi, kontrak Result, dependency rule, dan reproduksibilitas

1. **Prediksi sebelum refactor:** keputusan tentang `EquipmentUnavailable` dicatat pada `PREDICTION.md`. Kondisi equipment sedang dipinjam diperlakukan sebagai outcome operasional yang diharapkan dan dikembalikan sebagai `Result<Loan>` dari `LoanService::borrow()`.
2. **Empat lapisan:** snapshot repository dosen sudah memisahkan `domain`, `service`, `io`, dan `app`. Pengerjaan mempertahankan batas tersebut dan tidak mengklaim memindahkan god file yang tidak terdapat pada snapshot. Graf sebelum/sesudah tersedia di `docs/dependency-graph-before.mmd` dan `docs/dependency-graph-after.mmd`.
3. **Pemeriksaan dependency rule:** `scripts/check_dependencies.py` dijalankan oleh `run_tests.sh` untuk mencegah domain bergantung pada `io`, `service`, atau `app`, termasuk akses file/console yang tidak semestinya.
4. **Perbandingan:** `LAB3_REPORT.md` memuat tabel strategi catch-all dibanding handler bertipe, serta tabel RAII C++ dibanding context manager Python.
5. **Reproduksi dari clone bersih:** prosedur terdapat pada `REPRODUCE_FROM_CLEAN.md`. Clone branch `m3-work`, jalankan `run_tests.sh`, lalu cocokkan pesan dan exit code untuk `bad-requests.csv` serta `my-bad-requests.csv`.

Artefak utama: `PREDICTION.md`, `LAB3_REPORT.md`, `REPRODUCE_FROM_CLEAN.md`, `scripts/check_dependencies.py`, dan folder `docs/`.

## Catatan pengujian yang tercatat

- **Baseline Windows sebelum perubahan:** output yang dibagikan menunjukkan tiga suite C++ berisi 8, 4, dan 5 test case yang lulus; pytest menunjukkan 20 test lulus; pemeriksaan dependency purity lulus dan fixed golden output cocok byte demi byte.
- **Lab 2 pada Windows:** pengguna mengonfirmasi bahwa pengujian setelah perbaikan selesai tanpa hasil `FAILED`.
- **Lab 3:** gunakan output aktual dari `run_tests.sh` pada branch `m3-work` sebagai bukti akhir. README ini tidak menyalin angka pengujian dari lingkungan operasi lain; simpan screenshot atau log dari komputer yang benar-benar digunakan untuk pengumpulan.

## Git dan branch

- Branch pengerjaan: `m3-work`.
- Tag baseline: `m3-start` (jangan dipindahkan ke commit implementasi).
- Branch remote: `origin/m3-work` setelah push.

Periksa status sebelum commit/push:

```powershell
git branch --show-current
git status
git log --oneline --decorate -5
```

Pastikan `m3-work` aktif dan working tree bersih setelah semua dokumen serta kode yang diperlukan di-commit.
