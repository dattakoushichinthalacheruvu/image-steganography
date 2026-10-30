#include <stdio.h>
#include "encode.h"
#include "decode.h"

static void print_usage(const char *program)
{
    printf("\nSteganography - Hide data inside a 24-bit BMP image\n\n");
    printf("Encoding:\n");
    printf("  %s -e source.bmp secret.txt output.bmp\n", program);
    printf("  %s -e source.bmp secret.txt\n", program);
    printf("\nDecoding:\n");
    printf("  %s -d stego.bmp output.txt\n", program);
    printf("  %s -d stego.bmp\n\n", program);
}

int main(int argc, char *argv[])
{
    EncodeInfo encInfo = {0};
    DecodeInfo decInfo = {0};
    OperationType operation;

    if (argc < 3)
    {
        print_usage(argv[0]);
        return 1;
    }

    operation = check_operation_type(argv);

    if (operation == e_encode)
    {
        if (argc < 4 || argc > 5)
        {
            print_usage(argv[0]);
            return 1;
        }

        if (read_and_validate_encode_args(argv, &encInfo) == e_failure)
        {
            printf("ERROR: Invalid encoding arguments.\n");
            print_usage(argv[0]);
            return 1;
        }

        return do_encoding(&encInfo) == e_success ? 0 : 1;
    }

    if (operation == e_decode)
    {
        if (argc < 3 || argc > 4)
        {
            print_usage(argv[0]);
            return 1;
        }

        if (read_and_validate_decode_args(argv, &decInfo) == e_failure)
        {
            printf("ERROR: Invalid decoding arguments.\n");
            print_usage(argv[0]);
            return 1;
        }

        return do_decoding(&decInfo) == e_success ? 0 : 1;
    }

    printf("ERROR: Invalid operation. Use -e for encoding or -d for decoding.\n");
    print_usage(argv[0]);
    return 1;
}
