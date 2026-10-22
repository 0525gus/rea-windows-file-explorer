"""Collect exact Qt attribution records and referenced notice text, without source files."""
from __future__ import annotations

import concurrent.futures
import hashlib
import json
import posixpath
import re
import time
import urllib.error
import urllib.request
import urllib.parse
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SELECTION = json.loads((ROOT / "tree-selection.json").read_text(encoding="utf-8-sig"))
COMMIT = SELECTION["commitSha"]
REPOSITORY = "qt/qtbase"
RAW = f"https://raw.githubusercontent.com/{REPOSITORY}/{COMMIT}/"
HEADERS = {"User-Agent": "REA-Explorer-Qt-6.8.3-notice-collection", "Accept": "application/vnd.github+json"}
OLD_MANIFEST_PATH = ROOT / "manifest.json"
OLD_MANIFEST = json.loads(OLD_MANIFEST_PATH.read_text(encoding="utf-8")) if OLD_MANIFEST_PATH.exists() else None
if OLD_MANIFEST and OLD_MANIFEST["commitSha"] != COMMIT:
    raise RuntimeError("Refusing to reuse a notice directory pinned to a different commit")
CACHED_HASHES = {record["sourcePath"]: record["sha256"] for record in OLD_MANIFEST["files"]} if OLD_MANIFEST else {}


def request_bytes(url: str) -> bytes:
    for attempt in range(3):
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers=HEADERS), timeout=35) as response:
                return response.read()
        except (urllib.error.URLError, TimeoutError):
            if attempt == 2:
                raise
            time.sleep(1 + attempt)
    raise AssertionError("unreachable")


def save_source(path: str, kind: str) -> dict:
    if path.startswith("/") or path.startswith("../") or "/../" in path or "\\" in path:
        raise ValueError(f"Invalid repository path: {path}")
    if kind == "attribution":
        if posixpath.basename(path) != "qt_attribution.json":
            raise ValueError(f"Not an attribution record: {path}")
    elif not re.search(r"LICEN[SC]E|COPYRIGHT|COPYING|NOTICE", posixpath.basename(path), re.IGNORECASE):
        raise ValueError(f"Not an allowed notice filename: {path}")
    url = RAW + urllib.parse.quote(path, safe="/")
    destination = ROOT / "upstream" / Path(path)
    if destination.exists() and path in CACHED_HASHES:
        data = destination.read_bytes()
        if hashlib.sha256(data).hexdigest() != CACHED_HASHES[path]:
            raise ValueError(f"Cached notice changed: {path}")
    else:
        data = request_bytes(url)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(data)
    return {"path": f"upstream/{path}", "sourcePath": path, "sourceUrl": url, "kind": kind,
            "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def main() -> None:
    commit = json.loads(request_bytes(f"https://api.github.com/repos/{REPOSITORY}/git/commits/{COMMIT}"))
    tree_sha = commit["tree"]["sha"]
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as workers:
        records = list(workers.map(lambda path: save_source(path, "attribution"), SELECTION["attributionPaths"]))
    entries = []
    notice_users: dict[str, list[dict]] = {}
    excluded_references = []
    for record in records:
        # Some upstream records contain literal newlines in quoted fields. Preserve
        # the downloaded bytes and allow those control characters for inventory.
        value = json.loads((ROOT / record["path"]).read_text(encoding="utf-8-sig"), strict=False)
        values = value if isinstance(value, list) else [value]
        for item in values:
            entry = {"attributionPath": record["sourcePath"], "id": item.get("Id"), "name": item.get("Name"),
                     "module": item.get("QDocModule"), "qtPart": item.get("QtParts", item.get("QtPart")),
                     "usage": item.get("QtUsage"), "version": item.get("Version"),
                     "license": item.get("License"), "licenseId": item.get("LicenseId"),
                     "copyright": item.get("Copyright"), "noticePaths": []}
            reference = item.get("LicenseFile", [])
            references = reference if isinstance(reference, list) else ([reference] if reference else [])
            for field in ("CopyrightFile", "NoticeFile", "LicenseFiles"):
                reference = item.get(field, [])
                references.extend(reference if isinstance(reference, list) else ([reference] if reference else []))
            for reference in references:
                resolved = posixpath.normpath(posixpath.join(posixpath.dirname(record["sourcePath"]), reference))
                if resolved.startswith("../") or not re.search(r"LICEN[SC]E|COPYRIGHT|COPYING|NOTICE", posixpath.basename(resolved), re.IGNORECASE):
                    excluded_references.append({"attributionPath": record["sourcePath"], "id": item.get("Id"), "reference": reference, "reason": "Outside approved notice filename/path selection"})
                    continue
                entry["noticePaths"].append(resolved)
                notice_users.setdefault(resolved, []).append({"id": item.get("Id"), "attributionPath": record["sourcePath"]})
            entries.append(entry)
    errors = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as workers:
        pending = {workers.submit(save_source, path, "notice"): path for path in sorted(notice_users)}
        for future in concurrent.futures.as_completed(pending):
            try:
                record = future.result()
                record["referencedBy"] = notice_users[pending[future]]
                records.append(record)
            except Exception as error:
                errors.append({"sourcePath": pending[future], "error": str(error), "referencedBy": notice_users[pending[future]]})
    manifest = {"collectedUtc": datetime.now(timezone.utc).isoformat(), "repository": "https://github.com/qt/qtbase",
                "tag": "v6.8.3", "commitSha": COMMIT, "commitTreeSha": tree_sha,
                "selection": "Every qt_attribution.json enumerated from exact tag, plus its referenced LICENSE/COPYRIGHT/COPYING/NOTICE text files only. No Files/DownloadLocation implementation inputs fetched.",
                "attributionFiles": len(SELECTION["attributionPaths"]), "attributionEntries": len(entries),
                "noticeFiles": sum(record["kind"] == "notice" for record in records),
                "excludedReferences": excluded_references, "fetchErrors": errors,
                "entries": entries, "files": sorted(records, key=lambda record: record["sourcePath"])}
    (ROOT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: manifest[key] for key in ("tag", "commitSha", "commitTreeSha", "attributionFiles", "attributionEntries", "noticeFiles", "excludedReferences", "fetchErrors")}, ensure_ascii=False, indent=2))
    if errors or excluded_references:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
