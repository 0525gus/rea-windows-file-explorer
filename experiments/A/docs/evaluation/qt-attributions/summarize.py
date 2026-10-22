"""Render a reviewable inventory from the preserved Qt attribution metadata."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parent
manifest = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
lines = ["# Qt 6.8.3 qtbase attribution inventory", "",
         "This is the full collected qtbase notice inventory, a possible superset of the local application package. See README.md for scope and limitations.", "",
         f"Upstream tag: `v6.8.3`; commit: `{manifest['commitSha']}`.", ""]
strict_failures = []
for file in manifest["files"]:
    path = root / file["path"]
    data = path.read_bytes()
    if len(data) != file["bytes"] or hashlib.sha256(data).hexdigest() != file["sha256"]:
        raise SystemExit(f"Integrity mismatch: {file['path']}")
    if file["kind"] == "attribution":
        try:
            json.loads(data.decode("utf-8-sig"))
        except json.JSONDecodeError as error:
            strict_failures.append({"path": file["sourcePath"], "error": str(error)})
for entry in manifest["entries"]:
    lines.extend([f"## {entry['name']} (`{entry['id']}`)", "",
                  f"Upstream record: [{entry['attributionPath']}](upstream/{entry['attributionPath']})",
                  f"Module label: `{entry['module']}`; upstream version: `{entry['version'] or 'not stated'}`.", "",
                  f"Usage stated upstream: {entry['usage'] or 'not stated'}", "",
                  f"License stated upstream: {entry['license']} (`{entry['licenseId']}`).", ""])
    copyright_value = entry["copyright"]
    values = copyright_value if isinstance(copyright_value, list) else ([copyright_value] if copyright_value else [])
    if values:
        lines.append("Copyright / attribution supplied upstream:")
        lines.append("")
        for value in values:
            lines.extend(["> " + str(value).replace("\n", "\n> "), ""])
    else:
        lines.extend(["No Copyright field was supplied in this attribution entry.", ""])
    if entry["noticePaths"]:
        lines.append("Referenced notice text:")
        lines.append("")
        lines.extend(f"- [{path}](upstream/{path})" for path in entry["noticePaths"])
    else:
        lines.append("No separate LicenseFile was referenced by this entry. Its original attribution metadata is retained; a generic SPDX text may be supplied separately by the packaging owner.")
    lines.append("")
(root / "NOTICES.md").write_text("\n".join(lines), encoding="utf-8")
validation = {"verifiedFiles": len(manifest["files"]), "bytesUnmodified": True,
              "strictJsonParseExceptions": strict_failures,
              "parsePolicy": "Raw source bytes are preserved. Inventory parser permits literal control characters in JSON string values where upstream files are not strict JSON.",
              "separateLicenseFileAbsentIds": [entry["id"] for entry in manifest["entries"] if not entry["noticePaths"]]}
(root / "validation.json").write_text(json.dumps(validation, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(json.dumps(validation, ensure_ascii=False, indent=2))
