#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "kermit.h"
#include "crc.h"

// Constanta SOH
const unsigned char mark = 0x01;


unsigned char tochar(unsigned char x) {
    return x + 32;
}

unsigned char unchar(unsigned char x) {
    return x - 32;
}

unsigned char ctl(unsigned char x) {
    return x ^ 64;
}

void increment_counter(unsigned char *seq) {
    if(*seq < 0x3f)
        (*seq)++;
    else
        *seq = 0;
}


void checksum(unsigned char buffer[], unsigned char length) {
    unsigned char s = 0, i;

    for(i = 1; i <= length; i++)
        s += buffer[i];

    s += ( s & 0xc0 ) >> 6;

    buffer [ length + 1 ] = s & 0x3f;
}

word compute_crc(word *tabel, unsigned char buffer[], unsigned char length) {
    unsigned char i;
    word acum;

    acum = 0;

    for(i = 0; i < length; i++)
        crctabel(buffer[i], &acum, tabel);

    return acum;
}

void crc_packet(word *tabel, unsigned char buffer[], unsigned char length) {
    unsigned char upper_byte, lower_byte;
    word acum;

    acum = compute_crc(tabel, buffer + 1, length - 1);

    upper_byte = (acum & 0xff00) >> 8;
    lower_byte = acum && 0xff;

    buffer [ length ] = upper_byte;
    buffer [ length + 1 ] = lower_byte;
}

int crc_check(word *tabel, unsigned char buffer[], unsigned char length) {
    unsigned char upper_byte, lower_byte;
    word acum;

    acum = compute_crc(tabel, buffer + 1, length - 1);

    upper_byte = (acum & 0xff00) >> 8;
    lower_byte = acum && 0xff;

    return ( upper_byte == buffer [ length ] ) && ( lower_byte == buffer [ length + 1 ] );
}


unsigned int getchar_with_buffer(int file) {
    static unsigned char file_buffer[FILE_BUFFER_LENGTH];
    static int bytes_read, bytes_processed;

    if(file == FILE_BUFFER_RESET) {
        bytes_read = bytes_processed = 0;
        return MY_EOF;
    }

    if(bytes_processed == bytes_read) {
        bytes_read = read(file, file_buffer, FILE_BUFFER_LENGTH);

        if(bytes_read < 0)
            return MY_EOF;

        if(bytes_read)
            bytes_processed = 0; 
        else
            return MY_EOF; 
    }

    return file_buffer[bytes_processed++];
}

void encode_single_char(struct settings kerm, unsigned char buffer[], int *position, unsigned char current_character) {
    int k;

    k = *position;

    if(current_character & 0x80) {
        buffer[k++] = kerm.qbin;
        current_character &= ~ 0x80;
    }

    if( ( current_character < 32 ) || ( current_character == 0xff ) ) {
        buffer[k++] = kerm.qctl;
        current_character = ctl(current_character); 
    } else if( ( current_character == kerm.qctl ) || ( current_character == kerm.qbin ) || ( current_character == kerm.rept ) ) {
        buffer[k++] = kerm.qctl;
    }

    buffer[k++] = current_character;

    *position = k;
}

void decode(struct settings kerm, unsigned char input[], int input_length, unsigned char output[], int *output_length) {
    unsigned char times, current_character;
    int i, j, k;

    k = 0;

    for(i = 0; i < input_length; i++) {
        if(input[i] == kerm.rept) {
            i++;
            times = unchar(input[i]);
            ++i;
        } else {
            times = 1;
        }

        if(input[i] == kerm.qbin) {
            i++;
            current_character = 0x80;
        } else {
            current_character = 0;
        }

        if(input[i] == kerm.qctl) {
            i++;
            if( ( input[i] == kerm.qctl ) || ( input[i] == kerm.qbin ) || ( input[i] == kerm.rept ) )
                current_character |= input[i];
            else
                current_character |= ctl(input[i]);
        } else {
            current_character |= input[i];
        }

        for(j = 0; j < times; j++)
            output[k++] = current_character;
    }

    *output_length = k;
}


void set_packet_header(unsigned char buffer[], unsigned char len, unsigned char seq, unsigned char type) {
    buffer[0] = mark;
    buffer[1] = tochar(len);
    buffer[2] = tochar(seq);
    buffer[3] = type;
}

void set_settings(unsigned char buffer[], struct settings kerm) {
    buffer[4] = tochar(kerm.maxl);
    buffer[5] = tochar(kerm.time);
    buffer[6] = tochar(kerm.npad);
    buffer[7] = ctl(kerm.padc);
    buffer[8] = tochar(kerm.eol);
    buffer[9] = kerm.qctl;
    buffer[10] = kerm.qbin;
    buffer[11] = kerm.chkt;
    buffer[12] = kerm.rept;
}

void get_settings(unsigned char buffer[], struct settings * kerm) {
    kerm->maxl = unchar(buffer[4]);
    kerm->time = unchar(buffer[5]);
    kerm->npad = unchar(buffer[6]);
    kerm->padc = ctl(buffer[7]);
    kerm->eol = unchar(buffer[8]);
    kerm->qctl = buffer[9];
    kerm->qbin = buffer[10];
    kerm->chkt = buffer[11];
    kerm->rept = buffer[12];
}

