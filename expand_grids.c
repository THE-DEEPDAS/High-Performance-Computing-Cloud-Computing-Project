#include <stdio.h>
#include <stdlib.h>

#define NEW_N 500   
#define TOTAL 20

int read_small_grid(const char *filename, int grid[200][200], int *n) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;

    fscanf(fp, "%d %d", n, n);

    for (int i = 0; i < *n; i++)
        for (int j = 0; j < *n; j++)
            fscanf(fp, "%d", &grid[i][j]);

    fclose(fp);
    return 1;
}

int main() {
    int small[200][200];
    int n;

    for (int g = 0; g < TOTAL; g++) {

        char src[50];
        sprintf(src, "grid_%d.txt", g % 5);

        if (!read_small_grid(src, small, &n)) {
            printf("Error reading %s\n", src);
            continue;
        }

        char out[50];
        sprintf(out, "big_grid_%d.txt", g);

        FILE *fp = fopen(out, "w");
        fprintf(fp, "%d %d\n", NEW_N, NEW_N);

        for (int i = 0; i < NEW_N; i++) {
            for (int j = 0; j < NEW_N; j++) {

                // tile + noise
                int val = small[i % n][j % n];
                val += rand() % 20; // variation

                if (val > 100) val = 100;

                fprintf(fp, "%d ", val);
            }
            fprintf(fp, "\n");
        }

        fclose(fp);
        printf("Generated %s\n", out);
    }

}