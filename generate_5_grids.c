#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 200   
#define NUM_GRIDS 5

int main() {
    srand(time(NULL));

    for (int g = 0; g < NUM_GRIDS; g++) {
        char filename[50];
        sprintf(filename, "grid_%d.txt", g);

        FILE *fp = fopen(filename, "w");

        fprintf(fp, "%d %d\n", N, N);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                int density = rand() % 100 + 1;
                fprintf(fp, "%d ", density);
            }
            fprintf(fp, "\n");
        }

        fclose(fp);
        printf("Generated %s\n", filename);
    }
}