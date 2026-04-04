#include <stdio.h>
#include <stdlib.h>

#define INF 1000000000
#define MAX 600

int grid[MAX][MAX];
int dist[MAX][MAX];
int visited[MAX][MAX];

int dx[4] = {-1, 1, 0, 0};
int dy[4] = {0, 0, -1, 1};

int minDist(int n, int m, int *x, int *y) {
    int min = INF;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (!visited[i][j] && dist[i][j] < min) {
                min = dist[i][j];
                *x = i;
                *y = j;
            }
        }
    }
    return min;
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("Usage: %s input.txt output.txt\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Error opening input file\n");
        return 1;
    }

    int n, m;
    fscanf(fp, "%d %d", &n, &m);

    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            fscanf(fp, "%d", &grid[i][j]);

    fclose(fp);

    // initialize
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            dist[i][j] = INF;
            visited[i][j] = 0;
        }

    dist[0][0] = grid[0][0];

    for (int k = 0; k < n * m; k++) {
        int x = -1, y = -1;
        minDist(n, m, &x, &y);

        if (x == -1) break;

        visited[x][y] = 1;

        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];

            if (nx >= 0 && ny >= 0 && nx < n && ny < m) {
                if (dist[nx][ny] > dist[x][y] + grid[nx][ny]) {
                    dist[nx][ny] = dist[x][y] + grid[nx][ny];
                }
            }
        }
    }

    FILE *out = fopen(argv[2], "w");

    fprintf(out, "Grid Size: %d x %d\n", n, m);
    fprintf(out, "Minimum Evacuation Cost: %d\n", dist[n-1][m-1]);

    // save full distance matrix (useful for analysis)
    fprintf(out, "\nDistance Matrix:\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            fprintf(out, "%d ", dist[i][j]);
        }
        fprintf(out, "\n");
    }

    fclose(out);

    printf("Output written to %s\n", argv[2]);

    return 0;
}