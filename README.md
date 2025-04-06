# 🛠️ File Toolkit

A command-line utility written in C for performing various file operations like copying, truncating, working with flags, temporary files, umask settings, and more.

## ✅ Features

- Copy files
- Truncate files
- Create and write to temporary files
- Set and view file descriptor flags (`O_APPEND`, `O_NONBLOCK`, etc.)
- Atomically create files
- Duplicate file descriptors
- Lock files using `fcntl()`
- Set `umask` and display previous values
- Read files using `pread`
- Check, set, and clear file flags dynamically

---

## 🧰 Build Instructions

Make sure you have `gcc` installed.

```bash
make



