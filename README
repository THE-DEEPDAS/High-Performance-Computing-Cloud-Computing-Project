# HPC + Cloud Project (MPI Pathfinding + API + AWS S3)

## What has been implemented

### 1) MPI upgraded to distributed pathfinding
The file [MPI_Part.c](MPI_Part.c) is no longer doing a simple grid sum. It now performs distributed shortest-path style computation for evacuation cost.

Implemented features:
- Row-wise domain decomposition across MPI ranks
- Uneven row handling (`n % size`) support
- Ghost rows (top + bottom) per process
- Boundary exchange using `MPI_Send` / `MPI_Recv`
- Iterative relaxation over 4-neighbor grid (up/down/left/right)
- Global convergence detection using `MPI_Allreduce`
- Final minimum evacuation cost reduction at root rank
- Optional gathered distance matrix output

Output now includes:
- Grid Size
- Minimum Evacuation Cost
- Iterations
- Processes Used
- Distance Matrix

---

### 2) Backend API created
The file [app.py](app.py) exposes:
- `GET /` → serves test frontend
- `GET /health` → basic health check
- `POST /run` → accepts `.txt`, runs MPI, uploads I/O to S3, returns result JSON

Backend behavior for `POST /run`:
1. Accept uploaded grid `.txt`
2. Save input to local temp (`/tmp/mpi_jobs` by default)
3. Run MPI command (`mpirun -np <N> phase2 input output`)
4. Auto-handle slot constraints with `--oversubscribe` support
5. Read output file
6. Upload input and output files to S3
7. Return `job_id`, execution logs, preview, and S3 paths

---

### 3) Small frontend added for quick testing
The file [frontend/index.html](frontend/index.html) provides a simple UI:
- upload `.txt`
- click **Run MPI**
- view JSON response in browser

---
### 4) Environment/dependencies prepared
The file [requirements.txt](requirements.txt) includes:
- `fastapi`
- `uvicorn[standard]`
- `python-multipart`
- `boto3`
- `python-dotenv`

The file [.env](.env) controls runtime variables.

Recommended values:
- `AWS_REGION=ap-south-1`
- `S3_BUCKET=<your-bucket-name>`
- `MPI_BINARY=./phase2`
- `MPI_PROCS=2` (or 4 depending on instance capacity)
- `MPI_OVERSUBSCRIBE=true`
- `LOCAL_WORK_DIR=/tmp/mpi_jobs`

> Note: Prefer `MPI_BINARY=./phase2` (or absolute path). Avoid fragile relative paths from home directory.

---

## How to run locally (Linux/EC2)

### Compile MPI
`mpicc MPI_Part.c -o phase2`

### Test MPI directly
`mpirun --oversubscribe -np 2 ./phase2 Dataset/grid_0.txt output.txt`

### Start API
`uvicorn app:app --host 0.0.0.0 --port 8000`

### Test endpoints
- Browser UI: `http://<HOST>:8000/`
- Health: `http://<HOST>:8000/health`
- MPI API: `POST http://<HOST>:8000/run` with `file=@grid.txt`

---

## AWS deployment summary

1. Launch EC2 (Ubuntu)
2. Install OpenMPI, GCC, Python, pip
3. Upload project files
4. Create/activate virtual environment
5. Install dependencies from [requirements.txt](requirements.txt)
6. Compile `phase2`
7. Set `.env`
8. Ensure EC2 IAM role has S3 permissions:
	- `s3:PutObject`
	- `s3:GetObject`
	- `s3:ListBucket`
9. Open Security Group port `8000`
10. Run Uvicorn and test upload flow from browser

---

## Main deliverables in this repo
- [MPI_Part.c](MPI_Part.c) — distributed MPI pathfinding
- [app.py](app.py) — FastAPI backend
- [frontend/index.html](frontend/index.html) — test frontend
- [requirements.txt](requirements.txt) — Python dependencies
- [MPI_Distributed_Pathfinding_Notes.md](MPI_Distributed_Pathfinding_Notes.md) — technical explanation