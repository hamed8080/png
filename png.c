#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define IHDR_TYPE 0x52444849 // 'IHDR' in big-endian binary representation
#define IEND_TYPE 0x444E4549 // 'IEND' in big-endian binary representation
#define PNG_SIG_CAP 8
#define CHUNK_BUFF_CAP (32 * 1024)
uint8_t chunk_buff[CHUNK_BUFF_CAP]; // The data buffer of the data section of the each PNG chunk.
const uint8_t PNG_SIG[PNG_SIG_CAP] = {137, 80, 78, 71, 13, 10, 26, 10};

#define read_bytes_or_panic(file, buff, buf_cap) read_bytes_or_panic_(file, buff, buf_cap, __FILE__, __LINE__)
#define write_bytes_or_panic(file, buff, buf_cap) write_bytes_or_panic_(file, buff, buf_cap, __FILE__, __LINE__)

void read_bytes_or_panic_(FILE *file, void *buff, size_t buf_cap, const char *source_file, int source_line);
void write_bytes_or_panic_(FILE *file, void *buff, size_t buf_cap, const char *source_file, int source_line);

void print_bytes(uint8_t *buff);
void reverse_bytes(void *buff, size_t cap);

int main(int argc, char **argv)
{

    (void)argc;
    assert(*argv != NULL);
    // take the first element pointer or argv[0] then move the pointer to the next element and set it to argv variable
    char *program = *argv++;

    if (*argv == NULL)
    {
        fprintf(stderr, "Usage: %s <input.png> <output.png>\n", program);
        fprintf(stderr, "ERROR: No input file is provided!\n");
        exit(1);
    }

    char *input_filepath = *argv++;
    printf("Inspected file is %s \n", input_filepath);
    FILE *input_file = fopen(input_filepath, "rb");
    if (input_file == NULL)
    {
        fprintf(stderr, "ERROR: errorno: %d\n", errno);
        fprintf(stderr, "ERROR: Could not open the file %s: %s\n", input_filepath, strerror(errno));
        exit(1);
    }

    if (*argv == NULL)
    {
        fprintf(stderr, "Usage: %s <input.png> <output.png>\n", program);
        fprintf(stderr, "ERROR: No output file is provided!\n");
        exit(1);
    }
    char *output_filepath = *argv++;

    FILE *output_file = fopen(output_filepath, "wb");
    if (output_file == NULL)
    {
        fprintf(stderr, "ERROR: Could not open the  output file %s: %s\n", output_filepath, strerror(errno));
        exit(1);
    }
    // int arr[] = {1, 2, 3, 4, 5};

    /*
    64bit or 8 bytes exatly the size of the signature in the document.
    The sig array has 8 items and each item has 8 bits or 1 byte.
    So we are telling that to read all '64 bits/8 bytes' in one buffer.
     */
    uint8_t sig[PNG_SIG_CAP];
    read_bytes_or_panic(input_file, sig, sizeof(sig));
    write_bytes_or_panic(output_file, sig, sizeof(sig));
    printf("Signature: ");
    print_bytes(sig);

    if (memcmp(sig, PNG_SIG, PNG_SIG_CAP) != 0)
    {
        fprintf(stderr, "ERROR: %s file does not appear to be a valid PNG image.\n", input_filepath);
        exit(1);
    }

    printf("\n\n");

    bool quite = false;

    while (!quite)
    {
        // Read the lenght header
        uint32_t chunk_size;
        read_bytes_or_panic(input_file, &chunk_size, sizeof(chunk_size));
        write_bytes_or_panic(output_file, &chunk_size, sizeof(chunk_size));

        // uint32_t longEndian = ntohl(chunk_size); // it will print 13 like the below function, if we use the PNG_transparency_demonstration_1.png.
        reverse_bytes(&chunk_size, sizeof(chunk_size));

        uint8_t chunk_type[4];
        read_bytes_or_panic(input_file, &chunk_type, sizeof(chunk_type));
        write_bytes_or_panic(output_file, &chunk_type, sizeof(chunk_type));
        // Convert the type to host byte order if needed (from big-endian)
        if (*(uint32_t *)chunk_type == IEND_TYPE)
        {
            quite = true;
        }

        // SKIP the data chunk: SEEK_CUR set it to Current position.
        // if (fseek(input_file, chunk_size, SEEK_CUR) < 0)
        // {
        //     fprintf(stderr, "ERROR: could not skip a chunk: %s\n", strerror(errno));
        //     exit(1);
        // }

        // Copying all chunk data by 32 kilo bytes
        size_t n = chunk_size;
        while (n > 0)
        {
            size_t m = n;
            if (m > CHUNK_BUFF_CAP)
            {
                m = CHUNK_BUFF_CAP;
            }
            read_bytes_or_panic(input_file, chunk_buff, m);
            write_bytes_or_panic(output_file, chunk_buff, m);
            n -= m;
        }

        uint32_t chunk_crc;
        read_bytes_or_panic(input_file, &chunk_crc, sizeof(chunk_crc));
        write_bytes_or_panic(output_file, &chunk_crc, sizeof(chunk_crc));

        // Inject new custom chunk 
        if (*(uint32_t *)chunk_type == IHDR_TYPE) {
            uint32_t injected_size = 3;
            reverse_bytes(&injected_size, sizeof(injected_size)); // first reverse it to write it 
            write_bytes_or_panic(output_file, &injected_size, sizeof(injected_size));
            reverse_bytes(&injected_size, sizeof(injected_size)); // second reverse it to print out and use on other functions 
            
            char *injected_type = "itYP";
            write_bytes_or_panic(output_file, injected_type, 4);
            
            char *injected_data = "HI!";
            write_bytes_or_panic(output_file, injected_data, 3);
            
            uint32_t injected_crc = 0;
            write_bytes_or_panic(output_file, &injected_crc, sizeof(injected_crc));
        }

        printf("Chunk size is: %u\n", chunk_size);
        printf("Chunk type is: %.*s (0x%08X) \n", (int)sizeof(chunk_type), chunk_type, *(uint32_t *)chunk_type);
        printf("Chunk CRC: 0x%08X\n", chunk_crc);

        printf("------------------------------------\n");
    }
    printf("Decimal value is: %u", IEND_TYPE);

    printf("\n");
    fclose(input_file);
    fclose(output_file);

    return 0;
}

void read_bytes_or_panic_(FILE *file, void *buff, size_t buf_cap, const char *source_file, int source_line)
{
    // one buffer of 8 bits, we can also do this other way around: fread(sig, 1, sizeof(sig), input_file)
    size_t n = fread(buff, buf_cap, 1, file);

    // n != 1 means that we couldn't read one buffer that we expected.
    if (n != 1)
    {
        if (ferror(file))
        {
            fprintf(stderr, "%s:%d ERROR: Could not read the PNG header: %s\n", source_file, source_line, strerror(errno));
            exit(1);
        }
        else if (feof(file))
        {
            fprintf(stderr, "%s:%d ERROR: Could not read the PNG: reached the end of file: %s\n", source_file, source_line, strerror(errno));
            exit(1);
        }
        else
        {
            assert(0 && "unreachable");
        }
    }
}

void write_bytes_or_panic_(FILE *file, void *buff, size_t buf_cap, const char *source_file, int source_line)
{
    size_t n = fwrite(buff, buf_cap, 1, file);
    if (n != 1)
    {
        if (ferror(file))
        {
            fprintf(stderr, "%s:%d ERROR: Could not write to the output file: %s\n", source_file, source_line, strerror(errno));
            exit(1);
        }
        else
        {
            assert(0 && "unreachable");
        }
    }
}

void print_bytes(uint8_t *buff)
{
    for (unsigned long i = 0; i < sizeof(buff); i++)
    {
        printf("%u ", buff[i]);
    }
    printf("\n");
}

void reverse_bytes(void *buff0, size_t cap)
{
    uint8_t *buff = buff0;
    for (size_t i = 0; i < cap / 2; i++)
    {
        uint8_t t = buff[i];
        // ORIGINAL: 1 2 3 4 5
        // REVERSE: 5 4 3 2 1 //  5 - 1 - 1
        buff[i] = buff[cap - i - 1];
        buff[cap - i - 1] = t;
    }
}