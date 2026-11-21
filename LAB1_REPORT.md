# Lab 1 — Analisis Kegagalan, Baris Buruk, dan Insiden

**Mata kuliah:** Laboratorium Pemrograman II  
**Modul:** Modul 3 — Penanganan Eksepsi & Pemrograman Modular  
**Proyek:** PinjamLab  
**Branch kerja:** `m3-work`  

## Konteks dan batas pemeriksaan

Repository yang diberikan dosen sudah berbentuk modular: `domain/`, `service/`, `io/`, dan `app/`. Pada ZIP yang diperiksa tidak ditemukan satu *god file* tunggal. Karena itu, bagian 1.1 di bawah memetakan semua sumber exception dan blok penanganannya pada implementasi aktual C++ dan Python, bukan mengasumsikan ada file monolitik yang tidak tersedia.

Baseline `./run_tests.sh` yang dijalankan sebelum perubahan dinyatakan berhasil oleh mahasiswa: pemeriksaan dependency rule lulus, seluruh 18 test case C++ lulus, 20 test Python lulus, dan keluaran C++ cocok byte-for-byte dengan `sample-data/expected-output.txt`. Jumlah test case yang dilaporkan adalah 38 (18 C++ + 20 Python); pemeriksaan purity dan golden-file merupakan pemeriksaan tambahan.

**Catatan penting:** Lab 1 ini menambahkan dokumen rancangan dan berkas input khusus. Kode produksi tidak diubah pada tahap Lab 1. Karena tiga guard baru di bagian 1.2 belum diimplementasikan pada starter, `my-expected-output.txt` adalah kontrak pesan yang ditargetkan setelah guard tersebut diterapkan, bukan klaim bahwa runner starter saat ini sudah menghasilkan pesan itu.

---

## 1.1 Pemetaan sumber kegagalan dan handler

### A. C++20

| Lokasi | Sumber kegagalan / blok | Apa yang dilakukan | Penilaian lapisan |
|---|---|---|---|
| `cpp/include/pinjamlab/io/csv_reader.hpp:28` | `throw std::runtime_error("cannot open input: " + path)` | Melaporkan file input yang gagal dibuka melalui exception | Belum ada handler tingkat app khusus. `runner` belum masuk ke loop baris ketika `read()` gagal; exception dapat keluar dari `main`. Perlu handler di batas app untuk pesan dan exit code yang jelas. |
| `cpp/include/pinjamlab/domain/equipment.hpp:31-33` | `throw EquipmentUnavailable(tag_)` | Menolak perubahan state equipment yang sudah tidak tersedia | Tepat sebagai aturan domain; domain melaporkan kegagalan dan tidak memilih respons UI. |
| `cpp/include/pinjamlab/domain/member.hpp:33-35` | `throw QuotaExceeded(activeLoans_, maxLoans())` | Menolak peminjaman ketika kuota member penuh | Tepat sebagai aturan domain. Handler yang dapat memberi tahu member pinjaman mana yang harus dikembalikan berada di app. |
| `cpp/include/pinjamlab/domain/result.hpp:31-34` | `throw std::logic_error(...)` dari `value()` ketika `Result` gagal | Menandai pemanggilan API yang melanggar prasyarat; bukan kesalahan input normal | Tepat sebagai kesalahan kontrak/programmer. Jangan mengubahnya menjadi penolakan baris biasa; pemanggil harus memeriksa `ok()` lebih dahulu. |
| `cpp/include/pinjamlab/service/loan_service.hpp:45-58` | `UnknownAssetTag`, `UnknownMember`, `InvalidLoanDuration`, `EquipmentUnavailable`, `QuotaExceeded` | Menerapkan guard bisnis sebelum perubahan state | Tepat: service/domain mengangkat kegagalan bertipe; service tidak mengubahnya menjadi pesan pengguna. `app/runner.hpp` menangkap keluarga `LoanError`. |
| `cpp/include/pinjamlab/service/loan_service.hpp:63-68` | `try { eq.markLoaned(); } catch (...) { mem.undoRecordLoan(); throw; }` | Jika perubahan equipment gagal setelah counter member dinaikkan, counter member dikembalikan; exception asli dilempar ulang | Tepat untuk kompensasi lokal. Ini bukan catch yang menelan error: `throw;` mempertahankan propagasi. `TxnGuard` kemudian me-rollback ledger saat keluar dari scope tanpa commit. |
| `cpp/include/pinjamlab/service/txn_guard.hpp:34-42` | `try { db_.rollback(); } catch (...)` di destructor `noexcept` | Menangkap kegagalan rollback dan menulis `rollback failed during unwind` ke `std::cerr` agar destructor tidak melempar ketika stack unwinding | Catch-all dapat dibenarkan di sini karena destructor harus `noexcept`. Informasi kegagalan rollback tetap dicatat; exception kedua tidak boleh keluar dari destructor. |
| `cpp/include/pinjamlab/app/runner.hpp:48-55` | `try { svc.borrow(...) } catch (const LoanError& e)` | Menghitung baris yang ditolak dan menyimpan pesan dengan nomor baris | Tepat: app adalah lapisan yang dapat melaporkan kesalahan kepada pengguna dan menentukan exit code. Exception non-`LoanError` dibiarkan merambat. |

### B. Python 3.12

| Lokasi | Sumber kegagalan / blok | Apa yang dilakukan | Penilaian lapisan |
|---|---|---|---|
| `py/pinjamlab/io/csv_reader.py:20-31` | `open()` dapat menghasilkan `FileNotFoundError` / `OSError`; tidak ditangkap di sini | Gagal membaca file input | Belum ada handler khusus di `app/main.py`; error dapat keluar sebagai traceback. Handler untuk pesan dan exit code sebaiknya di batas app. |
| `py/pinjamlab/domain/equipment.py:27-30` | `raise EquipmentUnavailable(self.tag)` | Menolak equipment yang sudah dipinjam | Tepat sebagai aturan domain. |
| `py/pinjamlab/domain/member.py:33-36` | `raise QuotaExceeded(...)` | Menolak peminjaman ketika kuota penuh | Tepat sebagai aturan domain. |
| `py/pinjamlab/domain/result.py:25-28` | `raise RuntimeError(...)` dari `value` ketika Result gagal | Menandai pemanggilan API yang melanggar prasyarat | Tepat sebagai kesalahan kontrak/programmer, bukan input normal. |
| `py/pinjamlab/service/loan_service.py:44-61` | `raise UnknownAssetTag`, `UnknownMember`, `InvalidLoanDuration`, `EquipmentUnavailable`, `QuotaExceeded` | Menegakkan guard bisnis | Tepat: service/domain menghasilkan error bertipe; `app/runner.py` menangkap keluarga `LoanError`. |
| `py/pinjamlab/service/loan_service.py:66-70` | `try: equipment.mark_loaned()` / `except Exception` / `raise` | Mengembalikan counter member jika operasi equipment gagal, lalu melempar ulang error | Tepat untuk kompensasi lokal. Exception tidak ditelan. |
| `py/pinjamlab/service/txn_guard.py:20-32` | `try` / `except Exception`; di dalamnya ada `try` / `except Exception`; lalu `raise` dan `else` | Pada error body: rollback, catat jika rollback juga gagal, lalu lempar ulang exception asli. Pada jalur sukses: commit | Tepat untuk context manager transaksi. Catch bersarang hanya menahan kegagalan rollback agar tidak mengganti exception utama; exception dari body tetap dilempar ulang. |
| `py/pinjamlab/io/validator.py:26-29` | `try: days = int(...)` / `except ValueError` | Mengubah input angka yang tidak valid menjadi `Result.failure(...)` | Tepat: kesalahan format adalah hasil yang diharapkan di boundary I/O, bukan exception bisnis. |
| `py/pinjamlab/app/runner.py:44-51` | `try: svc.borrow(...)` / `except LoanError as e` | Menambahkan pesan per baris ke daftar penolakan | Tepat: app dapat melaporkan masalah dan menetapkan exit code. Error lain tidak ditelan. |

### C. Ringkasan aliran penanganan

```text
CSV file
   |
   v
io: CsvReader -> Validator
   |                 |
   |                 +-- format salah -> Result.failure -> app mencatat row N
   v
service: LoanService
   |
   v
 domain: Equipment / Member
   |
   +-- UnknownAssetTag / UnknownMember / InvalidLoanDuration /
   |   EquipmentUnavailable / QuotaExceeded
   |       -> app/runner menangkap LoanError dan mencatat pesan
   |
   +-- kegagalan ketika mutasi berlangsung
           -> kompensasi lokal + TxnGuard rollback saat unwind
```

Arah dependensi yang terlihat pada implementasi aktual adalah `app -> service`, `app -> io`, `service -> domain`, dan `io -> domain`. `domain` tidak bergantung pada format CSV, file, atau console. Hal ini sesuai dependency rule pada modul.

### D. Handler yang perlu diperhatikan

1. **File input tidak dapat dibuka:** C++ `CsvReader` melempar `std::runtime_error`; Python `open()` melempar `OSError`/`FileNotFoundError`. Keduanya belum ditangani secara khusus di entry point. Perbaikan yang tepat adalah handler di `app` yang mencetak nama file dan penyebab, lalu mengembalikan exit code bukan nol.
2. **Catch-all pada service:** bukan swallowed error karena selalu diikuti `throw;` / `raise` setelah kompensasi.
3. **Catch-all pada cleanup:** dibenarkan karena destructor C++ `noexcept` dan rollback tidak boleh memunculkan exception kedua ketika unwinding. Pesan rollback gagal tetap perlu dicatat.
4. **Exception dari `Result.value()`:** adalah pelanggaran kontrak pemanggil. Caller harus mengecek `ok()` / `ok` dahulu; bukan tugas app untuk mengubah bug ini menjadi error input.

---

## 1.2 Tabel tiga mode kegagalan tambahan

Ketiga mode berikut belum memiliki guard khusus di starter. Semuanya cocok ditolak di `io/Validator`, karena berasal dari format/representasi request dan bisa diperiksa sebelum domain dipanggil. Untuk kasus-kasus ini dipilih `Result.failure`, bukan exception, sebab input cacat merupakan hasil yang wajar ditemukan ketika membaca CSV.

| No. | Mode kegagalan baru | Guard yang perlu ditambahkan | Klasifikasi | Lapisan yang menangani | Pesan persis yang ditargetkan |
|---:|---|---|---|---|---|
| 1 | Field identifier hanya berisi whitespace | Setelah pemeriksaan kosong, pangkas whitespace; jika hasilnya kosong, tolak `tag` atau `member_id` | `Result.failure`; input file dapat mengandung spasi dan bukan kondisi luar biasa | `io/Validator`; `app` menampilkan hasilnya | `tag must not be whitespace-only` atau `memberId must not be whitespace-only` |
| 2 | Format/case identifier tidak kanonis | Validasi tag dengan pola `LP-` diikuti tiga digit dan member dengan pola `M-` diikuti tiga digit; jangan diam-diam mengubah huruf kecil menjadi huruf besar | `Result.failure`; identifier salah format adalah kesalahan request pada boundary | `io/Validator`; `app` menampilkan hasilnya | `tag must use canonical format LP-NNN, got: lp-001` atau `memberId must use canonical format M-NNN, got: m-101` |
| 3 | Nilai `days` di luar rentang integer 32-bit bertanda | Periksa rentang `1..2147483647` sebelum narrowing/conversion ke `int`; jangan mengandalkan cast | `Result.failure`; representasi angka tidak dapat diproses dengan aman pada tipe request C++ | `io/Validator`; `app` menampilkan hasilnya | `days is outside supported integer range: 2147483648` |

**Alasan penempatan:** domain hanya seharusnya menerima request yang sudah well-formed. Jika string whitespace, identifier dengan format yang salah, atau angka di luar rentang lolos ke domain, domain menerima data yang sebenarnya dapat ditolak lebih awal. Pemanggil langsung (`app` melalui validator) mampu bertindak: melaporkan nomor baris dan memperbaiki CSV.

**Catatan batas desain:** contoh `duplicate loan ID` tidak dipilih pada Lab 1 ini karena skema CSV starter hanya mempunyai `tag,member_id,days` dan model `Loan` tidak menerima loan ID dari input. Menambahkan kasus tersebut memerlukan perubahan skema dan aturan domain yang lebih luas. Tiga mode di atas dapat diuji dengan skema CSV yang sudah ada.

---

## 1.3 Bring-Your-Own-Input

File `sample-data/my-bad-requests.csv` berisi lima baris data yang tidak terdapat pada file tetap `bad-requests.csv`. Setiap baris dirancang untuk satu guard tambahan pada Bagian 1.2.

| Baris | Isi | Mode yang diuji | Pesan target |
|---:|---|---|---|
| 1 | tag berisi tiga spasi | whitespace-only tag | `row 1: tag must not be whitespace-only` |
| 2 | member_id berisi tiga spasi | whitespace-only member ID | `row 2: memberId must not be whitespace-only` |
| 3 | tag menggunakan huruf kecil | case/format tag tidak kanonis | `row 3: tag must use canonical format LP-NNN, got: lp-001` |
| 4 | member_id menggunakan huruf kecil | case/format member tidak kanonis | `row 4: memberId must use canonical format M-NNN, got: m-101` |
| 5 | days melampaui integer 32-bit | integer overflow/range | `row 5: days is outside supported integer range: 2147483648` |

Berkas `sample-data/my-expected-output.txt` menyimpan pesan target secara berurutan. Saat ini starter belum memiliki tiga guard tambahan tersebut, sehingga output target ini baru dapat dicocokkan setelah validasi tambahan diimplementasikan pada tahap berikutnya. Jangan mengubah `bad-requests.csv` atau `expected-output.txt` bawaan dosen.

### Observasi aktual sebelum guard baru diimplementasikan

Berkas khusus tersebut juga dicoba pada kode starter tanpa modifikasi. Empat input pertama memang ditolak, tetapi hanya sebagai `UnknownAssetTag` / `UnknownMember`, bukan dengan alasan format yang lebih spesifik. Baris kelima memperlihatkan ketidaksamaan ant bahasa: pada uji lokal dengan C++ (g++ 14.2), pesan yang keluar adalah `days must be positive, got -2147483648`; pada Python 3.13, pesannya `Loan duration exceeds maximum: 2147483648 > 90`. Ini menunjukkan bahwa C++ melakukan narrowing ke `int` tanpa pemeriksaan rentang, sedangkan integer Python tidak mengalami overflow pada ukuran tersebut. Karena itu, `my-expected-output.txt` merupakan acceptance target untuk guard baru, bukan output aktual starter.

---

## 1.4 Refleksi Ariane 5 Flight 501

Menurut saya, satu perubahan rancangan yang paling mungkin menyelamatkan Ariane 5 Flight 501 adalah menonaktifkan fungsi *alignment* pada Inertial Reference System (SRI) segera setelah lepas landas. Fungsi tersebut hanya diperlukan sebelum peluncuran, tetapi tetap berjalan ketika roket sudah terbang. Akibatnya, perangkat lunak masih memproses nilai kecepatan horizontal yang berada di luar asumsi rentang Ariane 4. Konversi nilai tersebut memicu Operand Error dan kedua SRI akhirnya berhenti. Inquiry Board menyimpulkan bahwa hilangnya informasi guidance dan attitude menyebabkan kegagalan penerbangan; ESA juga mencatat bahwa fungsi alignment yang tidak lagi diperlukan tetap aktif setelah lift-off dan tidak terwakili secara memadai dalam simulasi.

Saya memilih perubahan ini dibanding sekadar memperbesar tipe integer karena memperbesar tipe hanya memperbaiki satu batas numerik yang diketahui. Nilai lain tetap dapat melampaui asumsi baru, sedangkan menonaktifkan fungsi yang sudah tidak dibutuhkan menghilangkan jalur eksekusi pemicu setelah fase alignment selesai. Alternatif kedua adalah mempertahankan unit cadangan dengan desain yang identik. Cara itu tidak cukup karena kedua unit mengalami kesalahan perangkat lunak sistematis yang sama; redundansi identik tidak melindungi sistem dari common-mode failure. Penanganan Operand Error yang lebih baik tetap penting, tetapi jika responsnya hanya mematikan unit, sistem tetap kehilangan seluruh informasi navigasi. Pilihan yang lebih aman adalah mendeteksi nilai di luar rentang, tidak menggunakan hasil konversi yang tidak valid, dan mempertahankan mode kendali aman yang telah dirancang.

Handler PinjamLab yang paling dekat untuk direfleksikan adalah `TxnGuard` yang menangkap kegagalan rollback saat destructor berjalan. Catch-all ini memang diperlukan agar destructor `noexcept` tidak melempar exception kedua. Namun, kegagalan rollback tetap harus dicatat dengan jelas dan tidak boleh dianggap sebagai transaksi yang berhasil. Jadi, pelajarannya bukan menghapus semua catch-all, melainkan memastikan kebijakan setelah error sesuai kemampuan lapisan dan tidak membuang informasi penting.

---

## Referensi

1. Laboratorium Pemrograman II, *Modul 3: Penanganan Eksepsi & Pemrograman Modular*, khususnya Kegiatan Praktikum Lab 1 (hlm. 36–37) dan Tugas Analisis (hlm. 38–39).
2. Lions, J. L. et al., *Ariane 5 Flight 501 Failure — Report by the Inquiry Board*, 19 July 1996.
3. European Space Agency (ESA), *Ariane 501 — Presentation of Inquiry Board report*, 23 July 1996, https://www.esa.int/media-advisories/news-media/media/media-advisories/n-33-1996-ariane-501-presentation-of-inquiry-board-report
