# MPI Distributed Pathfinding — Technical Notes (Simple Version)

This document explains what the updated `MPI_Part.c` now does and why.

## 1) Goal

The program computes the **minimum evacuation/path cost** from:
- start cell `(0,0)`
- to destination cell `(n-1,m-1)`

using distributed computation across MPI processes.

---

## 2) High-level design

Each MPI process owns a **contiguous chunk of rows** from the full grid.

The code does:
1. Root process reads input grid.
2. Grid is split row-wise and scattered to all ranks.
3. Every rank keeps:
   - local cell weights (`local_grid`)
   - local distance state (`dist`)
   - one ghost row above + one ghost row below.
4. Ranks iteratively relax distances.
5. Neighbor boundary rows are exchanged each iteration.
6. Iterations stop when no rank changes any value.
7. Final minimum cost is reduced to root and written to output.

---

## 3) Domain decomposition (row-wise)

Two helper functions are used:
- `rows_for_rank(n,size,rank)`
- `start_row_for_rank(n,size,rank)`

This supports both cases:
- `n` divisible by `size`
- `n` not divisible by `size` (extra rows distributed to first ranks)

`MPI_Scatterv` is used so each rank can receive a different row count.

---

## 4) Ghost cells (boundary rows)

Each rank stores distances with shape:
- `(local_rows + 2) x m`

where:
- row `0` is top ghost row
- rows `1..local_rows` are real local rows
- row `local_rows+1` is bottom ghost row

Ghost rows hold neighbor boundary distances from adjacent ranks.

---

## 5) Communication model

Per iteration, ranks exchange boundary rows with neighbors:
- send first real row upward
- send last real row downward
- receive into top and bottom ghost rows

Communication uses explicit:
- `MPI_Send`
- `MPI_Recv`

An even/odd ordering is used to avoid deadlock.

---

## 6) Relaxation algorithm

This is an iterative shortest-path relaxation over a 4-neighbor grid:
- left
- right
- up
- down

For each local cell:
- candidate cost = neighbor distance + current cell weight
- keep the minimum candidate

The source is initialized as:
- `dist(0,0) = grid(0,0)`

All others start at `INF`.

This is effectively a distributed Bellman-Ford style relaxation on the grid graph.

---

## 7) Convergence detection

Each rank tracks `local_changed` in an iteration.

`MPI_Allreduce(..., MPI_LOR, ...)` combines all ranks into `global_changed`.
- If `global_changed == 0`, algorithm converged and loop stops.
- There is also a safety cap (`max_iterations`) to avoid infinite looping.

---

## 8) Final result and output

- Rank owning destination cell computes local destination distance.
- `MPI_Reduce(..., MPI_MIN, root)` obtains global minimum cost at rank 0.

Root writes:
- grid size
- minimum evacuation cost
- iteration count
- process count
- optional full distance matrix (gathered with `MPI_Gatherv`)

---

## 9) Why this matches your DEEP requirements

Your DEEP checklist requested:
- row-wise domain decomposition ✅
- ghost cells ✅
- `MPI_Send` / `MPI_Recv` boundary exchange ✅
- iterative relaxation until convergence ✅
- output minimum evacuation cost ✅

So this implementation is aligned with the required “Distributed Pathfinding” phase.

---

## 10) Practical note

On your current machine, `mpicc` was not found, so compile/run could not be validated in terminal yet.

Once MPI is installed/configured, expected commands are:
- compile: `mpicc MPI_Part.c -o phase2`
- run: `mpirun -np 4 ./phase2 Dataset/big_grid_0.txt Outputs/grid_0_output.txt`
