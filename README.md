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

```
Or compile manually:

``` 
gcc -o out file_toolkit.c
```
## 🚀 Usage

```
./out [OPTIONS]

```
## Options

| Short Option | Long Option     | Description                                 |
|--------------|------------------|---------------------------------------------|
| `-c`         | `--copy`         | Copy a file (source and destination required) |
| `-t`         | `--truncate`     | Truncate a file to 0 bytes                   |
| `-x`         | `--temp`         | Create and write to a temporary file         |
| `-d`         | `--dup`          | Duplicate file descriptor                    |
| `-u`         | `--umask`        | Set umask and print old value                |
| `-a`         | `--atomic`       | Atomically create a file                     |
|              | `--check-flag`   | Check if a specific flag is set              |
|              | `--set-flag`     | Set a specific file flag                     |
|              | `--clear-flag`   | Clear a specific file flag                   |
| `-p`         | `--pread`        | Read file using `pread()`                    |
| `-l`         | `--lock-read`    | Apply `fcntl()` read/write lock             |
| `-?`         | `--help`         | Display help message                         |

## 🔧 Examples
```
# Copy a file
./out --copy file1.txt file2.txt

# Truncate a file
./out --truncate file.txt

# Create a temporary file
./out --temp

# Set a file flag
./out --set-flag file.txt O_APPEND

# Check if a flag is set
./out --check-flag file.txt O_APPEND

# Set umask to 022
./out --umask 022

# Lock file for writing
./out --lock-read file.txt

```


`