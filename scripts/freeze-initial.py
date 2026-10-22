"""Freeze the initial B implementation and bind A's independent snapshot.

Only hashes A snapshot bytes; does not print or import its implementation.
Never traverses the locked verification/source directory.
"""
from pathlib import Path
import datetime
import hashlib
import json
import shutil
import subprocess


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    project = Path(__file__).resolve().parent.parent
    snapshot = project / "artifacts/initial-freeze-b"
    manifest_path = project / "docs/evaluation/initial-freeze-manifest.json"
    if snapshot.exists() or manifest_path.exists():
        raise RuntimeError("Initial freeze paths must be new; never replace a freeze")
    gate = json.loads((project / "docs/analysis/source-gate-status.json").read_text("utf-8-sig"))
    if gate.get("source_downloaded") or gate.get("source_read"):
        raise RuntimeError("Original source must remain unread before initial freeze")
    a_manifest_path = project / "experiments/A/docs/evaluation/initial-freeze-manifest.json"
    a_manifest = json.loads(a_manifest_path.read_text("utf-8-sig"))
    a_root = project / "experiments/A" / a_manifest["root"]
    if a_manifest.get("sourceAccess") or a_manifest.get("otherArmAccess") or a_manifest.get("reaCalls"):
        raise RuntimeError("A's reported isolation contract is not the expected initial contract")
    for record in a_manifest["files"]:
        path = (a_root / record["path"]).resolve()
        if not path.is_relative_to(a_root.resolve()):
            raise RuntimeError("A snapshot path escapes its root")
        if path.stat().st_size != record["bytes"] or digest(path) != record["sha256"]:
            raise RuntimeError(f"A snapshot changed: {record['path']}")
    roots = [project / name for name in ("src", "include", "tests", "scripts", "docs")]
    sources = [path for root in roots for path in root.rglob("*") if path.is_file()]
    sources += [project / name for name in ("CMakeLists.txt", "README.md", ".clang-format", ".gitattributes", ".gitignore")]
    snapshot.mkdir(parents=True)
    records = []
    for path in sorted(sources):
        if "__pycache__" in path.parts:
            continue
        relative = path.relative_to(project)
        destination = snapshot / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
        checksum = digest(path)
        if digest(destination) != checksum:
            raise RuntimeError(f"Snapshot copy changed: {relative}")
        records.append({"path": relative.as_posix(), "bytes": path.stat().st_size, "sha256": checksum})
    binary = project / "build-windows/rea_explorer.exe"
    shutil.copyfile(binary, snapshot / "rea_explorer.exe")
    records.append({"path": "rea_explorer.exe", "bytes": binary.stat().st_size, "sha256": digest(binary), "origin": "build-windows/rea_explorer.exe"})
    report = {
        "schema_version": 1,
        "frozen_at_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "scope": "Initial full-scope B implementation, analysis claims, reports and actually collected verification; remaining limitations are preserved",
        "snapshot_root": "artifacts/initial-freeze-b",
        "original_source_downloaded": False,
        "original_source_read": False,
        "A_implementation_read_by_B_before_this_freeze": False,
        "parent_git_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=project, text=True).strip(),
        "A_manifest_path": a_manifest_path.relative_to(project).as_posix(),
        "A_manifest_sha256": digest(a_manifest_path),
        "A_snapshot_files_rehashed": len(a_manifest["files"]),
        "files": records,
        "self_hash": "Manifest is outside snapshot; SHA256 stored in initial-freeze-manifest.sha256",
    }
    manifest_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    manifest_path.with_suffix(".sha256").write_text(digest(manifest_path) + "\n", encoding="ascii")
    print(json.dumps({"snapshot": str(snapshot), "files": len(records), "A_rehashed": len(a_manifest["files"]), "manifest_sha256": digest(manifest_path)}))


if __name__ == "__main__":
    main()
