STEGANOGRAPHY PROJECT - BEGINNER FRIENDLY C VERSION

This project hides a secret file inside a 24-bit BMP image using LSB
(Least Significant Bit) steganography.

FILES
-----
main.c       -> Starts the program and selects encode/decode.
encode.c     -> Hides the secret data inside the BMP image.
encode.h     -> Encode function declarations and EncodeInfo structure.
decode.c     -> Extracts the secret data from the stego image.
decode.h     -> Decode function declarations and DecodeInfo structure.
common.h     -> Common constants such as the magic string.
types.h      -> Status and operation types.

HOW TO COMPILE
--------------
GCC:
    gcc main.c encode.c decode.c -o steganography

Windows MinGW:
    gcc main.c encode.c decode.c -o steganography.exe

HOW TO ENCODE
-------------
    steganography -e beautiful.bmp (nature landscape image) secret.txt stego.bmp

If you do not provide the output image name:
    steganography -e beautiful.bmp (nature landscape image) secret.txt

The default output image will be:
    stego.bmp

HOW TO DECODE
-------------
    steganography -d stego.bmp decode.txt

If you do not provide the output file name:
    steganography -d stego.bmp

The program automatically creates a file such as:
    decode.txt

IMPORTANT
---------
1. The source image must be a 24-bit BMP file.
2. The secret file can be a text file or another small binary file.
3. The source image must have enough capacity for the secret data.
4. Do not edit or compress stego.bmp after encoding.

BASIC IDEA
----------
For every secret byte, the program takes 8 image bytes.
It stores one secret bit in the least significant bit (LSB) of each
image byte.

Example:
Secret bit: 1
Image byte: 10010010
New byte:   10010011

Only the last bit changes, so the image looks almost unchanged.
