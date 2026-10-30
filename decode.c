#include <stdio.h>
#include <string.h>
#include "decode.h"

static void close_decode_files(DecodeInfo *decInfo)
{
    if (decInfo->fptr_stego_image != NULL)
        fclose(decInfo->fptr_stego_image);
    if (decInfo->fptr_output != NULL)
        fclose(decInfo->fptr_output);

    decInfo->fptr_stego_image = NULL;
    decInfo->fptr_output = NULL;
}

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    if (argv[2] == NULL)
        return e_failure;

    if (strstr(argv[2], ".bmp") == NULL)
        return e_failure;

    decInfo->stego_image_fname = argv[2];

    if (argv[3] != NULL)
        decInfo->output_fname = argv[3];
    else
        decInfo->output_fname = NULL;

    return e_success;
}

Status open_decode_file(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");

    if (decInfo->fptr_stego_image == NULL)
    {
        printf("ERROR: Cannot open stego image: %s\n",
               decInfo->stego_image_fname);
        return e_failure;
    }

    return e_success;
}

Status decode_byte_from_lsb(unsigned char *data, FILE *image)
{
    unsigned char image_bytes[8];
    unsigned char value = 0;
    int i;

    if (fread(image_bytes, 1, 8, image) != 8)
        return e_failure;

    for (i = 0; i < 8; i++)
    {
        value = value | ((image_bytes[i] & 1) << i);
    }

    *data = value;
    return e_success;
}

Status decode_int_from_lsb(unsigned int *data, FILE *image)
{
    unsigned char image_bytes[32];
    unsigned int value = 0;
    int i;

    if (fread(image_bytes, 1, 32, image) != 32)
        return e_failure;

    for (i = 0; i < 32; i++)
    {
        value = value | ((unsigned int)(image_bytes[i] & 1) << i);
    }

    *data = value;
    return e_success;
}

Status decode_string_from_image(char *data, int size, FILE *image)
{
    int i;
    unsigned char ch;

    for (i = 0; i < size; i++)
    {
        if (decode_byte_from_lsb(&ch, image) == e_failure)
            return e_failure;

        data[i] = (char)ch;
    }

    data[size] = '\0';
    return e_success;
}

Status decode_file_data(DecodeInfo *decInfo)
{
    unsigned int i;
    unsigned char ch;

    for (i = 0; i < decInfo->secret_size; i++)
    {
        if (decode_byte_from_lsb(&ch, decInfo->fptr_stego_image) == e_failure)
            return e_failure;

        if (fwrite(&ch, 1, 1, decInfo->fptr_output) != 1)
            return e_failure;
    }

    return e_success;
}

Status do_decoding(DecodeInfo *decInfo)
{
    char magic[sizeof(MAGIC_STRING)];
    unsigned int extension_size;

    if (open_decode_file(decInfo) == e_failure)
        return e_failure;

    /* For a normal 24-bit BMP, pixel data starts at byte 54. */
    if (fseek(decInfo->fptr_stego_image, BMP_HEADER_SIZE, SEEK_SET) != 0)
    {
        close_decode_files(decInfo);
        return e_failure;
    }

    if (decode_string_from_image(magic,
                                 (int)strlen(MAGIC_STRING),
                                 decInfo->fptr_stego_image) == e_failure)
    {
        close_decode_files(decInfo);
        return e_failure;
    }

    if (strcmp(magic, MAGIC_STRING) != 0)
    {
        printf("ERROR: Magic string not found. This is not a valid stego image.\n");
        close_decode_files(decInfo);
        return e_failure;
    }
    printf("Magic string matched... SUCCESS\n");

    if (decode_int_from_lsb(&extension_size,
                            decInfo->fptr_stego_image) == e_failure)
    {
        close_decode_files(decInfo);
        return e_failure;
    }

    if (extension_size == 0 || extension_size >= MAX_EXTENSION_SIZE)
    {
        printf("ERROR: Invalid secret file extension size.\n");
        close_decode_files(decInfo);
        return e_failure;
    }

    if (decode_string_from_image(decInfo->secret_extn,
                                 (int)extension_size,
                                 decInfo->fptr_stego_image) == e_failure)
    {
        close_decode_files(decInfo);
        return e_failure;
    }

    if (decode_int_from_lsb(&decInfo->secret_size,
                            decInfo->fptr_stego_image) == e_failure)
    {
        close_decode_files(decInfo);
        return e_failure;
    }

    printf("Secret file extension: %s\n", decInfo->secret_extn);
    printf("Secret file size: %u bytes\n", decInfo->secret_size);

    if (decInfo->output_fname == NULL)
    {
        snprintf(decInfo->default_output_fname,
                 sizeof(decInfo->default_output_fname),
                 "decode%s", decInfo->secret_extn);
        decInfo->output_fname = decInfo->default_output_fname;
    }

    decInfo->fptr_output = fopen(decInfo->output_fname, "wb");
    if (decInfo->fptr_output == NULL)
    {
        printf("ERROR: Cannot create output file: %s\n",
               decInfo->output_fname);
        close_decode_files(decInfo);
        return e_failure;
    }

    if (decode_file_data(decInfo) == e_failure)
    {
        printf("ERROR: Unable to decode secret file data.\n");
        close_decode_files(decInfo);
        return e_failure;
    }

    close_decode_files(decInfo);
    printf("Decoding completed successfully.\n");
    printf("Output file: %s\n", decInfo->output_fname);

    return e_success;
}
