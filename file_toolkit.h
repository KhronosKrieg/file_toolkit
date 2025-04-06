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
void demo_pread_pwrite(const char *);
void lock_file(const char *, int);
int get_flag_from_string(const char *); 
void check_flag(const char *, const char *); 
void modify_flag(const char *, const char *, int ); 
void print_usage(const char *);

#endif
