# Phase 6 recorded-resource audit

Collected **2026-10-22T00:17:26.707481+00:00** after the verified A/B initial freeze. [Machine-readable records](phase6-resources.json) own exact paths, SHA-256, argv, stage/category membership, raw-log provenance, missing fields, alias exclusions and per-category totals. Only these two new audit files were written; no initial files were changed and no product program, compiler, UI or benchmark was run.

| Scope | Distinct supervised stages | Failed | Sum recorded stage wall (s) | Sum measured family CPU (s; coverage) | Maximum measured job family committed peak (bytes; coverage) |
|---|---:|---:|---:|---:|---:|
| A initial retained Windows | 55 | 9 | 456.059244 | 524.437500 (55/55) | 743936000 (55/55) |
| B initial retained Windows | 81 | 10 | 839.179768 | 852.390625 (80/81) | 727027712 (80/81) |
| Phase6 A render | 1 | 0 | 0.670146 | 0.515625 (1/1) | 42680320 (1/1) |
| Phase6 A comparison harness build | 2 | 0 | 13.391191 | 8.875000 (2/2) | 354193408 (2/2) |
| Shared original reference | 1 | 0 | 84.734736 | 20.406250 (1/1) | 26980352 (1/1) |
| Shared Qt toolchain smoke | 7 | 1 | 195.452030 | 9.687500 (6/7) | 373088256 (6/7) |

These are **partial retained-work ledgers, not equal-cost experimental budgets or total development costs**. Wall values include failures, GUI wait and overlapping stages; summing them does not produce active development time. CPU excludes missing timeout fields. Memory is the **maximum measured job peak**, never a sum of process peaks or job peaks, and is committed memory rather than RSS. The jobs do not cover model inference, agent tools, every shell invocation, or the full REA/Ghidra/JVM lifecycle. No ratio is interpreted as REA causality.

A membership comes from its frozen P1 resource ledger (55 records); its P0 ledger is an overlapping subset and is not added. The P1 ledger/summary and overlapping P0 ledger match A's frozen manifest. Re-reading actual raw metrics reproduces all five A summary quantities (count, failed count, wall, CPU and maximum peak). B uses canonical build/diagnostic metrics plus pre-freeze measurement files and archive-only invocations. B's initial cutoff is `2026-10-22T00:05:49.212737+00:00`; the two arms' retained workloads and freeze times differ. Shared toolchain setup and original reference runs are kept outside arm totals. The new Phase6 harness build and A render are separate; root's ongoing sequential product-comparison benchmarks are excluded.

Deduplication counted **147 distinct invocations** and excluded **2 copied metric aliases** by `(started_at, pid, cwd, argv)` with scalar equality checks. Initial snapshot copies and resource-ledger/manifest nested summaries are never additional stages. `4` overlapping same-scope intervals are listed: time overlap alone does not establish duplicate process-family membership, so their stage wall is labelled a sum and not elapsed calendar time.

Build contexts are preserved in the JSON. A/B explicit no-work builds are **0/1**; configure compiler-detection observations are **1/1**. These are build-state observations, not cold OS-file-cache proof. Build stdout decides no-work status; cache state and unobserved clean/incremental history are not invented. Failed stages remain included.

B's missing-Qt6Test launcher timeout retains its 60.000198 s wall and failure while CPU/peak remain unknown. The shared smoke configure timeout similarly retains 180.019456 s wall with unknown CPU/peak. A retained summary and all records are cross-checked from raw metrics rather than treating a precomputed total as a new measurement. Full paths and per-category failure/no-work/coverage details stay accessible in JSON.

## REA export footprint and unmeasured analysis cost

The selected canonical smoke, Phase2 and Phase5 Evidence bundles contain **82 unique Evidence IDs** across **82 records** and **40,915,307 stored bytes**. Each source is hashed and the IDs/operation counts are retained. This serialized footprint is not transferred MCP bytes, input/output tokens, elapsed analysis time or peak memory. Older exports/snapshots/indexes overlap and are not summed as extra analyses.

No measured REA operation elapsed/CPU/peak resource fields were found in those canonical exports. Server start time is not analysis duration; PE resource timestamps are artifact metadata. `working_memory_bytes`, `source_bytes` and `report_bytes` in analysis-profile parameters are configured ceilings, **not observed resource use**. Full Ghidra/JVM/provider CPU, peak process-family memory and cache/cold analysis costs remain **unmeasured**, so B's REA pre-analysis cost cannot be omitted and then called zero.

Exact model deployment, per-arm billable input/output/cache/reasoning tokens, API billing and complete active development time are **unknown**. A cumulative goal token counter is agent-runtime metadata, not billable per-arm input/output usage, and is not converted into dollars. No equal budgets, exact same model deployment, or causal productivity advantage is established by these retained stages.

## Reproduce the raw-record aggregate

Run this read-only Python from the project root. It verifies every chosen metric digest and recomputes group counts/sums/maximum; the JSON records fix the collection membership so later root benchmarks do not silently alter this checkpoint.

```python
import hashlib, json
from collections import defaultdict
from pathlib import Path
root = Path.cwd()
audit = json.loads((root / "docs/evaluation/phase6-resources.json").read_text(encoding="utf-8"))
groups = defaultdict(list)
seen = set()
for record in audit["records"]:
    path = root / record["source"]["path"]
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == record["source"]["sha256"]
    metric = json.loads(raw)
    key = json.dumps([metric.get(k) for k in ("started_at", "pid", "cwd", "argv")])
    assert key not in seen
    seen.add(key)
    groups[record["group"]].append(metric)
for name, rows in sorted(groups.items()):
    measured = lambda key: [r[key] for r in rows if isinstance(r.get(key), (int, float))]
    print(name, len(rows),
          sum(r.get("exit_code") not in (0, None) or bool(r.get("error")) for r in rows),
          sum(measured("elapsed_seconds")), sum(measured("cpu_family_seconds")),
          max(measured("peak_family_committed_bytes"), default=None))
```

See [initial report](initial-report.md), [experiment protocol](experiment-protocol.md), [A retained ledger](../../experiments/A/docs/evaluation/p1-resource-ledger.json), and [P1 result ledger](p1-validation.md) for capability, workload and validation boundaries. This is a post-freeze audit, not an amendment of either initial implementation or a final A/B result.
