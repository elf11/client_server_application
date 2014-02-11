#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kermit.h"
#include "alarm.h"
#include "crc.h"

static word *tabel;
static struct settings me, you;

extern const unsigned char mark;

static void send_file_data(int fd_out, int fd_in, int file, unsigned char *ptr_seq);
static void set_hf_send_packet(int fd_out, int fd_in, unsigned char buffer[],
                unsigned char len, unsigned char *seq, unsigned char type,
                unsigned char response[], int *response_length);
static void send_packet(int fd_out, int fd2, unsigned char buffer[], int buffer_length,
            unsigned char response[], int *response_length);


void kermit_send(int fd_out, int fd_in, char *file_list[], int list_size) {
    unsigned char buffer[0x64], response[0x64];
    unsigned char len, seq;
    int i, file, response_length;
    char *ptr;

    tabel = tabelcrc(CRCCCITT);

    seq = 0;

    init_alarm();

    me.maxl = 79;
    me.time = 3;
    me.npad = 0;
    me.padc = 0x00;
    me.eol = '\r';
    me.qctl = '#';
    me.qbin = '&';
    me.chkt = '1';
    me.rept = '~';

    len = 9 + 4;
    set_settings(buffer, me);

    set_hf_send_packet(fd_out, fd_in, buffer, len, &seq, 'S', response, &response_length);

    get_settings(response, &you);

    for(i = 0; i < list_size; i++) {
        file = open(file_list[i], O_RDONLY);

        if(file == -1) {
            fprintf(stderr, "sender: the file %s could not be opened\n", file_list[i]);
            continue;
        }

        ptr = strrchr(file_list[i], '/');

        if(ptr == NULL) {
            len = strlen(file_list[i]);
            memcpy(buffer + 4, file_list[i], len);
        } else {
            len = strlen(ptr + 1);
            memcpy(buffer + 4, ptr + 1, len);
        }
        len += 4;
        set_hf_send_packet(fd_out, fd_in, buffer, len, &seq, 'F', response, &response_length);

        fprintf(stderr, "sender: sending file %s\n", file_list[i]);

        send_file_data(fd_out, fd_in, file, &seq);

        len = 4;
        set_hf_send_packet(fd_out, fd_in, buffer, len, &seq, 'Z', response, &response_length);

        close(file);
    }

    len = 4;
    set_hf_send_packet(fd_out, fd_in, buffer, len, &seq, 'B', response, &response_length);

    free(tabel);
}


static void send_file_data(int fd_out, int fd_in, int file, unsigned char *ptr_seq) {
    unsigned char buffer[0x64], response[0x64];
    unsigned char len, seq, times, coded[200];
    unsigned int current_character, previous_character;
    int i, k, previous_position, response_length;

    seq = *ptr_seq;

    k = previous_position = 0;

    current_character = 0;

    previous_character = MY_EOF;

    times = 0;

    getchar_with_buffer(FILE_BUFFER_RESET);
    do {
        current_character = getchar_with_buffer(file);

        if(current_character == previous_character && times < 94) {
            times++;
        } else {
            if(times <= DONT_COMPRESS_THRESHOLD) {
                for(i = 0; i < times; i++)
                    encode_single_char(me, coded, &k, previous_character);
            } else {
                coded[k++] = '~';
                coded[k++] = tochar(times);
                encode_single_char(me, coded, &k, previous_character);
            }

            previous_character = current_character;
            times = 1;
        }

        if(k > me.maxl - 4) {
            len = previous_position + 4;
            for(i = 0; i < previous_position; i++)
                buffer[i+4] = coded[i];

            set_hf_send_packet(fd_out, fd_in, buffer, len, &seq, 'D', response, &response_length);

            k -= previous_position;
            for(i = 0; i < k; i++)
                coded[i] = coded[i + previous_position];
        }

        previous_position = k;
    } while(current_character != (unsigned int) MY_EOF);

    if(k) {
        len = k + 4;
        for(i = 0; i < k; i++)
            buffer[i+4] = coded[i];

        set_hf_send_packet(fd_out, fd_in, buffer, len, &seq, 'D', response, &response_length);
    }

    *ptr_seq = seq;
}


static void set_hf_send_packet(int fd_out, int fd_in, unsigned char buffer[], unsigned char len, unsigned char *seq, unsigned char type, unsigned char response[], int *response_length) {
    set_packet_header(buffer, len, *seq, type);

    crc_packet(tabel, buffer, len);

    send_packet(fd_out, fd_in, buffer, len + 2, response, response_length);

    increment_counter(seq);
}


static void send_packet(int fd_out, int fd2, unsigned char buffer[], int buffer_length, unsigned char response[], int *response_length) {
    int i, bytes_read, bytes_written;
    unsigned char len, seq;
    const int *was_timeout;

    seq = unchar(buffer[2]);

    for(i = 0; i < MAX_RETRIES; i++) {
        bytes_written = write(fd_out, buffer, buffer_length);

        if(bytes_written != buffer_length) {
            fprintf(stderr, "sender: error writing packet to pipe (?!?)\n");
            continue;
        }

        was_timeout = set_alarm(TIMEOUT);

        bytes_read = read(fd2, response, 0x64);

        cancel_alarm();

        if(*was_timeout) {
            fprintf(stderr, "sender: we timed out while waiting for the ACK to the packet %d\n", seq);
            continue;
        }

        len = unchar(response[1]);

        if(!crc_check(tabel, response, len)) {
            fprintf(stderr, "sender: failed CRC check for the ACK/NAK to the packet %d\n", seq);
            continue;
        }

        if ( ( response[3] == 'Y' && buffer[2] == response[2] ) ||
             ( response[3] == 'N' && buffer[2] + 1 == response[2] ) ) {
            *response_length = bytes_read;
            return;
        }
    }

    fprintf(stderr, "sender: exceeded maximum number of retries... exiting...\n");

    close(fd_out);
    close(fd2);
    exit(0);
}

