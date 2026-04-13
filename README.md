# HPC + Cloud Project: Distributed MPI Pathfinding Service

This repository implements an end-to-end High Performance Computing plus Cloud workflow:

1. Build weighted grid workloads.
2. Solve minimum evacuation cost using distributed-memory parallelism (MPI).
3. Expose the HPC solver as an API service.
4. Archive input and output artifacts to AWS S3.
5. Provide browser-based UI clients to run jobs and inspect results.

The project is intended for viva-level discussion across algorithms, parallel systems, and cloud integration.

---

## 1) Problem Statement

Given an n x m weighted grid, compute the minimum cumulative cost from source cell (0,0) to destination cell (n-1,m-1), where movement is restricted to 4-neighbors:

- up
- down
- left
- right

Interpretation:

- Each cell is a graph node.
- Each move to a neighbor adds the destination cell weight to path cost.
- Source starts with cost equal to grid[0][0].

Output includes:

- Grid size
- Minimum evacuation cost
- Number of iterations used by distributed relaxation
- Number of MPI processes
- Final distance matrix

---

## 2) Architecture Overview

Core files:

- [MPI_Part.c](MPI_Part.c): Distributed MPI solver (main HPC component).
- [Djikstra.c](Djikstra.c): Serial baseline implementation.
- [app.py](app.py): FastAPI orchestration layer that runs MPI jobs.
- [frontend/index.html](frontend/index.html): Basic upload-and-run browser UI.
- [client/app/page.tsx](client/app/page.tsx): Rich Next.js dashboard UI.
- [generate_5_grids.c](generate_5_grids.c): Generates base random grids.
- [expand_grids.c](expand_grids.c): Expands and perturbs base grids into larger datasets.

Pipeline:

1. User uploads a .txt grid.
2. API stores file to local working directory.
3. API launches mpirun with configured process count.
4. MPI program computes distributed shortest path cost.
5. API reads output file and optionally uploads input/output to S3.
6. API returns JSON response with logs, preview, job id, and S3 paths.

---

## 3) HPC Stack and Libraries

### 3.1 Core HPC libraries used

1. MPI (Message Passing Interface)
	- Header: mpi.h
	- Runtime/compiler expected: OpenMPI or MPICH family
	- Used for process management, data distribution, neighbor communication, collectives, reduction, and gather.

2. C standard libraries in solver
	- stdio.h for file I/O and console output
	- stdlib.h for dynamic allocation and process termination on failures

### 3.2 Backend/cloud libraries used

From [requirements.txt](requirements.txt):

- fastapi
- uvicorn[standard]
- python-multipart
- boto3
- python-dotenv

Purpose:

- FastAPI/Uvicorn: API hosting.
- python-multipart: file upload handling.
- boto3: S3 archival operations.
- dotenv: environment-based configuration.

---

## 4) Distributed MPI Solver: Very Detailed Walkthrough

Reference implementation: [MPI_Part.c](MPI_Part.c)

### 4.1 Program phases

Phase A: MPI startup and rank metadata

- MPI_Init initializes MPI runtime.
- MPI_Comm_rank gets current process rank.
- MPI_Comm_size gets total process count.

Phase B: Input and global metadata propagation

- Rank 0 reads grid from file.
- MPI_Bcast sends n and m to all ranks.

Phase C: Decomposition planning

- rows_for_rank computes local row count with remainder-aware load balancing.
- start_row_for_rank computes each rank global start row.
- send_counts and displs are prepared for variable-sized scatter/gather.

Phase D: Data distribution

- MPI_Scatterv distributes row blocks from global_grid to local_grid.
- Scatterv is essential because row counts may differ across ranks.

Phase E: Local memory model setup

Each rank allocates:

- local_grid: local_rows x m (cell weights)
- dist: (local_rows + 2) x m
- next_dist: (local_rows + 2) x m

The extra 2 rows are ghost rows:

- ghost top row at index 0
- ghost bottom row at index local_rows + 1

Why ghost rows matter:

- Boundary cells need neighbor distances owned by adjacent ranks.
- Ghost rows store these imported boundary values each iteration.

Phase F: Iterative relaxation loop

Loop condition:

- while iterations < max_iterations

Per iteration steps:

1. Apply physical boundary conditions (first and last rank ghost boundaries set to INF).
2. Exchange boundary rows with neighboring ranks.
3. Copy dist to next_dist baseline.
4. Relax each local cell using 4-neighbor candidate costs.
5. Swap dist and next_dist pointers.
6. Global convergence check via MPI_Allreduce on local_changed.
7. Stop if no rank changed any value.

Phase G: Final result assembly

- Rank containing destination contributes its local goal value.
- MPI_Reduce with MPI_MIN gives global goal at root.
- MPI_Gatherv reconstructs full distance matrix at root.
- Root writes final report to output file.

Phase H: Cleanup and shutdown

- Free all allocated buffers.
- MPI_Finalize terminates MPI runtime.

---

## 5) MPI Call-by-Call Explanation

This section is useful for viva answers about why each call exists.

1. MPI_Init
	- Initializes MPI execution context.
	- Must be called before almost all MPI operations.

2. MPI_Comm_rank
	- Retrieves process identity (rank).
	- Used for role-based logic (root-only I/O, neighbor selection, etc.).

3. MPI_Comm_size
	- Retrieves world size (number of ranks).
	- Required for decomposition and process-level loops.

4. MPI_Bcast
	- Broadcasts scalar metadata (n, m) from root to all ranks.
	- Ensures every rank knows global dimensions before allocation.

5. MPI_Scatterv
	- Distributes variable-length blocks from root to ranks.
	- Needed because decomposition can be uneven.

6. MPI_Send
	- Sends local boundary row to neighbor rank.
	- Used for ghost-row update.

7. MPI_Recv
	- Receives neighbor boundary row into ghost row.
	- Completes boundary exchange for cross-partition dependencies.

8. MPI_Allreduce with MPI_LOR
	- Combines local_changed flags into global_changed.
	- Logical OR means if any rank changed, continue iterating.
	- Allreduce returns result to all ranks, so everyone can stop consistently.

9. MPI_Reduce with MPI_MIN
	- Reduces candidate destination costs to single global minimum on root.

10. MPI_Gatherv
	- Reassembles local distance blocks into full global distance matrix at root.
	- Variable-sized gather to match uneven decomposition.

11. MPI_Abort
	- Immediate global termination on unrecoverable errors (for example file open or allocation failures).

12. MPI_Finalize
	- Final cleanup for MPI runtime.

---

## 6) Why Even/Odd Ordering Is Used

Neighbor exchanges use blocking MPI_Send and MPI_Recv.

Potential issue:

- If every rank sends first and no receives are posted, deadlock can occur.

Current solution:

- Even ranks: send then receive.
- Odd ranks: receive then send.

This ordering guarantees matching receive opportunities and prevents circular wait for this communication pattern.

---

## 7) Algorithmic Notes and Correctness Intuition

The solver performs repeated local relaxations until no value changes globally.

Key properties:

- Distances start at INF except source.
- Update rule only decreases or keeps values.
- Lower bounded by 0 if weights are non-negative.
- Therefore sequence converges to a fixed point.

On this 4-neighbor weighted grid, the converged fixed point corresponds to shortest path costs from source to all reachable cells.

---

## 8) Performance and Scaling Discussion

Per-rank compute per iteration:

- O(local_rows x m)

Per-rank communication per iteration:

- Two row exchanges with neighbors, each of length m
- Approximate communication volume O(m)

Overall time depends on:

- Number of iterations to convergence
- Process count
- Network and MPI runtime overhead
- Load balance (especially when n not divisible by size)

Important scaling tradeoff:

- More ranks reduce local compute but increase synchronization and communication overhead.

---

## 9) Serial Baseline (for Comparison)

Reference file: [Djikstra.c](Djikstra.c)

Purpose:

- Provides non-distributed baseline for correctness/performance comparison.

Approach:

- Uses minDist scan over all unvisited nodes and relaxes 4-neighbors.
- Writes minimum cost and distance matrix.

---

## 10) Dataset Generation Tools

### 10.1 Base grid generation

File: [generate_5_grids.c](generate_5_grids.c)

- Produces 5 random grids of size 200 x 200.
- Cell values sampled from 1..100.

### 10.2 Expanded grid generation

File: [expand_grids.c](expand_grids.c)

- Reads one of base grids.
- Tiles it to 500 x 500.
- Adds random noise (rand() % 20).
- Clamps values to max 100.
- Generates 20 larger files.

These utilities create controlled but varied workloads for HPC testing.

---

## 11) API Layer Details

Reference: [app.py](app.py)

Endpoints:

- GET /
  - Serves static HTML test page.
- GET /health
  - Returns status and UTC timestamp.
- POST /run
  - Accepts uploaded .txt grid file.
  - Runs MPI binary via mpirun.
  - Reads output.
  - Optionally uploads input/output to S3.
  - Returns structured JSON response.

Robustness features:

- File type validation for .txt uploads.
- MPI binary existence check.
- Timeout handling for long jobs.
- Optional retry with --oversubscribe on slot error.
- S3 failure isolation: compute result still returned with warning.

---

## 12) UI Clients

1. Basic static client
	- [frontend/index.html](frontend/index.html)
	- Upload and run workflow for quick testing.

2. Next.js dashboard
	- [client/app/page.tsx](client/app/page.tsx)
	- Parses metrics from output text.
	- Displays status, logs, key values, and raw JSON.

---

## 13) Configuration and Environment

Runtime variables are in [.env](.env):

- AWS_REGION
- S3_BUCKET
- MPI_BINARY
- MPI_PROCS
- MPI_OVERSUBSCRIBE
- LOCAL_WORK_DIR

Current sample values:

- AWS_REGION=ap-south-1
- S3_BUCKET=cc-hpc-bucket
- MPI_BINARY=./phase2
- MPI_PROCS=2
- MPI_OVERSUBSCRIBE=true
- LOCAL_WORK_DIR=/tmp/mpi_jobs

---

## 14) Build and Run

Compile MPI solver:

mpicc MPI_Part.c -o phase2

Run MPI directly:

mpirun --oversubscribe -np 2 ./phase2 Dataset/grid_0.txt Outputs/grid_0_output.txt

Run API server:

uvicorn app:app --host 0.0.0.0 --port 8000

Open:

- http://localhost:8000/
- http://localhost:8000/health

---

## 15) AWS Deployment Notes

1. Launch Ubuntu EC2 instance.
2. Install OpenMPI, GCC, Python, pip.
3. Upload project files.
4. Create and activate virtual environment.
5. Install Python dependencies.
6. Compile MPI binary phase2.
7. Configure environment variables.
8. Attach IAM role with S3 permissions (PutObject, GetObject, ListBucket).
9. Open port 8000 in Security Group.
10. Run uvicorn and test upload flow.

---

## 16) Viva-Focused Talking Points

If asked What is the HPC contribution:

- Distributed-memory decomposition with MPI.
- Ghost-boundary communication for cross-rank dependency propagation.
- Global convergence and reduction via collectives.

If asked Which HPC calls/libraries are used and why:

- MPI library via mpi.h.
- MPI_Init, MPI_Comm_rank, MPI_Comm_size for process setup.
- MPI_Bcast for metadata distribution.
- MPI_Scatterv and MPI_Gatherv for uneven row partition handling.
- MPI_Send and MPI_Recv for boundary exchange.
- MPI_Allreduce for global convergence detection.
- MPI_Reduce for final global minimum extraction.
- MPI_Finalize for shutdown.

If asked Why this is a cloud-integrated HPC system and not only HPC:

- Solver is exposed as API.
- Jobs are upload-driven from web client.
- Artifacts are archived in S3 for traceability and reproducibility.

---

## 17) Main Deliverables

- [MPI_Part.c](MPI_Part.c): Distributed pathfinding solver.
- [Djikstra.c](Djikstra.c): Serial baseline implementation.
- [app.py](app.py): API runner and cloud archival orchestrator.
- [frontend/index.html](frontend/index.html): Minimal browser test client.
- [client/app/page.tsx](client/app/page.tsx): Full dashboard client.
- [generate_5_grids.c](generate_5_grids.c): Base dataset creation.
- [expand_grids.c](expand_grids.c): Large dataset creation.
- [MPI_Distributed_Pathfinding_Notes.md](MPI_Distributed_Pathfinding_Notes.md): Technical note companion.