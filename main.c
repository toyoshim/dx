#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include "i86intrf.h"
#include "process.h"
#include "memory.h"
#include "int.h"
#include "file.h"
#include "debug.h"

extern unsigned char *memory;
jmp_buf jmpdata;
int jmpcode = 0;

void
sig_int(int arg)
{
#ifdef _DEBUG
	debug_step_on();
#else /* !_DEBUG */
	exit(1);
#endif /* !_DEBUG */
}

int
main(int argc, char **argv, char **env)
{
	int i;
	process_info *pi;
	int vector_seg_size = 256 * 4 * 2 / 16;
	int vector_seg = 0;
	int env_seg_size;
	int env_seg;
	unsigned char cmd[128];
	int offset;
	char *p;

	const char *fname = file_search(argv[1]);
	if (NULL == fname) {
		char *exename = malloc(strlen(argv[1]) + 4 + 1);
		if (NULL != exename) {
			strcpy(exename, argv[1]);
			strcat(exename, ".exe");
			fname = file_search(exename);
			free(exename);
		}
		if (NULL == fname) {
			fprintf(stderr, "command not found: %s\n", argv[1]);
			exit(1);
		}
	}

	/* Ctl-C handling */
	signal(SIGINT, sig_int);

	/* calc mem size for env block */
	env_seg_size = 5 + strlen(fname);
	for (i = 0; env[i] != NULL; i++) {
		char *eq = strchr(env[i], '=');
		int key_len = (unsigned long)eq - (unsigned long)env[i];
		char *key = malloc(key_len + 4);
		if (NULL != key) {
			char *val;
			strcpy(key, "DX_");
			strncpy(&key[3], env[i], key_len);
			key[key_len + 3] = 0;
			val = getenv(key);
			if (NULL != val) {
				free(key);
				continue;
			} else {
				if (0 == strncmp(env[i], "DX_", 3)) {
					env_seg_size += strlen(env[i]) - 2;
				} else {
					env_seg_size += strlen(env[i]) + 1;
				}
			}
			free(key);
		}
	}
	env_seg_size += 16;
	env_seg_size /= 16;
	env_seg = vector_seg_size;
	if (0 > memory_alloc(vector_seg_size + env_seg_size)) {
		fprintf(stderr, "no memory\n");
		exit(1);
	}

	/* int vector */
	/* 0x0f: invalid op code = native hook */
	/* 0xcf: iret */
	/* intXX : 0x0f 0xXX 0xcf 0x00 */
	for (i = 0; i < 256; i++) {
		int to = (256 + i) * 4;
		*(unsigned short *)&memory[i * 4 + 0] = WORD(to);
		*(unsigned short *)&memory[i * 4 + 2] = WORD(0);
		memory[to + 0] = 0x0f;
		memory[to + 1] = i;
		memory[to + 2] = 0xcf;
	}

	/* cmd */
	offset = 0x01;
	for (i = 2; i < argc; i++) {
		if (offset == 0x7f) break;
		for (p = argv[i]; (*p != 0) && (offset != 0x7f); offset++) {
			cmd[offset] = *p++;
		}
		if (((i + 1) != argc) && (offset != 0x7f)) cmd[offset++] = ' ';
	}
	cmd[offset] = 0x0d;
	cmd[0x00] = offset - 1;

	/* env block */
	p = &memory[env_seg * 16];
	offset = 0;
	for (i = 0; env[i] != NULL; i++) {
		char *eq = strchr(env[i], '=');
		int key_len = (unsigned long)eq - (unsigned long)env[i];
		char *key = malloc(key_len + 4);
		if (NULL != key) {
			char *val;
			strcpy(key, "DX_");
			strncpy(&key[3], env[i], key_len);
			key[key_len + 3] = 0;
			val = getenv(key);
			if (NULL != val) {
				free(key);
				continue;
			} else {
				char *envline = &p[offset];
				char *envp = env[i];
				if (0 == strncmp(env[i], "DX_", 3)) envp += 3;
else continue; /* DEBUG */
				if (0 == strncmp(envp, "PATH=", 5)) {
					char *sep;
					for (sep = strchr(envp, ':'); NULL != sep; sep = strchr(&sep[1], ':')) *sep = ';';
					for (sep = strchr(envp, '/'); NULL != sep; sep = strchr(&sep[1], '/')) *sep = '\\';
				}
				offset += sprintf(&p[offset], "%s", envp) + 1;
#ifdef _DEBUG
				puts(envline);
#endif /* _DEBUG */
			}
			free(key);
		}
	}
	p[offset++] = 0;
	p[offset++] = 1;
	p[offset++] = 0;
	p[offset++] = 'A';
	p[offset++] = ':';
	for (i = 0; fname[i] != 0; i++) {
		if (fname[i] == '/') p[offset++] = '\\';
		else p[offset++] = fname[i];
	}
	p[offset] = 0;

	/* create process */
	pi = process_open(fname, env_seg, cmd);
	if (NULL == pi) {
		fprintf(stderr, "can not create process : %s\n", argv[1]);
		exit(2);
	}

	file_init(256);	/* FILES=256 */

	if (0 == setjmp(jmpdata)) process_resume(pi);

	return jmpcode;
}

