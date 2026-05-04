"use client";

import { FormEvent, useMemo, useState } from "react";

type JobStatus = "idle" | "running" | "success" | "error";

type RunResponse = {
  job_id: string;
  return_code: number;
  stdout: string;
  stderr: string;
  output_preview: string;
  input_s3?: string | null;
  output_s3?: string | null;
  s3_warning?: string | null;
};

type ParsedMetrics = {
  minimumEvacuationCost: string;
  iterations: string;
  processCount: string;
};

const STATUS_LABELS: Record<JobStatus, string> = {
  idle: "Waiting For Upload",
  running: "Running MPI Job",
  success: "Job Completed",
  error: "Job Failed",
};

const STATUS_HELP: Record<JobStatus, string> = {
  idle: "Choose a .txt grid file, then click Upload And Run MPI.",
  running: "Input is being submitted and processed. This may take a few moments.",
  success: "Results are ready and parsed below.",
  error: "The backend returned an error. Review the details section.",
};

function parseMetrics(text: string): ParsedMetrics {
  const minCostMatch = text.match(/Minimum\s+Evacuation\s+Cost:\s*([^\r\n]+)/i);
  const iterationsMatch = text.match(/Iterations:\s*([^\r\n]+)/i);
  const processCountMatch = text.match(/Processes\s+Used:\s*([^\r\n]+)/i);

  return {
    minimumEvacuationCost: minCostMatch?.[1]?.trim() || "Not found",
    iterations: iterationsMatch?.[1]?.trim() || "Not found",
    processCount: processCountMatch?.[1]?.trim() || "Not found",
  };
}

function parseErrorDetail(detail: unknown): string {
  if (typeof detail === "string") {
    return detail;
  }

  if (detail && typeof detail === "object") {
    const maybeMessage = (detail as { message?: unknown }).message;
    if (typeof maybeMessage === "string") {
      return maybeMessage;
    }

    try {
      return JSON.stringify(detail, null, 2);
    } catch {
      return "Unexpected backend error format.";
    }
  }

  return "Unexpected backend error format.";
}

export default function Home() {
  const apiBase = "http://51.20.130.46:8000";

  const [file, setFile] = useState<File | null>(null);
  const [status, setStatus] = useState<JobStatus>("idle");
  const [result, setResult] = useState<RunResponse | null>(null);
  const [error, setError] = useState<string>("");
  const [submittedAt, setSubmittedAt] = useState<string>("");

  const metrics = useMemo(() => {
    if (!result) {
      return {
        minimumEvacuationCost: "-",
        iterations: "-",
        processCount: "-",
      };
    }

    const searchableText = `${result.output_preview}\n${result.stdout}\n${result.stderr}`;
    return parseMetrics(searchableText);
  }, [result]);

  async function handleSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    if (!file) {
      setStatus("error");
      setError("Please choose a .txt file first.");
      return;
    }

    setStatus("running");
    setError("");
    setResult(null);
    setSubmittedAt(new Date().toLocaleString());

    const formData = new FormData();
    formData.append("file", file);

    try {
      const response = await fetch(`${apiBase}/run`, {
        method: "POST",
        body: formData,
      });

      const data = await response.json();

      if (!response.ok) {
        const detail = parseErrorDetail((data as { detail?: unknown }).detail);
        throw new Error(detail || "MPI job failed.");
      }

      setResult(data as RunResponse);
      setStatus("success");
    } catch (uploadError) {
      const message = uploadError instanceof Error ? uploadError.message : "Failed to run MPI job.";
      setError(message);
      setStatus("error");
    }
  }

  return (
    <div className="page-shell">
      <main className="mx-auto flex w-full max-w-6xl flex-col gap-8 px-4 py-8 sm:px-6 lg:px-10 lg:py-10">
        <section className="hero-panel">
          <p className="chip">High Performance Computing Dashboard</p>
          <h1 className="text-3xl font-bold tracking-tight sm:text-4xl">MPI Evacuation Pathfinding Runner</h1>
          <p className="max-w-3xl text-sm text-slate-700 sm:text-base">
            Upload a grid file and run distributed MPI pathfinding. Results are parsed into readable sections so
            you can quickly review key metrics.
          </p>
        </section>

        <section className="grid gap-6 lg:grid-cols-[1.2fr,0.8fr]">
          <div className="panel">
            <h2 className="section-title">Upload Input</h2>
            <form className="mt-4 flex flex-col gap-4" onSubmit={handleSubmit}>
              <label className="text-sm font-medium text-slate-700" htmlFor="grid-file">
                Grid File (.txt)
              </label>
              <input
                id="grid-file"
                type="file"
                accept=".txt"
                className="file-input"
                onChange={(event) => setFile(event.target.files?.[0] ?? null)}
              />

              <button type="submit" className="action-btn" disabled={status === "running"}>
                {status === "running" ? "Running..." : "Upload And Run MPI"}
              </button>

              <div className="rounded-xl border border-slate-200 bg-slate-50 p-3 text-sm text-slate-700">
                <p>
                  <strong>Selected file:</strong> {file?.name ?? "None"}
                </p>
                {submittedAt && (
                  <p>
                    <strong>Last submitted:</strong> {submittedAt}
                  </p>
                )}
              </div>
            </form>
          </div>

          <div className="panel">
            <h2 className="section-title">Job Status</h2>
            <div className="mt-4 space-y-3">
              <span className={`status-pill status-${status}`}>{STATUS_LABELS[status]}</span>
              <p className="text-sm text-slate-700">{STATUS_HELP[status]}</p>
              {status === "running" && (
                <div className="h-2 overflow-hidden rounded-full bg-slate-200">
                  <div className="progress-bar h-full w-2/3" />
                </div>
              )}
              {error && <p className="rounded-lg bg-red-50 p-3 text-sm text-red-700">{error}</p>}
            </div>
          </div>
        </section>

        <section className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
          <article className="metric-card">
            <p className="metric-label">Minimum Evacuation Cost</p>
            <p className="metric-value">{metrics.minimumEvacuationCost}</p>
          </article>
          <article className="metric-card">
            <p className="metric-label">Iterations</p>
            <p className="metric-value">{metrics.iterations}</p>
          </article>
          <article className="metric-card">
            <p className="metric-label">Process Count</p>
            <p className="metric-value">{metrics.processCount}</p>
          </article>
          <article className="metric-card">
            <p className="metric-label">Return Code</p>
            <p className="metric-value">{result ? String(result.return_code) : "-"}</p>
          </article>
        </section>

        <section className="panel">
          <h2 className="section-title">Output Preview</h2>
          <pre className="output-box mt-4">{result?.output_preview || "Run a job to see parsed output preview here."}</pre>
        </section>

        <section className="grid gap-6 lg:grid-cols-2">
          <article className="panel">
            <h2 className="section-title">Execution Details</h2>
            <dl className="mt-4 grid grid-cols-1 gap-3 text-sm text-slate-700 sm:grid-cols-2">
              <div>
                <dt className="font-semibold text-slate-900">Job ID</dt>
                <dd className="break-all">{result?.job_id ?? "-"}</dd>
              </div>
              <div>
                <dt className="font-semibold text-slate-900">Input Archive (S3)</dt>
                <dd className="break-all">{result?.input_s3 || "Not uploaded"}</dd>
              </div>
              <div>
                <dt className="font-semibold text-slate-900">Output Archive (S3)</dt>
                <dd className="break-all">{result?.output_s3 || "Not uploaded"}</dd>
              </div>
              <div>
                <dt className="font-semibold text-slate-900">S3 Warning</dt>
                <dd className="break-all">{result?.s3_warning || "None"}</dd>
              </div>
            </dl>

            <h3 className="mt-5 text-sm font-semibold text-slate-900">STDOUT</h3>
            <pre className="output-mini mt-2">{result?.stdout || "-"}</pre>

            <h3 className="mt-5 text-sm font-semibold text-slate-900">STDERR</h3>
            <pre className="output-mini mt-2">{result?.stderr || "-"}</pre>
          </article>

          <article className="panel">
            <h2 className="section-title">Raw JSON (Optional)</h2>
            <p className="mt-2 text-sm text-slate-700">The structured cards above are easier to read; this block is for debugging.</p>
            <details className="mt-4 rounded-xl border border-slate-200 bg-slate-50 p-3">
              <summary className="cursor-pointer text-sm font-semibold text-slate-900">Show Full API Response</summary>
              <pre className="output-mini mt-3">{result ? JSON.stringify(result, null, 2) : "No response yet."}</pre>
            </details>
          </article>
        </section>
      </main>
    </div>
  );
}
