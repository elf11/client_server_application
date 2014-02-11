#ifndef KERMIT_H_
#define KERMIT_H_

#include "crc.h"

#define MAX_RETRIES 5
#define TIMEOUT 3

#define FILE_BUFFER_LENGTH 825
#define FILE_BUFFER_RESET -1
#define MY_EOF ~0

#define DONT_COMPRESS_THRESHOLD 2

struct settings {
    unsigned char maxl, time, npad, padc, eol;
    unsigned char qctl, qbin, chkt, rept;
};

void kermit_send(int fd1, int fd2, char *file_list[], int list_size);
char* kermit_receive(int fd1, int fd2, char *down_dir);

unsigned char tochar(unsigned char x);
unsigned char unchar(unsigned char x);
unsigned char ctl(unsigned char x);
void increment_counter(unsigned char *seq);

void checksum(unsigned char *buffer, unsigned char length);
word compute_crc(word *tabel, unsigned char buffer[], unsigned char length);
void crc_packet(word *tabel, unsigned char buffer[], unsigned char length);
int crc_check(word *tabel, unsigned char buffer[], unsigned char length);

unsigned int getchar_with_buffer(int file);

void encode_single_char(struct settings kerm, unsigned char buffer[], int *position, unsigned char current);
void decode(struct settings kerm, unsigned char input[], int input_length, unsigned char output[], int *output_length);

void set_packet_header(unsigned char buffer[], unsigned char len, unsigned char seq, unsigned char type);

void set_settings(unsigned char buffer[], struct settings kerm);
void get_settings(unsigned char buffer[], struct settings * kerm);

#endif /*KERMIT_H_*/
