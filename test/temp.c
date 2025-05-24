#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Generate and print 10 random numbers between 0 and 99
    printf("Random numbers:\n");
    for (int i = 0; i < 10; i++) {
        int r = rand() % 100;
        printf("%d ", r);
    }
    printf("\n");

    return 0;
}