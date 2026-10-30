#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "encode.h"

static void close_encode_files(EncodeInfo *encInfo)
{
    if (encInfo->fptr_src_image != NULL)
        fclose(encInfo->fptr_src_image);
    if (encInfo->fptr_secret != NULL)
        fclose(encInfo->fptr_secret);
    if (encInfo->fptr_stego_image != NULL)
        fclose(encInfo->fptr_stego_image);

    encInfo->fptr_src_image = NULL;
    encInfo->fptr_secret = NULL;
    encInfo->fptr_stego_image = NULL;
}

OperationType check_operation_type(char *argv[])
{
    if (strcmp(argv[1], "-e") == 0)
        return e_encode;

    if (strcmp(argv[1], "-d") == 0)
        return e_decode;

    return e_unsupported;
}

static int get_extension(const char *filename, char *extension)
{
    const char *dot = strrchr(filename, '.');
    size_t length;

    if (dot == NULL || dot == filename || dot[1] == '\0')
        return 0;

    length = strlen(dot);
    if (length >= MAX_EXTENSION_SIZE)
        return 0;

    strcpy(extension, dot);
    return 1;
}

Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if (argv[2] == NULL || argv[3] == NULL)
        return e_failure;

    if (strstr(argv[2], ".bmp") == NULL)
        return e_failure;

    if (!get_extension(argv[3], encInfo->secret_extn))
        return e_failure;

    encInfo->src_image_fname = argv[2];
    encInfo->secret_fname = argv[3];

    if (argv[4] != NULL)
        encInfo->stego_image_fname = argv[4];
    else
        encInfo->stego_image_fname = "stego.bmp";

    return e_success;
}

Status open_encode_files(EncodeInfo *encInfo)
{
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "rb");
    if (encInfo->fptr_src_image == NULL)
    {
        printf("ERROR: Cannot open source image: %s\n", encInfo->src_image_fname);
        return e_failure;
    }

    encInfo->fptr_secret = fopen(encInfo->secret_fname, "rb");
    if (encInfo->fptr_secret == NULL)
    {
        printf("ERROR: Cannot open secret file: %s\n", encInfo->secret_fname);
        close_encode_files(encInfo);
        return e_failure;
    }

    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb");
    if (encInfo->fptr_stego_image == NULL)
    {
        printf("ERROR: Cannot create output image: %s\n", encInfo->stego_image_fname);
        close_encode_files(encInfo);
        return e_failure;
    }

    return e_success;
}

uint get_image_capacity(FILE *fptr_image)
{
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned short bits_per_pixel = 0;

    if (fseek(fptr_image, 18, SEEK_SET) != 0)
        return 0;

    if (fread(&width, sizeof(width), 1, fptr_image) != 1)
        return 0;

    if (fread(&height, sizeof(height), 1, fptr_image) != 1)
        return 0;

    if (fseek(fptr_image, 28, SEEK_SET) != 0)
        return 0;

    if (fread(&bits_per_pixel, sizeof(bits_per_pixel), 1, fptr_image) != 1)
        return 0;

    if (bits_per_pixel != 24)
    {
        printf("ERROR: Only 24-bit BMP images are supported.\n");
        return 0;
    }

    rewind(fptr_image);
    return width * height * 3;
}

uint get_file_size(FILE *fptr_file)
{
    long size;

    if (fseek(fptr_file, 0, SEEK_END) != 0)
        return 0;

    size = ftell(fptr_file);
    rewind(fptr_file);

    if (size < 0)
        return 0;

    return (uint)size;
}

Status check_capacity(EncodeInfo *encInfo)
{
    unsigned long required_bytes;

    encInfo->image_capacity = get_image_capacity(encInfo->fptr_src_image);
    encInfo->secret_size = get_file_size(encInfo->fptr_secret);

    /*
       16 bytes  -> magic string (2 characters x 8)
       32 bytes  -> extension size
       N x 8     -> extension
       32 bytes  -> secret file size
       S x 8     -> secret file data
    */
    required_bytes = 16UL + 32UL +
                     ((unsigned long)strlen(encInfo->secret_extn) * 8UL) +
                     32UL + ((unsigned long)encInfo->secret_size * 8UL);

    if (encInfo->image_capacity >= required_bytes)
        return e_success;

    printf("ERROR: Image does not have enough capacity.\n");
    printf("Required: %lu bytes, Available: %u bytes\n",
           required_bytes, encInfo->image_capacity);
    return e_failure;
}

Status copy_bmp_header(FILE *src, FILE *dest)
{
    unsigned char header[BMP_HEADER_SIZE];

    rewind(src);

    if (fread(header, 1, BMP_HEADER_SIZE, src) != BMP_HEADER_SIZE)
        return e_failure;

    if (fwrite(header, 1, BMP_HEADER_SIZE, dest) != BMP_HEADER_SIZE)
        return e_failure;

    return e_success;
}

Status encode_byte_to_lsb(unsigned char data, FILE *src, FILE *dest)
{
    unsigned char image_bytes[8];
    int i;

    if (fread(image_bytes, 1, 8, src) != 8)
        return e_failure;

    for (i = 0; i < 8; i++)
    {
        image_bytes[i] = (image_bytes[i] & 0xFE) | ((data >> i) & 1);
    }

    if (fwrite(image_bytes, 1, 8, dest) != 8)
        return e_failure;

    return e_success;
}

Status encode_int_to_lsb(unsigned int data, FILE *src, FILE *dest)
{
    unsigned char image_bytes[32];
    int i;

    if (fread(image_bytes, 1, 32, src) != 32)
        return e_failure;

    for (i = 0; i < 32; i++)
    {
        image_bytes[i] = (image_bytes[i] & 0xFE) | ((data >> i) & 1);
    }

    if (fwrite(image_bytes, 1, 32, dest) != 32)
        return e_failure;

    return e_success;
}

Status encode_string_to_image(const char *data, FILE *src, FILE *dest)
{
    size_t i;

    for (i = 0; i < strlen(data); i++)
    {
        if (encode_byte_to_lsb((unsigned char)data[i], src, dest) == e_failure)
            return e_failure;
    }

    return e_success;
}

Status encode_file_data(EncodeInfo *encInfo)
{
    unsigned char ch;
    uint i;

    rewind(encInfo->fptr_secret);

    for (i = 0; i < encInfo->secret_size; i++)
    {
        if (fread(&ch, 1, 1, encInfo->fptr_secret) != 1)
            return e_failure;

        if (encode_byte_to_lsb(ch, encInfo->fptr_src_image,
                               encInfo->fptr_stego_image) == e_failure)
            return e_failure;
    }

    return e_success;
}

Status copy_remaining_data(FILE *src, FILE *dest)
{
    unsigned char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, dest) != bytes_read)
            return e_failure;
    }

    if (ferror(src))
        return e_failure;

    return e_success;
}

Status do_encoding(EncodeInfo *encInfo)
{
    unsigned int extension_size;

    if (open_encode_files(encInfo) == e_failure)
        return e_failure;

    printf("Opening files... SUCCESS\n");

    if (check_capacity(encInfo) == e_failure)
    {
        close_encode_files(encInfo);
        return e_failure;
    }
    printf("Checking image capacity... SUCCESS\n");

    if (copy_bmp_header(encInfo->fptr_src_image,
                        encInfo->fptr_stego_image) == e_failure)
    {
        close_encode_files(encInfo);
        return e_failure;
    }

    if (encode_string_to_image(MAGIC_STRING,
                               encInfo->fptr_src_image,
                               encInfo->fptr_stego_image) == e_failure)
    {
        close_encode_files(encInfo);
        return e_failure;
    }
    printf("Encoding magic string... SUCCESS\n");

    extension_size = (unsigned int)strlen(encInfo->secret_extn);
    if (encode_int_to_lsb(extension_size,
                          encInfo->fptr_src_image,
                          encInfo->fptr_stego_image) == e_failure)
    {
        close_encode_files(encInfo);
        return e_failure;
    }

    if (encode_string_to_image(encInfo->secret_extn,
                               encInfo->fptr_src_image,
                               encInfo->fptr_stego_image) == e_failure)
    {
        close_encode_files(encInfo);
        return e_failure;
    }
    printf("Encoding file extension... SUCCESS\n");

    if (encode_int_to_lsb(encInfo->secret_size,
                          encInfo->fptr_src_image,
                          encInfo->fptr_stego_image) == e_failure)
    {
        close_encode_files(encInfo);
        return e_failure;
    }
    printf("Encoding secret file size... SUCCESS\n");

    if (encode_file_data(encInfo) == e_failure)
    {
        close_encode_files(encInfo);
        return e_failure;
    }
    printf("Encoding secret file data... SUCCESS\n");

    if (copy_remaining_data(encInfo->fptr_src_image,
                            encInfo->fptr_stego_image) == e_failure)
    {
        close_encode_files(encInfo);
        return e_failure;
    }

    close_encode_files(encInfo);
    printf("Encoding completed successfully.\n");
    printf("Output image: %s\n", encInfo->stego_image_fname);

    return e_success;
}
