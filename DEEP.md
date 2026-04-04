## Cloud Computing ma mare shu karvanu che?
✅ STEP 5 — Build API

✔ Accept:

Grid file

✔ Trigger:

mpirun -np 4 ./phase2 input.txt output.txt

✔ Return:

Output result

👉 Deliverable:

app.py
✅ STEP 6 — Local Integration

Test:

API → MPI → Output file
✅ STEP 7 — Deploy on AWS
You MUST do:
1. Launch EC2
Ubuntu
2. Install:
openmpi
gcc
python3
fastapi
3. Upload your code
4. Run API:
uvicorn app:app --host 0.0.0.0 --port 8000

👉 Deliverable:

Working API endpoint:
http://<EC2-IP>:8000/run
✅ STEP 8 — Run MPI on Cloud

✔ Ensure:

mpirun -np 4 ./phase2

works on EC2

✅ STEP 9 — Output Storage

Store:

Inputs
Outputs

Locally OR S3 (optional)

------- 

mpicc phase2_mpi.c -o phase2
mpirun -np 4 ./phase2 big_grid_0.txt output_phase2.txt

phase 2 no code run karvano baaki che