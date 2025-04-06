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
    /*Define an array of struct option to store long options for command-line parsing*/
    static struct option long_options[] = 
    {
        {"copy",       required_argument, 0,  'c'},
        {"truncate",   required_argument, 0,  't'},
        {"temp",       no_argument,       0 , 'x'},
        {"dup",        required_argument, 0 , 'd'},
        {"umask",      required_argument, 0,  'u'},
        {"atomic",     required_argument, 0,  'a'},
        {"check-flag", required_argument, 0, 500},
        {"set-flag",   required_argument, 0, 501},
        {"clear-flag", required_argument, 0, 502},
        {"pread",      required_argument, 0,  'p'},
        {"help",       no_argument,       0,  '?'},
        {"lock-read",  required_argument, 0,  'l'},
        {0, 0, 0, 0}
    };


    int opt;
    int option_index = 0;

    if(argc < 2)
    {
        //print_usage(argv[0]);
        return 1;
    }

/* "c:t:h:xf:u:a:?": A string of short options (single characters) that the program accepts. The colon (:) after an option indicates that it requires an argument.*/
/* - long_options: An array of struct option that defines the long options (multi-character options) that the program accepts.
- &option_index: A pointer to an integer that stores the index of the current option in the long_options array.
- opt: The current option being processed.

The getopt_long function returns the current option being processed, or -1 if there are no more options to process.*/
    
    while ((opt = getopt_long(argc, argv, "c:t:xd:u:a:p:l:?", long_options, &option_index))!= -1)
    {
        switch(opt)
        {
            case 'c':
                if (optind >= argc) /*checks if a destination file is provided. optind is an index into the argv array, and argc is the total number of arguments. If optind is greater than or equal to argc, it means there are no more arguments, and therefore no destination file is provided.*/
                {
                    fprintf(stderr, "Missing destination for copy\n");
                    return 1;
                }
                //optarg is a pointer to the argument of an option. When an option requires an argument, optarg points to the argument string.
                //optarg is a pointer to the argument that follows an option in the command line.
                
                //argv[optind] is the next command-line argument after the option that was just processed
                //optind points to the next argument after the option that was just processed
                copy_file(optarg, argv[optind]); // optind is an index into the argv array
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
                print_usage(argv[0]);
                return 0;

        }
    }

    return 0;
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("File Toolkit Utility - Perform various file operations\n\n");

    printf("Options:\n");
    printf("  -c, --copy <source> <dest>         Copy a file\n");
    printf("  -t, --truncate <file>              Truncate file to 0 length\n");
    printf("  -x, --temp                         Create and write to a temporary file\n");
    printf("  -d, --dup <file>                   Duplicate file descriptor for the file\n");
    printf("  -u, --umask <value>                Set umask and show the old value\n");
    printf("  -a, --atomic <file>                Atomically create file if it doesn't exist\n");
    printf("      --check-flag <file> <flag>     Check if a flag (e.g., O_APPEND) is set\n");
    printf("      --set-flag <file> <flag>       Set a flag (e.g., O_APPEND)\n");
    printf("      --clear-flag <file> <flag>     Clear a flag (e.g., O_APPEND)\n");
    printf("  -p, --pread <file>                 Read file using pread()\n");
    printf("  -l, --lock-read <file>             Apply a read/write lock using fcntl()\n");
    printf("  -?, --help                         Show this help message\n\n");

    printf("Examples:\n");
    printf("  %s --copy file1.txt file2.txt\n", prog_name);
    printf("  %s --set-flag file.txt O_APPEND\n", prog_name);
    printf("  %s -u 022\n", prog_name);

    printf("\nAvailable flags: O_APPEND, O_NONBLOCK, O_SYNC\n");
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
/*  - pread is a system call that reads from a file descriptor at a specified offset.
    - fd_src is the file descriptor of the source file.
    - buffer is the buffer where the read data is stored.
    - BUF_SIZE is the size of the buffer.
    - offset is the offset in the file where the read operation starts.
    - The return value of pread is stored in bytes_read, which represents the number of bytes read.
    - The loop continues as long as bytes_read is greater than 0.
*/
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
    if (truncate(filename, size) == -1)  //size is the desired size of the file after truncation.
    {
        perror("truncate");
        return -1;
    }
    printf("File '%s' truncated to %ld bytes successfully.\n", filename, size);
    return 0;

}

void create_temp_file()
{
    struct stat sb;
    char template[] = "/tmp/tempfileXXXXXX";//XXXXXX in the string is a placeholder for a unique suffix that will be generated by mkstemp.
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
/* The umask function sets the file mode creation mask, which determines the permissions of newly created files.
- The new_mask value is used to clear bits in the file mode, i.e., if a bit is set in new_mask, the corresponding bit in the file mode will be cleared.
*/
    mode_t new_mask = strtol(mask_str, NULL, 8);  // Convert from string (octal)
    mode_t old_mask = umask(new_mask);           // Set new umask, get old
    /* umask is a function that sets the file mode creation mask.
    - new_mask is the new umask to be set.
    - The return value of umask is the previous umask, which is stored in the old_mask variable.
*/
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
    /* O_WRONLY is a flag that specifies the file should be opened in write-only mode.
    - O_CREAT is a flag that specifies the file should be created if it doesn't exist.
    - O_EXCL is a flag that ensures the file is created exclusively, i.e., if the file already exists, the open call will fail.
    - 0644 is the mode (permissions) to be set for the file if it is created. In this case, the permissions are:
        - 6 for the owner (read and write permissions)
        - 4 for the group (read permission)
        - 4 for others (read permission)

*/
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
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644); //This code opens a file in write-only mode, creating it if it doesn't exist, and truncating its contents if it does exist.
    if(fd == -1)
    {
        perror("bad open");
        return;
    }
/* - If the dup2 call is successful, the file descriptor fd is duplicated to STDOUT_FILENO.
- This means that any writes to STDOUT_FILENO (e.g., using printf) will be redirected to the file associated with fd.
*/
    if (dup2(fd, STDOUT_FILENO) == -1) //duplicates a file descriptor to redirect the standard output.
    {
        perror("dup2");
        close(fd);
        return;
    }
    printf("This goes into the file %s via STDOUT! \n", filename); /*Normally this should get into standard output but we duplicated our file descriptor to redirect standard output. so this goes to file*/
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

    int curr_flags = fcntl(fd, F_GETFL); //int curr_flags = fcntl(fd, F_GETFL);
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

/* int new_flags = set_flag ? (curr_flags | flag) : (curr_flags & ~flag);
  If set_flag is true (non-zero), the expression evaluates to curr_flags | flag.
    - If set_flag is false (zero), the expression evaluates to curr_flags & ~flag.
- If set_flag is true, the flag bit is set in the new_flags value.
- If set_flag is false, the flag bit is cleared in the new_flags value.
*/
    int new_flags = set_flag ? (curr_flags | flag) : (curr_flags & ~flag);
    if (fcntl(fd, F_SETFL, new_flags) == -1) //This code sets the new flags for a file descriptor.
    {
        perror("fcntl - set");
    } 
    else 
    {
        printf("Successfully %s flag %s on %s\n", 
               set_flag ? "SET" : "CLEARED", flag_str, filename);
    }

    int verify_flags = fcntl(fd, F_GETFL); //Just to verify flag is set or cleared
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
    lock.l_whence = SEEK_SET; /*This sets the starting point for the lock.
    - SEEK_SET means the lock starts from the beginning of the file.*/

    lock.l_start = 0; /*  0 means the lock starts from the beginning of the file.*/
    lock.l_len = 0; /*  0 means the lock applies to the entire file.*/
/* - lock_type can be one of the following:
        - F_RDLCK: Read lock
        - F_WRLCK: Write lock
        - F_UNLCK: Unlock
*/
/* lock_type == F_WRLCK ? "WRITE" : "READ":
    - This is a ternary expression that checks the value of lock_type.
    - If lock_type is equal to F_WRLCK, the expression evaluates to "WRITE".
    - If lock_type is not equal to F_WRLCK, the expression evaluates to "READ".
*/
    printf("Attempting to acquire %s lock...\n",
                lock_type == F_WRLCK ? "WRITE":"READ");


/*F_SETLKW is the command to set a lock on the file.
    - &lock is a pointer to a struct flock object that describes the lock to be acquired.
*/
    if(fcntl(fd, F_SETLKW, &lock) == -1) 
    {
        perror("fnctl - locking");
        close(fd);
        return;
    }

    printf("Lock acquired! Press Enter to release...\n");
    getchar();//This code reads a single character from the standard input.

    lock.l_type = F_UNLCK; //unlock
    if(fcntl(fd, F_SETLK, &lock) == -1) //F_SETLK is the command to set or release a lock.
    {
        perror("fnctl - unlocking");
    }
    else{
        printf("Lock released.\n");
    }
    close(fd);
}



/* The permissions are represented as a 3-digit octal number, where each digit represents the permissions for the owner, group, and others, respectively.
- Each digit can have a value between 0 and 7, where:
    - 0 means no permissions (---)
    - 1 means execute permission (--x)
    - 2 means write permission (-w-)
    - 3 means write and execute permissions (-wx)
    - 4 means read permission (r--)
    - 5 means read and execute permissions (r-x)
    - 6 means read and write permissions (rw-)
    - 7 means read, write, and execute permissions (rwx)*/