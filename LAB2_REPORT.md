# Laporan Praktikum Modul 3 — Lab 2

## 1. Lab 2.1 — Hirarki exception kustom

Dua subclass baru dibuat pada C++ dan Python.

| Exception | Data terstruktur | Lapisan yang melempar | Lapisan yang menangani |
|---|---|---|---|
| `ReservationConflict` | Tag equipment, member yang memegang reservasi, member yang meminta | Domain `Equipment` melalui operasi service `reserveEquipment` / `reserve_equipment`, serta pemeriksaan pada `borrow` | `app` menangkap tipe spesifik sebelum `LoanError` umum dan menggunakan semua field konteks untuk menyusun pesan |
| `FineCapReached` | Loan ID, hasil perhitungan denda (sen), batas denda (sen) | Service `calculateFineCents` / `calculate_fine_cents` | Handler app `assessFineForDisplay` / `assess_fine_for_display` menangkap exception dan menyusun pesan memakai data terstruktur |

Fine dihitung menggunakan bilangan bulat dalam satuan sen, bukan floating point. Parameter negatif ditolak sebagai pelanggaran kontrak pemanggil. `FineCapReached` muncul bila hasil perhitungan melebihi cap.

Test membuktikan bahwa reservasi pemilik dapat digunakan, member lain ditolak, exception membawa konteks, dan handler app memakai konteks tersebut.

## 2. Lab 2.2 — Result<T> from-scratch

`Result<T>` C++ dan `Result[T]` Python menyimpan status sukses/gagal, value, dan pesan kegagalan. Validator memakai Result untuk kesalahan format di boundary. Test membandingkan validator dengan fungsi throwing yang menerapkan aturan sama pada input yang sama. Test mencakup input normal, bukan angka, angka nonpositif, dan integer yang melampaui rentang signed 32-bit; pesan kegagalan dibandingkan.

## 3. Lab 2.3 — RAII dan context manager

C++ memakai `TxnGuard`: constructor memulai transaksi, `commit()` menandai sukses, dan destructor melakukan rollback bila transaksi keluar tanpa commit. Python memakai `@contextmanager` `txn_guard`: `with` menjalankan commit pada jalur normal dan rollback pada jalur exception, kemudian `raise` meneruskan exception semula.

Test di kedua bahasa membuktikan bahwa catatan pending tidak tersisa setelah exception. Test Python tambahan mendokumentasikan bahwa jaminan context manager hilang bila pemanggil lupa menulis `with`.

## 4. Lab 2.4 — Diagnose-and-fix

### Defect A: error yang ditelan

Snapshot source dari dosen yang diunggah sudah memiliki penanganan bertipe `LoanError` di app, sehingga snapshot tersebut tidak menunjukkan `catch (...) { }` kosong pada jalur pemrosesan CSV. Oleh karena itu, laporan ini tidak mengklaim telah memperbaiki catch-all kosong yang tidak ada pada snapshot tersebut.

Sebagai regression/acceptance evidence, test memeriksa:

- 10 baris pada `bad-requests.csv` menghasilkan 10 pesan yang tepat dan exit code 1.
- 5 baris pada `my-bad-requests.csv` menghasilkan 5 pesan yang tepat dan exit code 1.
- `ReservationConflict` ditangani secara spesifik dan data konteks ditampilkan.
- Error membuka file yang tidak tersedia tidak diserap oleh app; error tersebut merambat ke pemanggil.

Di repository ini, test app menjaga agar error yang dikenal tidak menghilang secara senyap serta error yang tidak dikenal tidak ditangkap oleh handler yang tidak berhak menanganinya.

### Defect B: leak pada error path

`cpp/tests/leak_demo.cpp` memiliki dua jalur yang melempar exception setelah objek `LoanResource` dialokasikan:

- Jalur defect memakai `new` dan pointer mentah tanpa owner; destructor objek tidak berjalan saat stack unwinding, sehingga live-object counter tetap 1 dan AddressSanitizer melaporkan direct leak.
- Jalur perbaikan memakai `std::unique_ptr`; destructor dipanggil saat unwinding, live-object counter kembali 0, dan AddressSanitizer tidak melaporkan leak.

`run_tests.sh` menyimpan bukti pada `bin/asan_before.txt` dan `bin/asan_after.txt` bila ASan tersedia. Jika runtime ASan tidak tersedia, script menggunakan live-object counter sebagai fallback sesuai catatan pada README starter.

## 5. Hasil verifikasi paket

Verifikasi dilakukan pada lingkungan Linux dengan GNU g++ 14.2.0, Python 3.13.5, dan pytest 9.0.2.

| Pemeriksaan | Hasil |
|---|---:|
| C++ `test_errors` | 12 passed |
| C++ `test_raii` | 4 passed |
| C++ `test_result` | 5 passed |
| C++ `test_app` | 5 passed |
| Python | 29 passed |
| Golden output fixed CSV | Match |
| Golden output custom CSV | Match |
| AddressSanitizer raw pointer | Direct leak terdeteksi |
| AddressSanitizer `unique_ptr` | Bersih |

Hasil ini adalah hasil verifikasi paket di lingkungan tersebut. Mahasiswa tetap perlu menjalankan `bash ./run_tests.sh` di komputer sendiri dan menyimpan output lokal sebagai bukti praktikum.
