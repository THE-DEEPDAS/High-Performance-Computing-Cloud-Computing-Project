#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX 600

int grid[MAX][MAX];

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0)
            printf("Usage: %s input.txt output.txt\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int n, m;

    if (rank == 0) {
        FILE *fp = fopen(argv[1], "r");
        if (!fp) {
            printf("Error opening input file\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fscanf(fp, "%d %d", &n, &m);

        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++)
                fscanf(fp, "%d", &grid[i][j]);

        fclose(fp);
    }

    // broadcast dimensions
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // broadcast grid
    MPI_Bcast(grid, MAX*MAX, MPI_INT, 0, MPI_COMM_WORLD);

    int rows = n / size;
    int start = rank * rows;
    int end = (rank == size - 1) ? n : start + rows;

    int local_sum = 0;

    for (int i = start; i < end; i++) {
        for (int j = 0; j < m; j++) {
            local_sum += grid[i][j];
        }
    }

    int global_sum = 0;

    MPI_Reduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        FILE *out = fopen(argv[2], "w");

        fprintf(out, "Grid Size: %d x %d\n", n, m);
        fprintf(out, "Total Grid Cost (sanity check): %d\n", global_sum);
        fprintf(out, "Processes Used: %d\n", size);

        fclose(out);

        printf("Output written to %s\n", argv[2]);
    }

    MPI_Finalize();
}