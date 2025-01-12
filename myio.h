/*
 *myio.h
*/

#include <stdbool.h>
#include <stdio.h>

#define BUF_SIZE 1000

struct myFILE {
   void* myreadbuf;
   void* mywritebuf;
   int fd;
   int read_buffer_start;
   int read_buffer_end;
   int write_buffer_start;
   int write_buffer_end;
   bool read_last;
   int user_offset;
};


struct myFILE* myopen(char *file_name, char* mode);
size_t myread(void* buf, size_t size, struct myFILE* stream);
size_t mywrite(void* buf, size_t size, struct myFILE* stream);
int myclose(struct myFILE* stream);
int myflush(struct myFILE* stream);
off_t myseek(struct myFILE* stream, off_t offset, int whence);



