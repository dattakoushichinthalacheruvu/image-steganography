#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"
#include "common.h"

typedef struct
{
    char *stego_image_fname;
    FILE *fptr_stego_image;

    char *output_fname;
    char default_output_fname[100];
    FILE *fptr_output;

    char secret_extn[MAX_EXTENSION_SIZE];
    uint secret_size;
} DecodeInfo;

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);
Status open_decode_file(DecodeInfo *decInfo);
Status do_decoding(DecodeInfo *decInfo);

Status decode_byte_from_lsb(unsigned char *data, FILE *image);
Status decode_int_from_lsb(unsigned int *data, FILE *image);
Status decode_string_from_image(char *data, int size, FILE *image);
Status decode_file_data(DecodeInfo *decInfo);

#endif
