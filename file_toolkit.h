#ifndef FILE_TOOLKIT_H
#define FILE_TOOLKIT_H

void copy_file(const char *, const char *);
int truncate_file(const char *, off_t );
void create_sparse_file(const char *);
void create_temp_file();
void show_file_flags(const char *);
void set_umask_and_create_file(const char *, const char *);
void atomic_create_file(const char *);
void duplicate_fd(const char *);
void inspect_and_toggle_append(const char *);
void toggle_append_flag(int);
void inspect_fd_flags(int);
void demo_pread_pwrite(const char *);

//void print_usage

#endif
