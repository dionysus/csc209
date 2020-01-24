#include <stdio.h>
#include <stdlib.h>

#include "benford_helpers.h"

/*
 * The only print statement that you may use in your main function is the following:
 * - printf("%ds: %d\n")
 *
 */
int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "benford position [datafile]\n");
        return 1;
    }
   
    int i;
    
    // initialize tally
    int tally[BASE];
    for (i = 0; i < BASE; i++){
        tally[i] = 0;
    } 
    
    // tally input
    
    int pos = strtol(argv[1], NULL, 10);
    int num = strtol(argv[2], NULL, 10);

    printf("pos: %d\n", pos);
    printf("num: %d\n", num);
    printf("str: %s\n", argv[2]);

    add_to_tally(num, pos, tally);
            
    // print tally
    for (i = 0; i < BASE; i++){
        printf("%ds: %d\n", i, tally[i]);
    }

}
