"""Recheck frozen evidence and summarize post-freeze measurements, read-only inputs.

This never imports source from the original project or changes either product.
The report is validation output, not a replacement for the immutable initial ledger.
"""
from collections import Counter
from pathlib import Path
import datetime
import hashlib
import json
import statistics


ROOT = Path(__file__).resolve().parent.parent


def load(name):
    return json.loads((ROOT / name).read_text(encoding="utf-8-sig"))


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def bound(base, relative):
    path = (base / relative.replace("\\", "/")).resolve()
    if not path.is_relative_to(base.resolve()):
        raise ValueError("Manifest path escapes its root")
    return path


def check_snapshot(manifest, base):
    for item in manifest["files"]:
        path = bound(base, item["path"])
        assert path.stat().st_size == item["bytes"], path
        assert sha(path) == item["sha256"], path
    return len(manifest["files"])


def benchmark(arm):
    relative = f"docs/evaluation/measurements/{arm}-phase6-common.json"
    data = load(relative)
    metrics = load(relative.replace(".json", ".metrics.json"))
    assert metrics["exit_code"] == 0
    assert metrics["active_family_processes_at_completion"] == 0
    assert len(data["samples"]) == 6
    for item in data["samples"]:
        assert item["count"] == 1200
        assert item["names_sha256"] == "ff84bde034b35b40f6a7d8ef0e85c04966d069644ac12ea9db0408591d4fa8b0"
        assert not item.get("error")
    warm = [item["elapsed_ms"] for item in data["samples"][1:]]
    return {
        "measurement": relative,
        "measurement_sha256": sha(ROOT / relative),
        "runner": relative.replace(".json", ".metrics.json"),
        "first_listing_ms": data["samples"][0]["elapsed_ms"],
        "warm_refresh_ms": warm,
        "warm_median_ms": statistics.median(warm),
        "max_gui_heartbeat_gap_ms": data["maximum_gui_heartbeat_gap_ms"],
        "slow_events_20ms_or_more": len(data["slow_gui_events"]),
        "process_elapsed_seconds": metrics["elapsed_seconds"],
        "process_family_cpu_seconds": metrics["cpu_family_seconds"],
        "peak_family_committed_bytes": metrics["peak_family_committed_bytes"],
        "processes": metrics["total_family_processes"],
        "b_prior_target_checks": None if arm == "a" else {
            "first_listing_at_most_2000ms": data["samples"][0]["elapsed_ms"] <= 2000,
            "warm_refreshes_at_most_1000ms": sum(value <= 1000 for value in warm),
            "warm_refresh_denominator": 5,
            "gui_gap_at_most_100ms": data["maximum_gui_heartbeat_gap_ms"] <= 100,
            "peak_at_most_256MiB": metrics["peak_family_committed_bytes"] <= 256 * 1024 * 1024,
        },
    }


def main():
    b = load("docs/evaluation/initial-freeze-manifest.json")
    a = load("experiments/A/docs/evaluation/initial-freeze-manifest.json")
    snapshots = {
        "B": check_snapshot(b, ROOT / b["snapshot_root"]),
        "A": check_snapshot(a, ROOT / "experiments/A" / a["root"]),
    }
    # Initial production and tests must stay byte-identical after source access.
    checked = 0
    for item in b["files"]:
        if item["path"].startswith(("src/", "include/", "tests/")):
            assert sha(ROOT / item["path"]) == item["sha256"], item["path"]
            checked += 1
    assert sha(ROOT / "docs/analysis/initial-claims.json") == "aaa44d0e410267784b116a95cf3ce9e46c0182b212221ce724ab3e385a02156a"
    target_sha = "bed10b64b0dbfb13606b91d8647f7c99f1062fff455a4730547f7d8d51e3c1fb"
    assert sha(ROOT / "build-windows/rea_explorer.exe") == target_sha
    assert sha(ROOT / "dist/windows-20261022T090415366/rea_explorer.exe") == target_sha
    initial = {item["question_id"]: item for item in load("docs/analysis/initial-claims.json")["claims"]}
    nav = load("docs/evaluation/phase6-navigation-source.json")
    shell = load("docs/evaluation/phase6-shell-source.json")
    rows = nav["questions"] + shell["comparisons"]
    assert len(rows) == len(initial) == 25
    assert len({item["question_id"] for item in rows}) == 25
    statuses = Counter()
    for row in rows:
        old = initial[row["question_id"]]
        for new_key, old_key in (("initial_assertion", "initial_assertion"),
                                 ("initial_epistemic_status", "epistemic_status"),
                                 ("initial_confidence", "initial_confidence"),
                                 ("initial_evidence_ids", "evidence_ids"),
                                 ("initial_addresses", "addresses"),
                                 ("initial_unresolved", "unresolved")):
            assert row[new_key] == old[old_key], (row["question_id"], new_key)
        status = row["comparison_status"]
        if status == "no_initial_positive_claim":
            status = "no_initial_positive"
        statuses[status] += 1
    report = {
        "schema_version": 1,
        "created_at_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "kind": "post_freeze_evaluation_summary_not_initial_recovery",
        "frozen_commit": "64032d766cf634fe00b45aa0d8c951878e0ef5c3",
        "initial_snapshots_rehashed": snapshots,
        "B_live_production_and_test_files_unchanged": checked,
        "B_live_and_packaged_exe_sha256": target_sha,
        "source_comparison": {
            "fixed_questions": 25,
            "statuses": dict(statuses),
            "narrow_positive_questions": 13,
            "positive_question_coverage_percent": 100 * 13 / 25,
            "confirmed_narrow_content_among_positive_questions": "12/13; 1 partial, not a full-feature or binary-identity accuracy rate",
            "complete_end_to_end_REA_recoveries_established": 0,
            "exact_VA_source_mappings_proven": 0,
            "exact_shipped_build_2293_proven_by_tag": False,
            "coverage_note": "Each positive question is partial. Unknowns earn no correct-negative credit. Shared evidence is not independent proof.",
        },
        "benchmarks": {arm.upper(): benchmark(arm) for arm in ("a", "b")},
        "benchmark_limitations": [
            "One sequential A then B trial, same host, native windows QPA/Qt6.8.3 plugins, shown Details windows and 1200-name oracle; OS caches not flushed; no statistical generalization",
            "A validation-only harness links frozen existing libraries; A lacks global worker-idle API; B checks session idle; different UI styles and metadata/type/identity workloads",
            "A logs include a pre-timer 328ms event during setup; GUI heartbeat is scoped after show and not full startup",
            "Original same-folder five-F5 task has external UI status-only completion, initial icon->Details switch, different viewport/Shell tree, no full-name hash/internal heartbeat; do not rank times against A/B",
            "Wall time and CPU for complete process families include startup/teardown; Qt listing timers exclude process/Qt initialization; memory is Windows committed memory, not RSS",
            "No REA productivity, model-quality or performance causality inferred from two independent implementations",
        ],
        "later_validation": {
            "user_assisted_outbound_copy": {
                "path": "docs/observations/phase6-user-assisted-native-copy.json",
                "sha256": sha(ROOT / "docs/observations/phase6-user-assisted-native-copy.json"),
                "scope": "One retained source and outbound OS destination with matching 25-byte baseline; user-assisted gesture, not automated REA trace",
            },
            "user_assisted_inbound_transfer": {
                "path": "docs/observations/phase6-user-assisted-native-inbound.json",
                "sha256": sha(ROOT / "docs/observations/phase6-user-assisted-native-inbound.json"),
                "scope": "OS-to-B destination and retained source both match the 26-byte baseline; actual Copy postcondition, not source-removing Move",
            },
            "native_source_removing_move_gesture": "unverified; user chose to leave this variant unverified; actual Qt Move/drop test evidence remains separate",
            "common_external_task": {
                "path": "docs/evaluation/phase6-external-comparison.json",
                "sha256": sha(ROOT / "docs/evaluation/phase6-external-comparison.json"),
                "scope": "Same outer dimensions, Details, input and external action/status protocol; internal-generation/paint/heartbeat timing not established",
            },
        },
    }
    output = ROOT / "docs/evaluation/phase6-summary.json"
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False))


if __name__ == "__main__":
    main()
