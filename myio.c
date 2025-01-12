/*
 *myio.c
*/

#include "myio.h"
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


struct myFILE* myopen(char *file_name, char* mode){
    int flags;

    struct myFILE* file;

    if ((file = malloc(sizeof(struct myFILE))) == NULL){
        return NULL;
    }
    if (strcmp(mode, "r") == 0){
        flags = O_RDONLY;
    }
    else if (strcmp(mode, "w") == 0){
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    }
    else if (strcmp(mode, "a") == 0){
        flags = O_WRONLY | O_CREAT | O_APPEND;
    }
    else if (strcmp(mode, "r+") == 0){
        flags = O_RDWR;
    }
    else if (strcmp(mode, "w+") == 0){
        flags = O_RDWR | O_CREAT | O_TRUNC;
    }
    else if (strcmp(mode, "a+") == 0){
        flags = O_RDWR | O_CREAT | O_APPEND;
    }
    else{
        return NULL;
    }
    if ((file->fd = open(file_name, flags, 0666)) == -1){
        free(file);
        return NULL;
    }
    if ((file->myreadbuf = malloc(BUF_SIZE)) == NULL){
        close(file->fd);
        free(file);
        return NULL;
    }
    if ((file->mywritebuf = malloc(BUF_SIZE)) == NULL){
        close(file->fd);
        free(file->myreadbuf);
        free(file);
        return NULL;
    }
    file->read_buffer_start = -1;
    file->read_buffer_end = -1;
    file->write_buffer_start = -1;
    file->write_buffer_end = -1;
    file->read_last = false;
    file->user_offset = 0;

    return file;
}

size_t myread(void* buf, size_t size, struct myFILE* stream) {
    if(!stream->read_last){
        if(myseek(stream, stream->user_offset, SEEK_SET) == -1){
            return 0;
        }
    }

    int read_bytes = 0;
    stream->read_last = true;
    //read using the fd, myreadbuf, and read the amount of bytes in the buffer
    if(stream->read_buffer_start == -1 || stream->read_buffer_start == stream->read_buffer_end) {
        if ((stream->read_buffer_end = read(stream->fd, stream->myreadbuf, BUF_SIZE)) == -1){
            return read_bytes;
        } 
        stream->read_buffer_start = 0;
    }
 
 
    if(size <= (stream->read_buffer_end - stream->read_buffer_start)) {
        //If bytes wanted is less than the # of bytes left in the buffer.
        memcpy(buf, (char*)stream->myreadbuf + stream->read_buffer_start, size);
 
        stream->read_buffer_start += size;
        stream->user_offset += size;
        return size;
    }
    else if(size > stream->read_buffer_end - stream->read_buffer_start && stream->read_buffer_end != BUF_SIZE ) {
        //If the number of bytes wanted is greater than whats left
        // and the buffer isn't full -- meaning whole file has been read.
        memcpy(buf, (char*)stream->myreadbuf + stream->read_buffer_start, stream->read_buffer_end);
        stream->user_offset += stream->read_buffer_end - stream->read_buffer_start;
        return stream->read_buffer_end - stream->read_buffer_start;
    }
    
    else {
        //The Number of bytes wanted is greater than the amount in the 
        //buffer not because the file has been fully read.
        memcpy(buf, (char*)stream->myreadbuf + stream->read_buffer_start, stream->read_buffer_end);
        read_bytes = stream->read_buffer_end - stream->read_buffer_start;
        int current_bytes;
        if ((current_bytes = read(stream->fd, (char*)buf + (stream->read_buffer_end - stream->read_buffer_start), size - (read_bytes))) == -1){
            stream->user_offset += read_bytes;
            return read_bytes;
        }
        read_bytes += current_bytes;
        stream->read_buffer_start = -1;
        stream ->read_buffer_end = -1;

        stream->user_offset += read_bytes;
        return read_bytes;
    }
    
    return 0;
}

size_t mywrite(void* buf, size_t size, struct myFILE* stream){
    if(stream->read_last){
        if(myseek(stream, stream->user_offset, SEEK_SET) == -1){
            return 0;
        }
    }


    stream->read_last = false;
    //If there is nothing in the buffer and they ask for less than BUF_SIZE bytes, 
    //add the bytes to the buffer and don't call write
    if(stream->write_buffer_start == -1 && size < BUF_SIZE){
        memcpy((char*)stream->mywritebuf, buf, size);
        stream->write_buffer_start = 0; 
        stream->write_buffer_end = size;
    }
    //If there is nothing in the buffer and they ask for BUF_SIZE or more bytes
    else if(stream->write_buffer_start == -1 && size >= BUF_SIZE){
        if (write(stream->fd, buf, size) == -1){
            return 0;
        }
    }

    //If there is stuff in the buffer and they ask to write less bytes 
    //than how much is left, write to the buffer and don't call the system call
    else if(size < (BUF_SIZE - stream->write_buffer_end)){
        memcpy((char*)stream->mywritebuf + stream->write_buffer_end, buf, size);
        stream->write_buffer_end += size;
    }

    //If there is stuff in the buffer, and they ask to write more bytes than 
    //how much space is left in buffer, fill the buffer up, flush the buffer, 
    //then add the remaining to the buffer if less than BUF_SIZE bytes.  Else call write again.
    else if(stream->write_buffer_start != -1 && size > (BUF_SIZE - stream->write_buffer_end)){
        int chunk_size = BUF_SIZE - stream->write_buffer_end;
        memcpy((char*)stream->mywritebuf + stream->write_buffer_end, buf, chunk_size);
        stream->write_buffer_end += chunk_size;
        if ((write(stream->fd, stream->mywritebuf, stream->write_buffer_end - stream->write_buffer_start)) == -1){
            return 0;
        } 

        if (size - chunk_size >= BUF_SIZE){
            if ((write(stream->fd, (char*)buf + chunk_size, size - chunk_size)) == -1){
                return 0;
            }
            stream->write_buffer_start = -1;
            stream->write_buffer_end = -1;
        }
        else{
            memcpy((char*) stream->mywritebuf, (char*)buf + chunk_size, size - chunk_size);
            stream->write_buffer_start = 0;
            stream->write_buffer_end = size - chunk_size;
        } 
    }

    //If there is stuff in the buffer, and they ask to write exactly how many bytes are left in the buffer
    else if(stream->write_buffer_start != -1 && size == (BUF_SIZE - stream->write_buffer_end)){
        memcpy((char*)stream->mywritebuf + stream->write_buffer_end, buf, size);
        if ((write(stream->fd, stream->mywritebuf, stream->write_buffer_end - stream->write_buffer_start)) == -1){
            return 0;
        } 
        stream->write_buffer_start = -1;
        stream->write_buffer_end = -1;
    }

    stream->user_offset += size;
    return size;

}


int myclose(struct myFILE* stream){
    myflush(stream);
    free(stream->myreadbuf);
    free(stream->mywritebuf);

    int return_val = close(stream->fd);
    free(stream);
    return return_val;
}



int myflush(struct myFILE* stream){
    if (stream->write_buffer_start == -1){
        return 0;
    } 

    if (write(stream->fd, stream->mywritebuf, stream->write_buffer_end - stream->write_buffer_start) == -1){
        return -1;
    }

    return 0;
}


off_t myseek(struct myFILE* stream, off_t offset, int whence){
    if(whence == SEEK_SET){
        stream->read_buffer_start = -1;
        stream->read_buffer_end = -1;

        myflush(stream);
        stream->write_buffer_start = -1;
        stream->write_buffer_end = -1;

        return stream->user_offset = lseek(stream->fd, offset, SEEK_SET);
    }
    else if(whence == SEEK_CUR){
        if(offset > BUF_SIZE - stream->read_buffer_start){
            stream->read_buffer_start = -1;
            stream->read_buffer_end = -1;
        }
        else{
            stream->read_buffer_start += offset;
        }

        myflush(stream);
        stream->write_buffer_start = -1;
        stream->write_buffer_end = -1;

        return stream->user_offset = lseek(stream->fd, offset, SEEK_CUR);
    }

    return -1;
}




