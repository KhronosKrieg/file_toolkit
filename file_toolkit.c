#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "file_toolkit.h"

#define BUF_SIZE 4096


int main(int argc, char *argv[])
{
    static struct option long_options[] = 
    {
        {"copy",     required_argument, 0,  'c'},
        {"truncate", required_argument, 0,  't'},
        {"temp",     no_argument,       0 , 'x'},
        {"dup",      required_argument, 0 , 'd'},
        {"umask",    required_argument, 0,  'u'},
        {"atomic",   required_argument, 0,  'a'},
        {"check-flag", required_argument, 0, 500},
        {"set-flag",   required_argument, 0, 501},
        {"clear-flag", required_argument, 0, 502},
        {"pread",    required_argument, 0,  'p'},
        {"help",      no_argument,      0,  '?'},
        {"lock-read",      required_argument,      0,  'l'},
        {0, 0, 0, 0}
    };


    int opt;
    int option_index = 0;

    if(argc < 2)
    {
        //print_usage(argv[0]);
        return 1;
    }


    while ((opt = getopt_long(argc, argv, "c:t:h:xf:u:a:?", long_options, &option_index))!= -1)
    {
        switch(opt)
        {
            case 'c':
                if (optind >= argc)
                {
                    fprintf(stderr, "Missing destination for copy\n");
                    return 1;
                }
                copy_file(optarg, argv[optind]);
                break;
            case 't':
                truncate_file(optarg, atol(argv[optind]));
                break;
            
            
            case 'x':
                create_temp_file();
                break;

            case 'u':
                set_umask_and_create_file(optarg,argv[optind]);
                break;

            case 'a':
                atomic_create_file(optarg);
                break;
            
            case 'd':
                duplicate_fd(optarg);
                break;

            case 500: // check flag
                check_flag(optarg, argv[optind]);  // optarg = filename, argv[optind] = flag name
                break;

            case 501: // set flag
                 modify_flag(optarg, argv[optind], 1);  // optarg = filename
                break;

            case 502: // clear flag
                 modify_flag(optarg, argv[optind], 0);  // optarg = filename
                break;
            
            case 'p':
                demo_pread_pwrite(optarg);
                break;
            
            case 'l':
                {
                    int lock_type;
                    if(strcmp(optarg, "r") == 0) 
                    {
                        lock_type = F_RDLCK;
                    } 
                    else if(strcmp(optarg, "w") == 0) 
                    {
                        lock_type = F_WRLCK;
                    } 
                    else 
                    {
                        fprintf(stderr, "Invalid lock type. Use 'r' for read or 'w' for write.\n");
                                            exit(EXIT_FAILURE);
                    }

                     // optind points to next argument — should be the filename
                    if (optind >= argc) 
                    {
                        fprintf(stderr, "Expected filename after --lock option.\n");
                             exit(EXIT_FAILURE);
                    }

                    lock_file(argv[optind], lock_type);
                    break;
                }
                

            case '?':
                //print_usage(argv[0]);
                return 0;

        }
    }

    return 0;
}

void copy_file(const char *src, const char *dst)
{
    int fd_src, fd_dst;
    char buffer[BUF_SIZE];
    ssize_t bytes_read, bytes_written;
    off_t offset = 0;

    fd_src = open(src, O_RDONLY);
    if(fd_src < 0)
    {
        perror("Error opening source file");
        return;
    }

    fd_dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd_dst < 0)
    {
        perror("Error opening/creating destinaion file");
        close(fd_src);
        return;
    }

    while ((bytes_read = pread(fd_src, buffer, BUF_SIZE, offset))> 0 )
    {
        bytes_written = pwrite(fd_dst, buffer, bytes_read, offset);
        if(bytes_written != bytes_read)
        {
            perror("Error writing to destination file");
            break;
        }
        offset += bytes_read;
    }

    if(bytes_read < 0) perror("Error reading source file");

    close(fd_src);
    close(fd_dst);
}

int truncate_file(const char *filename, off_t size)
{
    if (truncate(filename, size) == -1) {
        perror("truncate");
        return -1;
    }
    printf("File '%s' truncated to %ld bytes successfully.\n", filename, size);
    return 0;

}

void create_temp_file()
{
    struct stat sb;
    struct timeval start, end;
    char template[] = "/tmp/tempfileXXXXXX";
    int fd = mkstemp(template);

    if(fd == -1)
    {
        perror("mkstemp");
        return;
    }
    printf("Temporary file created: %s\n", template);

    const char *msg = "This is a temporary file.\n";
    write(fd,msg, strlen(msg));

    if (fstat(fd, &sb) == 0) 
    {
        printf("File size: %ld bytes\n", sb.st_size);
    }
    fchmod(fd, 0600); // Only owner can read/write

    unlink(template);
    close(fd);
    return;

}

void set_umask_and_create_file(const char *mask_str, const char *filename)
{
    mode_t new_mask = strtol(mask_str, NULL, 8);  // Convert from string (octal)
    mode_t old_mask = umask(new_mask);           // Set new umask, get old
    printf("Old umask was: %03o, new umask is: %03o\n", old_mask, new_mask);

    int fd = open(filename, O_CREAT | O_WRONLY, 0777);
    if (fd == -1) {
        perror("open");
        return;
    }

    dprintf(fd, "This file was created after setting umask %03o\n", new_mask);
    close(fd);

    printf("File '%s' created. Check its permissions with: ls -l %s\n", filename, filename);
}
void atomic_create_file(const char *filename)
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        perror("atomic open");
        return;
    }
    printf("Created file %s safely!\n", filename);
    write(fd, "Atomic write\n", 13);
    close(fd);
}

void duplicate_fd(const char *filename)
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd == -1)
    {
        perror("bad open");
        return;
    }

    if (dup2(fd, STDOUT_FILENO) == -1)
    {
        perror("dup2");
        close(fd);
        return;
    }
    printf("This goes into the file %s via STDOUT! \n", filename);
    close(fd);
}




void check_flag(const char *filename, const char *flag_str) 
{
    int fd = open(filename, O_RDWR);
    if (fd == -1) 
    {
        perror(filename);
        return;
    }

    int flag = get_flag_from_string(flag_str);
    if (flag == -1) return;

    int curr_flags = fcntl(fd, F_GETFL);
    if (curr_flags == -1) 
    {
        perror("fcntl");
        close(fd);
        return;
    }

    if (curr_flags & flag)
        printf("Flag %s is SET on %s\n", flag_str, filename);
    else
        printf("Flag %s is NOT set on %s\n", flag_str, filename);

    close(fd);
}

void modify_flag(const char *filename, const char *flag_str, int set_flag) 
{
    int fd = open(filename, O_RDWR);
    if (fd == -1) 
    {
        perror("open");
        return;
    }

    int flag = get_flag_from_string(flag_str);
    if (flag == -1) return;

    int curr_flags = fcntl(fd, F_GETFL);
    if (curr_flags == -1) 
    {
        perror("fcntl");
        close(fd);
        return;
    }

    int new_flags = set_flag ? (curr_flags | flag) : (curr_flags & ~flag);
    if (fcntl(fd, F_SETFL, new_flags) == -1) 
    {
        perror("fcntl - set");
    } else 
    {
        printf("Successfully %s flag %s on %s\n", 
               set_flag ? "SET" : "CLEARED", flag_str, filename);
    }

    int verify_flags = fcntl(fd, F_GETFL);
    if (verify_flags & flag) {
        printf("Verified: %s is set!\n",flag_str);
    } else {
    printf("%s is NOT set!\n",flag_str);
    }

    close(fd);
}

int get_flag_from_string(const char *flag_str) 
{
    if (strcmp(flag_str, "O_APPEND") == 0) return O_APPEND;
    if (strcmp(flag_str, "O_NONBLOCK") == 0) return O_NONBLOCK;
    if (strcmp(flag_str, "O_SYNC") == 0) return O_SYNC;
    if (strcmp(flag_str, "O_RDONLY") == 0) return O_RDONLY;
    if (strcmp(flag_str, "O_WRONLY") == 0) return O_WRONLY;
    if (strcmp(flag_str, "O_RDWR") == 0) return O_RDWR;
    fprintf(stderr, "Unknown flag: %s\n", flag_str);
    return -1;
}


void demo_pread_pwrite(const char *filename)
{
    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    if(fd == -1)
    {
        perror("open");
        return;
    }

    const char *msg = "Hello Random Access!";
    if(pwrite(fd, msg, strlen(msg), 10) == -1)
    {
        perror("pwrite");
        close(fd);
        return;
    }

    printf("Written at offset 10: %s\n",msg);

    char buf[64] = {0};
    if(pread(fd, buf, strlen(msg), 10) == -1)
    {
        perror("pread");
        close(fd);
        return;
    }
    printf("Read back from offset 10: %s\n",buf);

    close(fd);
}

void lock_file(const char *filename, int lock_type)
{
    int fd = open(filename, O_RDWR);
    if(fd == -1)
    {
        perror("open");
        return;
    }

    struct flock lock;
    lock.l_type =lock_type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    printf("Attempting to acquire %s lock...\n",
                lock_type == F_WRLCK ? "WRITE":"READ");

    if(fcntl(fd, F_SETLKW, &lock) == -1)
    {
        perror("fnctl - locking");
        close(fd);
        return;
    }

    printf("Lock acquired! Press Enter to release...\n");
    getchar();

    lock.l_type = F_UNLCK;
    if(fcntl(fd, F_SETLK, &lock) == -1)
    {
        perror("fnctl - unlocking");
    }
    else{
        printf("Lock released.\n");
    }
    close(fd);
}