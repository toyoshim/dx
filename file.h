#ifndef __FILE_H__
#define __FILE_H__

#ifndef __DOS_H__
#include "dos.h"
#endif /* __DOS_H__ */

int file_init(int max);
int file_create(const char *fname, int attr);
int file_open(const char *fname, int mode);
int file_close(int fd);
int file_unlink(const char *fname);
int file_dup2(int srcfd, int dstfd);
int file_write(int fd, void *data, size_t size);
int file_read(int fd, void *data, size_t size);
int file_seek(int fd, int offset, int from);
int file_get_devinfo(int fd);
int file_set_devinfo(int fd, int info);
int file_attribute(const char *fname);

const char *file_search(const char *name);

int fcb_parse(char **pat, int opt, struct fcb *fcb);

#endif /* __FILE_H__ */

