#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stddef.h>
#include <emmintrin.h>
#include <string.h>
#include <immintrin.h>
#include <inttypes.h>
#include <time.h>
const char *help_msg =
    "Positional arguments:\n"
    "  <file>   Input file.\n"
    "\n"
    "Optional arguments:\n"
    "  -V X   The implementation that should be used. (default: X = 0 -> C with memset) (X = 1 -> assembly_opt) (X = 2 -> c basic) (X = 1 -> assembly basic)\n"
    "  -B X   Outputs the runtime of the specified implementation, with x, the number of repetitions of the function call. \n"
    "  -o<file>   Output file\n"
    "  -h     A description of all options of the program, usage examples, and exit.\n"
    "  --help     A description of all options of the program, usage examples, and exit.\n"
    
    "Usage example:\n"
    "  ./main <InputFile> -o <OutputFile> \n";

void print_help()
{
    fprintf(stderr, "\n%s", help_msg);
}

void bmp_rld_C_opt(const uint8_t *rle_data, size_t len, size_t width, size_t height, uint8_t *img)
{
    size_t j = 0; //img counter
    size_t rep; //number of times a byte should be repeated  
    uint8_t num; //dummby variable
    size_t up;   // number of columns to be skipped in delta case
    size_t right; // number of bytes in the right direction to be skipped in delta case
    size_t pass;  // total number of bytes to be copied
    size_t linecounter = 0; //count the number of filled bytes in the current row
    size_t linePadd = (4 - width % 4) % 4;  //count the number of padding bytes to be added if any

    if(height<=0 || width<=0 || len<=0){
            printf("invalid input file format\n");
            return;
        }

    for (size_t i = 0; i < len;)  //itraring through the rle_data file
    {
        if (rle_data[i] == 0)
        {
            switch (rle_data[i + 1])
            {
            case 0: //lineBreak
                i = i + 2;
                pass = width - linecounter;
                num=0;
                memset(&img[j],num,pass);
                j += pass+ linePadd; //adding padding bytes to the imgCounter 
                linecounter = 0;
                break;
            case 1: //End of File
                pass = width - linecounter;
                num=0;
                memset(&img[j],num,pass);
                j+=pass;
                return;
            case 2:  //Delta case
                i += 2;
                right = rle_data[i++];
                up = rle_data[i++];
                pass = up * width + right + linePadd * up; //calculating the total bytes to be skipped
                linecounter += right;
                num=0;
                memset(&img[j],num,pass); //filling the escaped bytes with 0 
                j+=pass;
                break;
            default:  //Absolute Mode
                rep = rle_data[i + 1];
                i = i + 2;
                memcpy(&img[j], &rle_data[i], rep);  //copy rep bytes from rle_data into img
                i += rep;
                linecounter += rep;
                j += rep;
                if (rep % 2 != 0) //skip one byte if rep is odd
                {
                    i++;
                }
            }
        }
        else  //Encoded mode
        {
            rep = rle_data[i++];
            num = rle_data[i++];
            linecounter += rep;
            memset(&img[j],num,rep); //fill rep bytes with num
            j+=rep;
        }
    }
}

void bmp_rld_c_basic(const uint8_t *rle_data, size_t len, size_t width, size_t height, uint8_t *img)
{
    size_t j = 0; //img counter
    size_t limit; // total number of bytes to be copied + j counter
    size_t rep;  //number of times a byte should be repeated 
    size_t num;  //dummby variable
    size_t up;  // number of colums to be skipped in delta case
    size_t right; // number of bytes in thr right direction to be skipped in delta case
    size_t pass; // total number of bytes to be copied
    size_t linecounter = 0; //count the number of filled bytes in the current row
    size_t linePadd = (4 - width % 4) % 4; //count the number of padding bytes to be added if any
     if(height<=0 || width<=0 || len<=0){
            printf("Invalid input file format\n");
            return;
        }
    for (size_t i = 0; i < len;)
    {
        if (rle_data[i] == 0 && rle_data[i + 1] > 2)
        {
            rep = rle_data[i + 1];
            linecounter += rep;
            limit = j + rle_data[i + 1];
            i = i + 2;
            for (; j < limit; j++)
            {
                img[j] = rle_data[i++];
            }
            if (rep % 2 != 0)
            {
                i++;
            }
        }
        else if (rle_data[i] == 0 && rle_data[i + 1] == 0) //end of line
        {
            pass = width - linecounter;
            limit = j + pass;
            for (; j < limit; j++)
            {
                img[j] = 0;
            }
            j += linePadd;
            i = i + 2;
            linecounter = 0;
        }
        else if (rle_data[i] == 0 && rle_data[i + 1] == 1) //end of file
        {
            pass = width - linecounter;
            limit = j + pass;
            for (; j < limit; j++) //adding zero byte by byte
            {
                img[j] = 0;
            }
            return;
        }
        else if (rle_data[i] == 0 && rle_data[i + 1] == 2) //delta case
        {
            i += 2;
            right = rle_data[i++];
            up = rle_data[i++];
            pass = up * width + right + linePadd * up;
            linecounter += right;
            limit = j + pass;
            for (; j < limit; j++)
            {
                img[j] = 0;
            }
        }
        else
        {
            rep = rle_data[i++];
            linecounter += rep;
            num = rle_data[i++];
            limit = j + rep;
            for (; j < limit; j++) //coby pixels byte by byte
            {
                img[j] = num;
            }
        }
    }
}

int main(int argc, char **argv)
{
    char *inputFile = "";
    if (argv[1] == NULL)
    {
        printf("Can not open input file.\n");
        exit(-1);
    }
    else
    {
        inputFile = argv[1];
    }
    char *outputFile = NULL;

    const char *optstring = "V:B:o:h";
    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'}};
    int opt;
    int impNr = 0; //decide which implementation to be used 
    int iterations = 1; 
    int printRuntime = 0; // Variable that tells us whether runtime should be printed or not 
    while ((opt = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) //handling options
    {
        switch (opt)
        {
        case 'V':
            impNr = atoi(optarg);
            if (impNr < 0 || impNr > 3){
                printf("Invalid implementation version!\nWrite -h for help.\n");
                exit(-1);
            }
            break;
        case 'h':
            print_help();
            return EXIT_SUCCESS;
        case 'B':
            iterations = atoi(optarg); // Parse iterations
            if (iterations <= 0){
                printf("Invalid amount of iterations!\nWrite -h for help.\n");
                exit(-1);
            }
            printRuntime = 1; // Runtime should be printed
            break;
        case 'o':
            outputFile = optarg;
            break;
        default: /* ? */
            print_help();
            return EXIT_FAILURE;
        }
    }

    FILE *df;

    if (!(df = fopen(inputFile, "rb"))) //opning the inputfile
    {
        printf("The source file isn't valid!\nWrite -h for help.\n");
        exit(-1);
    };

    uint8_t *info = malloc(50*sizeof(uint8_t)); //allocating space fpr the first 50 bytes of the header
    if (!info)
    {
        printf("Malloc allocation failed.\n");
        fclose(df);
        exit(-1);
    }

    if (!fread(info, sizeof(uint8_t), 50, df)) //reading the first 50 bytes of the input file 
    {
        printf("Failure when reading from input file.\n");
        free(info);
        fclose(df);
        exit(-1);
    }

    int offset = *(int *)&info[10]; // bytes from 10 to 13 save the index to the actual image 
    int width = *(int *)&info[18];  // bytes from 18 to 21 contain the width of the image
    int height = *(int *)&info[22]; // bytes from 22 to 25 contain the height of the image
    int imgSize = *(int *)&info[34]; // bytes from 34 to 37 contain the size of the image
    if (info[30] != 1 || info[28]!=8) //checking if the file is rle8 compressed
    {
        printf("Invalid input file format.\n");
        fclose(df);
        exit(-1);
    }
    info[30] = 0; // change the byte responsibe for compression from one to zero
    int restSize = (int)offset - 50; //the remaining size of the header

    uint8_t *rest = malloc(restSize*sizeof(uint8_t)); //allocating space for the remaining size
    if (!rest)
    {
        printf("Malloc allocation failed.\n");
        fclose(df);
        exit(-1);
    }

    if (!fread(rest, sizeof(uint8_t), restSize, df)) //reading the remaining hrader bytes
    {
        printf("Failure when reading from input file.\n");
        free(info);
        free(rest);
        fclose(df);
        exit(-1);
    }

    uint8_t *data = malloc(imgSize*sizeof(uint8_t)); //allocating space for the actual image 
    if (!data)
    {
        printf("Malloc allocation failed.\n");
        fclose(df);
        exit(-1);
    }

    if (!fread(data, sizeof(uint8_t), imgSize, df)) //reading the actual image 
    {
        printf("Failure when reading from input file.\n");
        free(info);
        free(rest);
        free(data);
        fclose(df);
        exit(-1);
    }

    fclose(df);

    uint8_t *img = malloc((height * width + height * ((4 - width % 4) % 4))*sizeof(uint8_t)); // allocating space for the decompressed image

    if (printRuntime){ // Print runtime and average runtime for the amount of iterations the user has given.
        switch (impNr)
        {
        case 2:;
            struct timespec start2;
            clock_gettime(CLOCK_MONOTONIC, &start2);
            for (int i = 0; i < iterations; i++){
            bmp_rld_c_basic(data, imgSize, width, height, img);
            }
            struct timespec end2; 
            clock_gettime(CLOCK_MONOTONIC, &end2);
            double time2 = end2.tv_sec - start2.tv_sec + 1e-9 * (end2.tv_nsec - start2.tv_nsec);
            double avg2 = time2/iterations;
            printf("Implementation: Basic C\n");
            printf("Iterations: %d\n", iterations);
            printf("Runtime: %lf\n", time2);
            printf("Average runtime: %lf\n", avg2);
            break;
        default:;
            struct timespec startDefault;
            clock_gettime(CLOCK_MONOTONIC, &startDefault);
            for (int i = 0; i < iterations; i++){
            bmp_rld_C_opt(data, imgSize, width, height, img);
            }
            struct timespec endDefault; 
            clock_gettime(CLOCK_MONOTONIC, &endDefault);
            double timeDefault = endDefault.tv_sec - startDefault.tv_sec + 1e-9 * (endDefault.tv_nsec - startDefault.tv_nsec);
            double avgDefault = timeDefault/iterations;
            printf("Implementation: C with memset and memcpy\n");
            printf("Iterations: %d\n", iterations);
            printf("Runtime: %lf\n", timeDefault);
            printf("Average runtime: %lf\n", avgDefault);
            break;
        }
    }
    else
    {
        switch (impNr) //calling the decompression function
        {
        case 2:
            bmp_rld_c_basic(data, imgSize, width, height, img);
            break;
        default:
            bmp_rld_C_opt(data, imgSize, width, height, img);
            break;
        }
    }
    free(data);
    
    FILE *out;

    if (!(out = fopen(outputFile, "wb"))) //opning the output file
    {
        printf("You didn't provide a valid output file.\nWrite -h for help.\n");
        goto invalid;
    };

    if (!fwrite(info, sizeof(uint8_t), 50, out)) //writing the first 50 bytes of the header to the output file
    {
        printf("Failure while writing in output file.\n");
        goto invalid;
    }
    if (!fwrite(rest, sizeof(uint8_t), restSize, out)) //writing the remaining bytes of the header to the output file
    {
        printf("Failure while writing in output file.\n");
        goto invalid;
    }
    if (!fwrite(img, sizeof(uint8_t), height * width + height * ((4 - width % 4) % 4), out)) //writing the image to the output file
    {
        printf("Failure while writing in output file.\n");
        goto invalid;
    }
    //closing the output file and free spaces
    fclose(out); 
    free(info);
    free(rest);
    free(img);

    return 0;

    invalid:
    fclose(out); 
    free(info);
    free(rest);
    free(img);

    return EXIT_FAILURE;

    }