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
        {"hole",     required_argument, 0 , 'h'},
        {"temp",     no_argument,       0 , 'x'},
        {"dup",      required_argument, 0 , 'd'},
        {"umask",    required_argument, 0,  'u'},
        {"atomic",   required_argument, 0,  'a'},
        {"flags",    required_argument, 0,  'f'},
        {"pread",    required_argument, 0,  'p'},
        {"help",      no_argument,      0,  '?'},
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
            
            case 'h':
                create_sparse_file(optarg);
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

            case 'f':
                inspect_and_toggle_append(optarg);
                break;
            
            case 'p':
                demo_pread_pwrite(optarg);
                break;

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
void create_sparse_file(const char *filename)
{
    return;

}
void create_temp_file()
{
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

    unlink(template);
    close(fd);
    return;

}
void show_file_flags(const char *filename)
{
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

void inspect_and_toggle_append(const char *filename) 
{
    int fd = open(filename, O_WRONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    printf("Before toggling:\n");
    inspect_fd_flags(fd);

    toggle_append_flag(fd);

    printf("After toggling:\n");
    inspect_fd_flags(fd);

    close(fd);
}

void inspect_fd_flags(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if(flags == -1)
    {
        perror("fcntl - get");
        return;
    }

    printf("Flags for fd %d:\n",fd);
    if(flags & O_APPEND)    printf(" -O_APPEND\n");
    if(flags & O_NONBLOCK)  printf(" -O_NONBLOCK\n");
    if(flags & O_SYNC)      printf(" -O_SYNC\n");
    if((flags & O_ACCMODE) == O_RDONLY) printf(" - O_RDONLY\n");
    if((flags & O_ACCMODE) == O_WRONLY) printf(" - O_WRONLY\n");
    if((flags & O_ACCMODE) == O_RDWR)   printf(" - O_RDWR\n");
}


void toggle_append_flag(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if(flags == -1)
    {
        perror("fnctl - get");
        return;
    }

    if(flags & O_APPEND)
    {
        printf("Removing O_APPEND\n");
        flags &= ~O_APPEND;
    }
    else
    {
        printf("Adding O_APPEND\n");
        flags |= O_APPEND;
    }

    if(fcntl(fd, F_SETFL, flags) == -1)
    {
        perror("fcntl -set");
    }
    
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
