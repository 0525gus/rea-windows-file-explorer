# Common external native-UI comparison — original and B

**Both apps were observed at the correct bulk path with 1,200-item status after the same Enter + five F5 actions. This establishes a common external task observation, not an internal listing-speed comparison.** B warm refreshes 1/2/4/5 already displayed 1200 on their first poll with no observed loading transition; their subsecond checks do not prove a fresh refresh completed.

The [protocol](../../verification/benchmarks/external-protocol.json), SHA-256 `5eb2d805ea334b971aa52f09c89d353b2c549c9de6805b0dc704b2edc3559384`, declared the procedure before execution. Its recorded flag and file mtime precede both launch timestamps (mtime is not a tamper-proof audit clock). The shared fixture manifest SHA-256 is `278d49240d795cabcd15be949cdc4f235722a0c8d92236854c503efc2dd14bc1`. Both retained preparations show the same empty path and 0 items, **Details**, maximized **2560×1392** outer window. Original and B run sequentially as fresh owned processes. The protocol specifies the same `sky.press_key` + `get_window_state(include_screenshot=false, include_text=true)` path; status text is retained, no timed screenshots. UI trees/providers and content/layout differ.

[Structured report](phase6-external-comparison.json) records every input SHA-256, executable digest, sample pointer, poll, first qualifying status and final repeated check. This report author read existing files only and did not execute any app/UI action or alter initial files.

| External action | Original first 1200+bulk status (s) | B first 1200+bulk status (s) | B last recorded check (s) | B observed transition |
|---|---:|---:|---:|---|
| Enter navigation | 14.925 | 11.511 | 11.511 | Loading → ready |
| F5 1 | 11.395 | 0.292 | 11.492 | already 1200; generation unknown |
| F5 2 | 7.951 | 0.276 | 7.393 | already 1200; generation unknown |
| F5 3 | 8.315 | 6.428 | 6.428 | Loading → ready |
| F5 4 | 8.689 | 0.097 | 5.010 | already 1200; generation unknown |
| F5 5 | 8.132 | 0.276 | 6.316 | already 1200; generation unknown |

These durations include action/control overhead, UIA-provider cost, model/tool scheduling and polling delays. Original had a nonqualifying/absent status poll before each final ready-status observation; its first navigation also exposed 0 items at the bulk path before 1200. B's initial navigation and F5 3 exposed **Loading** before ready. B's other four F5 rows exposed 1200 immediately and again later. Their raw `elapsed_upper_bound_ms` is the **last** repeated check, not the first ready-status time and not a proven generation-completion bound. The report preserves both numbers instead of substituting one for the other. A retained details-pane 1200 count alongside Loading is not counted as ready.

No internal generation timestamp, completed-paint time, heartbeat or full-name oracle was observed in this trial. Even an observed Loading→ready status transition does not reveal those internal times. **No speed ratio, “0.1 s listing” claim, or REA causal conclusion is justified.** Original-first ordering and unflushed OS cache further limit inference. There is one trial per app.

## Whole owned-process resources

| App | Job wall (s) | Family CPU (s) | Peak family committed (bytes) | Exit / active / residual owned |
|---|---:|---:|---:|---|
| Original | 189.833273 | 122.250000 | 70504448 | 0 / 0 / 0 |
| B | 179.392269 | 50.046875 | 35995648 | 0 / 0 / 0 |

[Original metrics](measurements/original-external-common.metrics.json) start `2026-10-22T09:32:20.017255+09:00`; [B metrics](measurements/b-external-common.metrics.json) start `2026-10-22T09:35:30.357479+09:00`. Each records one owned app-family process, exit 0 and no active/residual owned process at completion, consistent with the root's normal close. Peak is the Job Object's simultaneous **committed memory**, not RSS. The measured process lifetime covers startup, view/maximize/address setup, UIA-driven waits, agent/tool delays and teardown; Job CPU includes owned app/provider work, while separate CUA-client/tool/model CPU is unmeasured; they are **not six-listing-only resources**, and differences are not an efficiency or REA-effect estimate.

Preparation differs according to root's execution handoff and retained interruption fields: original Details required two clicks and user input interrupted preparation before measurement, after which state was refreshed; B's maximize accessibility attempt failed, followed by a fresh preparation screenshot/titlebar double-click. Timed observations then used the declared common path. These setup events belong in whole-process cost, not in a product refresh-failure classification. Final 1200 status does not verify every filename/metadata or completed rendering.

Validation checked six samples per app, Enter/F5 order and ordinals, Details/dimensions, initial empty path/zero status, final bulk path/1200 without Loading, monotonic poll elapsed, UTC/elapsed agreement within 3 ms, raw last-poll values, protocol/fixture digests, CPU user+kernel arithmetic, sequential process lifetimes, and exit/cleanup metrics. The [original observations](measurements/original-external-common-observations.json) and [B observations](measurements/b-external-common-observations.json) remain authoritative. Source/metric/executable hashes are in the structured report; no silent replacement or new measurement was performed.
