#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "file.h"

#define	ERR_INVALID_FUNC_NUM	-1
#define	ERR_FILE_NOT_FOUND	-2
#define	ERR_PATH_NOT_FOUND	-3
#define	ERR_TOO_MANY_OPEN_FILE	-4
#define	ERR_ACCESS_DENIED	-5
#define	ERR_INVALID_HANDLE	-6
#define	ERR_ACCESS_CODE_INVALID	-12
#define	ERR_INVALID_PASSWD	-86

typedef struct fd_info{
	int fd;
	int mode;
	int buf;
} fd_info;

static fd_info *fds = NULL;
static int last_fd = 0;
static int max_fd = 0;

#ifndef _DEBUG
#define	printf
#endif /* _DEBUG */

static int
find_new_fd(void)
{
	int i;
	for (i = last_fd + 1; i <= max_fd; i++) {
		if (fds[i].fd != -1) continue;
		last_fd = i - 1;
		return i;
	}
	for (i = 0; i <= last_fd; i++) {
		if (fds[i].fd != -1) continue;
		last_fd = i - 1;
		return i;
	}
	return -1;
}

static int
attr2mode(int attr)
{
/*------------------------------------
   7: shareable / pending deleted file
   6: unused
   5: archive
   4: directory
   3: volume label / execute-only
   2: system
   1: hidden
   0: read-only
------------------------------------*/
	int rc = S_IRUSR | S_IRGRP | S_IROTH;
	if (attr & 0x0001) return rc;
	return rc | S_IWUSR;
}

static int
attr2flag(int attr)
{
	int flag = O_EXCL;
	if (attr & 0x0080) flag = 0;
	if (attr & 0x0001) return flag | O_RDONLY;
	return flag | O_RDWR;
}

int
file_init(int num)
{
	int i;
	if (NULL != fds) free(fds);
	fds = malloc(sizeof(fd_info) * num);
	memset(fds, 0, sizeof(fd_info) * num);
	fds[0].fd = STDIN_FILENO;
	fds[0].mode = 0x5181;
	fds[0].buf = -1;
	fds[1].fd = STDOUT_FILENO;
	fds[1].mode = 0x5182;
	fds[1].buf = -1;
	fds[2].fd = STDERR_FILENO;
	fds[2].mode = 0x5180;
	fds[2].buf = -1;
	for (i = 3; i < num; i++) {
		fds[i].fd = -1;
		fds[i].mode = 0;
		fds[i].buf = -1;
	}
	last_fd = 2;
	max_fd = num - 1;
}

int
file_create(const char *fname, int attr)
{
	int rc;
	int mode = attr2mode(attr);
	int flag = attr2flag(attr);
	int fd = find_new_fd();
	if (fd < 0) return ERR_TOO_MANY_OPEN_FILE;
	rc = open(fname, flag | O_CREAT | O_TRUNC, mode);
	if (rc < 0) {
		switch (errno) {
		case EACCES:
		case EROFS:
			return ERR_ACCESS_DENIED;
		case ENOENT:
		case ENOTDIR:
			return ERR_PATH_NOT_FOUND;
		case EMFILE:
		case ENFILE:
			return ERR_TOO_MANY_OPEN_FILE;
		default:
			return ERR_INVALID_FUNC_NUM;
		}
	}
	fds[fd].fd = rc;
	fds[fd].mode = 0x0800; /* A: */
	fds[fd].buf = -1;
	last_fd = fd;
	return fd;
}

int
file_open(const char *_fname, int mode)
{
/*--------------------------------
   2-0: access mode
	000 read only
	001 write only
	010 read/write
   6-4: sharing mode (not impl.)
	000 compat mode
	001 deny all
	010 deny write
	011 deny read
	100 deny none
   7:   inheritance (not impl.)
	1 process internal only
	0 share with child process
--------------------------------*/

	int rc;
	int flags;
	int fd = find_new_fd();
	char fname[PATH_MAX];
	int i;
	if (fd < 0) return ERR_TOO_MANY_OPEN_FILE;
	switch (mode & 0x07) {
	case 0x00:
		flags = O_RDONLY;
		break;
	case 0x01:
		flags = O_WRONLY;
		break;
	case 0x02:
		flags = O_RDWR;
		break;
	default:
		return ERR_INVALID_FUNC_NUM;
	}
	if (_fname[1] == ':') _fname += 2;
	for (i = 0; i < PATH_MAX; i++) {
		if (_fname[i] == '\\') fname[i] = '/';
		else fname[i] = _fname[i];
		if (fname[i] == 0) break;
	}
	rc = open(fname, flags);
	if (rc < 0) {
		switch (errno) {
		case EISDIR:
			return ERR_FILE_NOT_FOUND;
		case EACCES:
		case EROFS:
			return ERR_ACCESS_DENIED;
		case ENOENT:
		case ENOTDIR:
			return ERR_PATH_NOT_FOUND;
		case EMFILE:
		case ENFILE:
			return ERR_TOO_MANY_OPEN_FILE;
		default:
			return ERR_INVALID_FUNC_NUM;
		}
	}
	fds[fd].fd = rc;
	fds[fd].mode = 0x0800; /* A: */
	fds[fd].buf = -1;
	last_fd = fd;
	return fd;
}

int
file_close(int fd)
{
	int rc;
	if (fds[fd].fd == -1) return ERR_INVALID_HANDLE;
	rc = close(fds[fd].fd);
	fds[fd].fd = -1;
	fds[fd].mode = 0;
	fds[fd].buf = -1;
	if (last_fd == fd) last_fd--;
	if (rc < 0) ERR_INVALID_HANDLE;
	return 0;
}

int
file_unlink(const char *fname)
{
	int rc;
#ifdef _DEBUG
	return 0;
#endif /* _DEBUG */
	rc = unlink(fname);
	if (rc < 0) {
		switch (errno) {
		case EPERM:
		case ENOTDIR:
			return ERR_PATH_NOT_FOUND;
		case ENOENT:
			return ERR_FILE_NOT_FOUND;
		default:
			return ERR_ACCESS_DENIED;
		}
	}
	return rc;
}

int
file_dup2(int srcfd, int dstfd)
{
	int rc;
	if (fds[srcfd].fd == -1) return ERR_INVALID_HANDLE;
	if (fds[dstfd].fd != -1) {
		rc = file_close(dstfd);
		if (rc < 0) return rc;
	}
	rc = dup(fds[srcfd].fd);
	if (rc < 0) {
		switch (errno) {
		case EBADF:
			return ERR_INVALID_HANDLE;
		case EMFILE:
			return ERR_TOO_MANY_OPEN_FILE;
		default:
			return ERR_INVALID_FUNC_NUM;
		}
	}
	fds[dstfd].fd = rc;
	fds[dstfd].mode = fds[srcfd].mode;	/* TODO */
	fds[dstfd].buf = 0;			/* TODO */
	return dstfd;
}

int
file_write(int fd, void *data, size_t size)
{
	ssize_t rc;
	if (fds[fd].fd == -1) {
		printf("invalid file descriptor: %d\n", fd);
		return ERR_INVALID_HANDLE;
	}
	rc = write(fds[fd].fd, data, size);
	if (rc < 0) {
		printf("file_write: failed(%d)\n", errno);
		switch (errno) {
		case EBADF:
		case EFAULT:
			return ERR_INVALID_HANDLE;
		default:
			return ERR_ACCESS_DENIED;
		}
	}
	return rc;
}

int
file_read(int fd, void *data, size_t size)
{
	ssize_t rc;
	if (fds[fd].fd == -1) {
		printf("invalid file descriptor: %d\n", fd);
		return ERR_INVALID_HANDLE;
	}
	if (fds[fd].buf >= 0) {
		rc = 0;
		if (size > 0) {
			((unsigned char *)data)[0] = fds[fd].buf;
			fds[fd].buf = -1;
			if (size > 1) rc = read(fds[fd].fd, data + 1, size - 1);
			else rc = 1;
		}
	} else {
		rc = read(fds[fd].fd, data, size);
	}
	if (rc < 0) {
		printf("file_read: failed(%d)\n", errno);
		switch (errno) {
		case EBADF:
		case EFAULT:
			return ERR_INVALID_HANDLE;
		default:
			return ERR_ACCESS_DENIED;
		}
	}
	return rc;
}

int
file_seek(int fd, int offset, int from)
{
	int rc;
	if (fds[fd].fd == -1) {
		printf("invalid file descriptor: %d\n", fd);
		return ERR_INVALID_HANDLE;
	}
#ifdef _DEBUG
	rc = lseek(fds[fd].fd, 0, SEEK_CUR);
	printf("lseek from: %d(%x)\n", rc, rc);
	printf("lseek diff: %d(%x)\n", offset, offset);
#endif /* _DEBUG */
	rc = lseek(fds[fd].fd, offset, from);
	if (rc < 0) {
		switch (errno) {
		case EBADF:
			return ERR_INVALID_HANDLE;
		default:
			return ERR_INVALID_FUNC_NUM;
		}
	} else {
		fds[fd].buf = -1;
	}
	return rc;
}

int
file_is_readable(int _fd)
{
	int old_flag;
	int fd = fds[_fd].fd;
	unsigned char dt;
	int size;
	if (fd < 0) return -1;
	if (fds[fd].buf >= 0) return fds[fd].buf;
	old_flag = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, old_flag | O_NONBLOCK);
	size = read(fd, &dt, 1);
	fcntl(fd, F_SETFL, old_flag);
	if (size <= 0) return -1;
	fds[fd].buf = dt;
	return dt;
}

int
file_get_devinfo(int fd)
{
/*--------------------------------------------------
	character device
	14: device driver can process IOCTL requests
	13: output until busy supported
	11: driver supports OPEN/CLOSE calls
	 8: ??? (set by MS-DOS 6.2x KEYB)
	 7: set (indicates device)
	 6: EOF on input
	 5: raw (binary) mode
	 4: device is special (ises INT 29)
	 3: clock device
	 2: NUL device
	 1: standard output
	 0: standard input
----------------------------------------------------
	disk file
	11: media not removable
	 7: clear (indicates file)
	 6: file has not been written
	5-0:drive number
--------------------------------------------------*/

	if (fds[fd].fd == -1) return ERR_INVALID_HANDLE;
	return fds[fd].mode;
}

int
file_set_devinfo(int fd, int info)
{
	if (fds[fd].fd == -1) return ERR_INVALID_HANDLE;
	fds[fd].mode = info;
	printf("WARNING: device information (%d) is set to %04x\n", fd, info);
	return 0;
}

int
file_attribute(const char *_fname)
{
/*------------------------------------
   7: shareable / pending deleted file
   6: unused
   5: archive
   4: directory
   3: volume label / execute-only
   2: system
   1: hidden
   0: read-only
--------------------------------------
drwx -> atrb conv. rule
    d -> directory
   !r -> hidden
   !w -> readonly
   !d -> archive
------------------------------------*/
	int rc;
	struct stat st;
	char fname[PATH_MAX];
	int i;
	for (i = 0; i < PATH_MAX; i++) {
		if (_fname[i] == '\\') fname[i] = '/';
		else fname[i] = _fname[i];
		if (fname[i] == 0) break;
	}
	rc = stat(fname, &st);
	if (rc < 0) {
		printf("file_attribute: failed\n");
		switch (errno) {
		case EBADF:
			return ERR_INVALID_HANDLE;
		case ENOENT:
		case ENOTDIR:
			return ERR_PATH_NOT_FOUND;
		case EACCES:
			return ERR_ACCESS_DENIED;
		default:
			return ERR_FILE_NOT_FOUND;
		}
	} else {
		if (S_ISDIR(st.st_mode)) {
			rc = 0x0010;
		} else {
			rc = 0x0020;
		}
		if (!(st.st_mode & S_IRUSR)) rc |= 0x0002;
		if (!(st.st_mode & S_IWUSR)) rc |= 0x0001;
	}
	return rc;
}

int
file_dos2native(int fd)
{
	return fds[fd].fd;
}

static char *file_search_result = NULL;

const char *
file_search(const char *name)
{
	DIR *dir;
	struct dirent *ent;
	char *path;
	char *apath;
	char *path_e;
	char *fullname;
	int plen;
	int flen;
	const char *env;
	char *p;

	if (name[1] == ':') name += 2;
#ifdef _DEBUG
	printf("file_search: %s\n", name);
#endif /* _DEBUG */
	flen = strlen(name);
	env = getenv("DX_PATH");
	if (NULL == env) {
		env = getenv("PATH");
		if (NULL == env) return NULL;
	}
	if (NULL != file_search_result) {
		free(file_search_result);
		file_search_result = NULL;
	}
	path = strdup(env);
	for (p = path; *p != 0; p++) if (*p == '\\') *p = '/';
	if (NULL == path) return NULL;
	for (apath = path;; apath = &path_e[1]) {
		path_e = strpbrk(apath, ":;");
		if (NULL != path_e) *path_e = 0;
#ifdef _DEBUG
		printf("searching in %s ... \n", apath);
#endif /* _DEBUG */
		plen = strlen(apath);
		if (apath[plen - 1] == '/') {
			apath[plen - 1] = 0;
			plen--;
		}
		dir = opendir(apath);
		if (NULL != dir) {
			while (NULL != (ent = readdir(dir))) {
#ifdef _DEBUG
				printf("check: '%s', '%s'\n", ent->d_name, name);
#endif /* _DEBUG */
				if (0 != strcasecmp(ent->d_name, name)) continue;
				closedir(dir);
				file_search_result = malloc(plen + 1 + flen + 1);
				if (NULL != file_search_result) {
					sprintf(file_search_result, "%s/%s",
						apath, name);
				}
				free(path);
				return file_search_result;
			}
			closedir(dir);
		}
		if (NULL == path_e) break;
	}
	free(path);
	return NULL;
}

int
fcb_parse(char **_pat, int opt, struct fcb *fcb)
{
	char *pat = *_pat;
	if (0 == (opt & 0x02)) fcb->drive_number = 0;
	if (0 == (opt & 0x04)) {
		int len = 0;
		while ((*pat != 0) && (*pat != '.') && (len < 8)) fcb->file_name[len++] = *pat++;
		while (len < 8) fcb->file_name[len++] = 0x20;
	} else {
		int len = 0;
		while ((*pat != 0) && (*pat != '.') && (len < 8)) pat++;
	}
	if (*pat == '.') pat++;
	if (0 == (opt & 0x08)) {
		int len = 0;
		while ((*pat != 0) && (len < 3)) fcb->file_ext[len++] = *pat++;
		while (len < 3) fcb->file_ext[len++] = 0x20;
	}
	*_pat = pat;
	return 0;
}
