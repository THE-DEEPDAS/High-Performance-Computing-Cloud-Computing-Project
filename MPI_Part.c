#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define INF 1000000000

static int rows_for_rank(int n, int size, int rank) {
    int base = n / size;
    int rem = n % size;
    return base + (rank < rem ? 1 : 0);
}

static int start_row_for_rank(int n, int size, int rank) {
    int base = n / size;
    int rem = n % size;
    return rank * base + (rank < rem ? rank : rem);
}

static inline int idx(int row, int col, int m) {
    return row * m + col;
}

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

    int n = 0, m = 0;
    int *global_grid = NULL;

    if (rank == 0) {
        FILE *fp = fopen(argv[1], "r");
        if (!fp) {
            printf("Error opening input file\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fscanf(fp, "%d %d", &n, &m);

        global_grid = (int *)malloc((size_t)n * (size_t)m * sizeof(int));
        if (!global_grid) {
            printf("Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++)
                fscanf(fp, "%d", &global_grid[idx(i, j, m)]);

        fclose(fp);
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (n <= 0 || m <= 0) {
        if (rank == 0)
            printf("Invalid grid size\n");
        free(global_grid);
        MPI_Finalize();
        return 1;
    }

    int *send_counts = (int *)malloc((size_t)size * sizeof(int));
    int *displs = (int *)malloc((size_t)size * sizeof(int));
    if (!send_counts || !displs) {
        printf("Rank %d: memory allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int r = 0; r < size; r++) {
        int rr = rows_for_rank(n, size, r);
        send_counts[r] = rr * m;
        displs[r] = start_row_for_rank(n, size, r) * m;
    }

    int local_rows = rows_for_rank(n, size, rank);
    int start_row = start_row_for_rank(n, size, rank);

    int *local_grid = (int *)malloc((size_t)local_rows * (size_t)m * sizeof(int));
    int *dist = (int *)malloc((size_t)(local_rows + 2) * (size_t)m * sizeof(int));
    int *next_dist = (int *)malloc((size_t)(local_rows + 2) * (size_t)m * sizeof(int));
    if (!local_grid || !dist || !next_dist) {
        printf("Rank %d: memory allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Scatterv(global_grid, send_counts, displs, MPI_INT,
                 local_grid, local_rows * m, MPI_INT,
                 0, MPI_COMM_WORLD);

    for (int i = 0; i < (local_rows + 2) * m; i++) {
        dist[i] = INF;
        next_dist[i] = INF;
    }

    if (start_row == 0) {
        dist[idx(1, 0, m)] = local_grid[idx(0, 0, m)];
    }

    const int TAG_SEND_UP = 100;
    const int TAG_SEND_DOWN = 200;

    int iterations = 0;
    int max_iterations = n * m * 2;

    while (iterations < max_iterations) {
        if (rank == 0) {
            for (int j = 0; j < m; j++) dist[idx(0, j, m)] = INF;
        }
        if (rank == size - 1) {
            for (int j = 0; j < m; j++) dist[idx(local_rows + 1, j, m)] = INF;
        }

        if (size > 1) {
            if (rank % 2 == 0) {
                if (rank > 0) {
                    MPI_Send(&dist[idx(1, 0, m)], m, MPI_INT, rank - 1, TAG_SEND_UP, MPI_COMM_WORLD);
                }
                if (rank < size - 1) {
                    MPI_Send(&dist[idx(local_rows, 0, m)], m, MPI_INT, rank + 1, TAG_SEND_DOWN, MPI_COMM_WORLD);
                }
                if (rank > 0) {
                    MPI_Recv(&dist[idx(0, 0, m)], m, MPI_INT, rank - 1, TAG_SEND_DOWN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                if (rank < size - 1) {
                    MPI_Recv(&dist[idx(local_rows + 1, 0, m)], m, MPI_INT, rank + 1, TAG_SEND_UP, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            } else {
                if (rank > 0) {
                    MPI_Recv(&dist[idx(0, 0, m)], m, MPI_INT, rank - 1, TAG_SEND_DOWN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                if (rank < size - 1) {
                    MPI_Recv(&dist[idx(local_rows + 1, 0, m)], m, MPI_INT, rank + 1, TAG_SEND_UP, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                if (rank > 0) {
                    MPI_Send(&dist[idx(1, 0, m)], m, MPI_INT, rank - 1, TAG_SEND_UP, MPI_COMM_WORLD);
                }
                if (rank < size - 1) {
                    MPI_Send(&dist[idx(local_rows, 0, m)], m, MPI_INT, rank + 1, TAG_SEND_DOWN, MPI_COMM_WORLD);
                }
            }
        }

        for (int i = 0; i < (local_rows + 2) * m; i++) {
            next_dist[i] = dist[i];
        }

        int local_changed = 0;

        for (int li = 1; li <= local_rows; li++) {
            int gi = start_row + (li - 1);
            for (int j = 0; j < m; j++) {
                if (gi == 0 && j == 0) {
                    next_dist[idx(li, j, m)] = local_grid[idx(li - 1, j, m)];
                    continue;
                }

                int best = dist[idx(li, j, m)];
                int w = local_grid[idx(li - 1, j, m)];

                if (j > 0 && dist[idx(li, j - 1, m)] < INF) {
                    int cand = dist[idx(li, j - 1, m)] + w;
                    if (cand < best) best = cand;
                }
                if (j + 1 < m && dist[idx(li, j + 1, m)] < INF) {
                    int cand = dist[idx(li, j + 1, m)] + w;
                    if (cand < best) best = cand;
                }
                if (dist[idx(li - 1, j, m)] < INF) {
                    int cand = dist[idx(li - 1, j, m)] + w;
                    if (cand < best) best = cand;
                }
                if (dist[idx(li + 1, j, m)] < INF) {
                    int cand = dist[idx(li + 1, j, m)] + w;
                    if (cand < best) best = cand;
                }

                next_dist[idx(li, j, m)] = best;
                if (best != dist[idx(li, j, m)]) {
                    local_changed = 1;
                }
            }
        }

        int *tmp = dist;
        dist = next_dist;
        next_dist = tmp;

        int global_changed = 0;
        MPI_Allreduce(&local_changed, &global_changed, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);

        iterations++;
        if (!global_changed) {
            break;
        }
    }

    int local_goal = INF;
    if (start_row <= (n - 1) && (n - 1) < (start_row + local_rows)) {
        int li = (n - 1) - start_row + 1;
        local_goal = dist[idx(li, m - 1, m)];
    }

    int global_goal = INF;
    MPI_Reduce(&local_goal, &global_goal, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

    int *local_dist_out = (int *)malloc((size_t)local_rows * (size_t)m * sizeof(int));
    for (int r = 0; r < local_rows; r++) {
        for (int c = 0; c < m; c++) {
            local_dist_out[idx(r, c, m)] = dist[idx(r + 1, c, m)];
        }
    }

    int *global_dist = NULL;
    if (rank == 0) {
        global_dist = (int *)malloc((size_t)n * (size_t)m * sizeof(int));
    }

    MPI_Gatherv(local_dist_out, local_rows * m, MPI_INT,
                global_dist, send_counts, displs, MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        FILE *out = fopen(argv[2], "w");
        if (!out) {
            printf("Error opening output file\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fprintf(out, "Grid Size: %d x %d\n", n, m);
        fprintf(out, "Minimum Evacuation Cost: %d\n", global_goal);
        fprintf(out, "Iterations: %d\n", iterations);
        fprintf(out, "Processes Used: %d\n", size);

        fprintf(out, "\nDistance Matrix:\n");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                fprintf(out, "%d ", global_dist[idx(i, j, m)]);
            }
            fprintf(out, "\n");
        }

        fclose(out);

        printf("Output written to %s\n", argv[2]);
    }

    free(global_grid);
    free(send_counts);
    free(displs);
    free(local_grid);
    free(dist);
    free(next_dist);
    free(local_dist_out);
    free(global_dist);

    MPI_Finalize();
}