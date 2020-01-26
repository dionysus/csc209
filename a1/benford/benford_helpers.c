#include <stdio.h>

#include "benford_helpers.h"

int count_digits(int num) {
    int count = 0;
    for (int k = num; k > 0; k /= BASE) {
        count++;
    }
    return count;
}

int get_ith_from_right(int num, int i) {
    for (int j = 0; j < i; j++) {
        num /= BASE; 
    } 
    return num % BASE;
}

int get_ith_from_left(int num, int i) {
    return get_ith_from_right(num, count_digits(num) - (i + 1));
}

void add_to_tally(int num, int i, int *tally) {
    int digit = get_ith_from_left(num, i);
    tally[digit]++; 
    return;
}
