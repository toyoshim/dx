#ifndef __DOS_H__
#define __DOS_H__

struct exehdr
{
	unsigned short magic;
	unsigned short bytes_on_last_page;
	unsigned short size;
	unsigned short nreloc;
	unsigned short hdr_size;
	unsigned short min_memory;
	unsigned short max_memory;
	unsigned short init_ss;
	unsigned short init_sp;
	unsigned short checksum;
	unsigned short init_ip;
	unsigned short init_cs;
	unsigned short reloc_offset;
	unsigned short overlay_num;
};

struct reloc_entry
{
	unsigned short off;
	unsigned short seg;
};

struct fcb
{
	unsigned char drive_number			__attribute__ ((packed));
	unsigned char file_name[8]			__attribute__ ((packed));
	unsigned char file_ext[3]			__attribute__ ((packed));
	unsigned short current_block_number		__attribute__ ((packed));
	unsigned short logical_record_size		__attribute__ ((packed));
	unsigned long file_size				__attribute__ ((packed));
	unsigned short date_of_last_write		__attribute__ ((packed));
	unsigned short time_of_last_write		__attribute__ ((packed));
	unsigned char reserved[8]			__attribute__ ((packed));
	unsigned char record_within_current_block	__attribute__ ((packed));
	unsigned long random_access_record_number	__attribute__ ((packed));
};

struct exec_pb
{
	unsigned short env_seg				__attribute__ ((packed));
	unsigned short command_offset			__attribute__ ((packed));
	unsigned short command_seg			__attribute__ ((packed));
	unsigned short first_fcb_offset			__attribute__ ((packed));
	unsigned short first_fcb_seg			__attribute__ ((packed));
	unsigned short second_fcb_offset		__attribute__ ((packed));
	unsigned short second_fcb_seg			__attribute__ ((packed));
};

#endif /* __DOS_H__ */

