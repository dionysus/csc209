#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"

int OFFSET_PIXEL = 10;
int OFFSET_WIDTH = 18;
int OFFSET_HEIGHT = 22;

/*
 * Read in the location of the pixel array, the image width, and the image 
 * height in the given bitmap file.
 */
void read_bitmap_metadata(FILE *image, int *pixel_array_offset, int *width, int *height) {
    // OFFSET PIXEL
        fseek(image, OFFSET_PIXEL, SEEK_SET);
        fread(pixel_array_offset, 4, 1, image);
    // OFFSET WIDTH
        fseek(image, OFFSET_WIDTH, SEEK_SET);
        fread(width, 4, 1, image);
    // OFFSET HEIGHT 
        fseek(image, OFFSET_HEIGHT, SEEK_SET);
        fread(height, 4, 1, image);
    return;
}

/*
 * Read in pixel array by following these instructions:
 *
 * 1. First, allocate space for m `struct pixel *` values, where m is the 
 *    height of the image.  Each pointer will eventually point to one row of 
 *    pixel data.
 * 2. For each pointer you just allocated, initialize it to point to 
 *    heap-allocated space for an entire row of pixel data.
 * 3. Use the given file and pixel_array_offset to initialize the actual 
 *    struct pixel values. Assume that `sizeof(struct pixel) == 3`, which is 
 *    consistent with the bitmap file format.
 *    NOTE: We've tested this assumption on the Teaching Lab machines, but 
 *    if you're trying to work on your own computer, we strongly recommend 
 *    checking this assumption!
 * 4. Return the address of the first `struct pixel *` you initialized.
 */
struct pixel **read_pixel_array(FILE *image, int pixel_array_offset, int width, int height) {

    unsigned char blue;
    unsigned char green;
    unsigned char red;

    // navigate to offest_pixel.
    fseek(image, pixel_array_offset, SEEK_SET);

    // allocate heap space for array of pointers for each row
    struct pixel **pixel_array = malloc(sizeof(struct pixel *) * height);

    for (int i = 0; i < height; i ++){
        //make new row in heap for width number of structs
        // struct pixel *new_row = malloc(sizeof(struct pixel) * width);
        struct pixel *new_row = malloc(3 * width);
        // for each set of 3 pixels in width
        for (int j = 0; j < width; j ++){
            // read the pixels into vars
            fread(&blue, sizeof(unsigned char), 1, image);
            fread(&green, sizeof(unsigned char), 1, image);
            fread(&red, sizeof(unsigned char), 1, image);
            // assign the struct value to corresponding var
            new_row[j].blue = blue;
            new_row[j].green = green;
            new_row[j].red = red;
        }
        // point the row of the pixel_array to the newly created row
        pixel_array[i] = new_row;
    };
    
    return pixel_array;
}


/*
 * Print the blue, green, and red colour values of a pixel.
 * You don't need to change this function.
 */
void print_pixel(struct pixel p) {
    printf("(%u, %u, %u)\n", p.blue, p.green, p.red);
}
