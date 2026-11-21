# Cara menerapkan ZIP Lab 3 ke working copy

> Penting: Modul meminta `PREDICTION.md` ditulis dan di-commit sebelum perubahan
> implementasi Lab 3.1. Ikuti dua tahap di bawah agar urutannya terlihat pada Git.

## Tahap A — commit prediksi terlebih dahulu

1. Ekstrak ZIP ke folder sementara, bukan langsung menimpa working copy.
2. Dari folder hasil ekstrak, salin hanya `Module3/PREDICTION.md` ke root proyek
   `Modul3` yang sedang berada pada branch `m3-work`.
3. Di PowerShell pada root proyek, jalankan:

   ```powershell
   git branch --show-current
   git add PREDICTION.md
   git commit -m "docs: record Lab 3.1 prediction before refactor"
   ```

4. Pastikan commit tersebut hanya berisi `PREDICTION.md`:

   ```powershell
   git show --stat --oneline HEAD
   ```

## Tahap B — replace kode dan dokumen Lab 3

1. Salin seluruh isi folder `Module3` dari ZIP ke root proyek lama.
2. Pilih **Replace** untuk file yang sama.
3. Jangan hapus atau mengganti folder tersembunyi `.git` milik working copy.
4. Jangan menyalin folder build `bin/`, `.pytest_cache/`, atau `__pycache__/` jika
   folder tersebut terbentuk setelah pengujian sebelumnya.
5. Pastikan `PREDICTION.md` tetap ada di root proyek.

## Verifikasi dan bukti diff

Jalankan:

```powershell
& "C:\Program Files\Git\bin\bash.exe" ./run_tests.sh
```

Jika semua test lulus, ambil perubahan implementasi setelah commit prediksi:

```powershell
git add -N .
git diff --stat
git status
```

Salin output `git diff --stat` ke bagian yang disediakan di `LAB3_REPORT.md`.

Commit perubahan kode dan artefak Lab 3 setelah test berhasil:

```powershell
git add .
git commit -m "feat: complete Module 3 Lab 3 refactor and reproducibility"
```

Jangan memasukkan folder `bin/` atau cache Python ke commit. `.gitignore` proyek
sudah mengabaikan folder build dan cache umum.
