#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "kermit.h"
#include "alarm.h"
#include "crc.h"


static word *tabel;
static struct settings me, you;

extern const unsigned char mark;


static void receive_packet(int fd_out, int fd_in, unsigned char received[], int *received_length, int seq);
static void send_confirmation(int fd_out, unsigned char seq, unsigned char type);


char* kermit_receive(int fd_out, int fd_in, char *down_dir) {
    char filename[0x74];
    unsigned char len, seq, data_length;
    unsigned char received[0x64], buffer[0x64], bytes[5000];
    int k, bytes_read, received_length, filename_length;
    int file;
    char *ret_name = NULL;

    tabel = tabelcrc(CRCCCITT);

    seq = 0;

    bytes_read = read(fd_in, received, 0x64);
    get_settings(received, &you);

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

    set_packet_header(buffer, len, seq, 'Y');
    crc_packet(tabel, buffer, len);

    if(write(fd_out, buffer, len + 2) != len + 2) {
        fprintf(stderr, "receiver: error writing packet to pipe (?!?)\n");
    }

    increment_counter(&seq);

    file = -1;

    while(1) {
        receive_packet(fd_out, fd_in, received, &received_length, seq);

        switch(received[3]) {
            case 'F':
                data_length = unchar(received[1]) - 4;

                strcpy(filename, down_dir);
                strcat(filename, "/");
                filename_length = strlen(filename);
                memcpy(filename + filename_length, received + 4, data_length);
                filename[filename_length + data_length] = 0;

                ret_name = (char*) malloc(0x74);
                memcpy(ret_name, received + 4, data_length);
                ret_name[data_length] = 0;

                file = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
                if(file == -1) {
                    fprintf(stderr, "receiver: file could not be opened for writing\n");
                    fprintf(stderr, "receiver: exiting...\n");
                    close(fd_out);
                    close(fd_in);
                    exit(-1);
                }

                fprintf(stderr, "receiver: receiving file %s\n", filename);
                break;

            case 'D':
                data_length = unchar(received[1]) - 4;

                decode(you, received + 4, data_length, bytes, &k);

                if(file != -1) {
                    if(write(file, bytes, k) != k) {
                        fprintf(stderr, "receiver: error writing in file\n");
                        exit(-1);
                    }
                } else {
                    fprintf(stderr, "receiver: a file should have been opened first\n");
                    fprintf(stderr, "receiver: exiting...\n");
                    close(fd_out);
                    close(fd_in);
                    exit(-1);
                }
                break;

            case 'Z':
                if(file != -1) {
                    close(file);
                    file = -1;
                } else {
                    fprintf(stderr, "receiver: a file should have been opened first\n");
                    fprintf(stderr, "receiver: exiting...\n");
                    close(fd_out);
                    close(fd_in);
                    exit(-1);
                }
                break;

            case 'B':
                free(tabel);
                return ret_name;
        }

        increment_counter(&seq);
    }

    return NULL;
}

static void receive_packet(int fd_out, int fd_in, unsigned char received[], int *received_length, int seq) {
    unsigned char len;
    int i, bytes_read;
    const int *was_timeout;

    for(i = 0; i < MAX_RETRIES; i++) {
        was_timeout = set_alarm(TIMEOUT);

        bytes_read = read(fd_in, received, 0x64);

        cancel_alarm();
        if(*was_timeout) {
            fprintf(stderr, "receiver: we timed out while waiting for the packet %d\n", seq);
            continue;
        }

        len = unchar(received[1]);

        if(crc_check(tabel, received, len) && unchar(received[2]) == seq) {
            send_confirmation(fd_out, seq, 'Y');

            *received_length = bytes_read;

            return;
        } else {
            send_confirmation(fd_out, seq, 'N');
        }
    }

    fprintf(stderr, "receiver: exceeded maximum number of retries... exiting...\n");

    close(fd_out);
    close(fd_in);

    exit(0);
}


static void send_confirmation(int fd_out, unsigned char seq, unsigned char type) {
    unsigned char len, buffer[0x64];
    int bytes_written;

    len = 4;
    set_packet_header(buffer, len, seq, type);
    crc_packet(tabel, buffer, len);

    bytes_written = write(fd_out, buffer, len + 2);

    if(bytes_written != len + 2) {
        fprintf(stderr, "receiver: error writing packet to pipe (?!?)\n");
    }
}

