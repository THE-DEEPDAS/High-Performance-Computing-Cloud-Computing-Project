## Cloud Computing ma mare shu karvanu che?
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
Inputs and Outputs both on S3


