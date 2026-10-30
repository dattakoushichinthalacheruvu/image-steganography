#ifndef ENCODE_H
#define ENCODE_H

#include <stdio.h>
#include "types.h"
#include "common.h"

typedef struct
{
    char *src_image_fname;
    FILE *fptr_src_image;

    char *secret_fname;
    FILE *fptr_secret;
    char secret_extn[MAX_EXTENSION_SIZE];
    uint secret_size;

    char *stego_image_fname;
    FILE *fptr_stego_image;

    uint image_capacity;
} EncodeInfo;

OperationType check_operation_type(char *argv[]);
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo);
Status open_encode_files(EncodeInfo *encInfo);
Status check_capacity(EncodeInfo *encInfo);
Status do_encoding(EncodeInfo *encInfo);

uint get_image_capacity(FILE *fptr_image);
uint get_file_size(FILE *fptr_file);
Status copy_bmp_header(FILE *src, FILE *dest);
Status copy_remaining_data(FILE *src, FILE *dest);

Status encode_byte_to_lsb(unsigned char data, FILE *src, FILE *dest);
Status encode_int_to_lsb(unsigned int data, FILE *src, FILE *dest);
Status encode_string_to_image(const char *data, FILE *src, FILE *dest);
Status encode_file_data(EncodeInfo *encInfo);

#endif
