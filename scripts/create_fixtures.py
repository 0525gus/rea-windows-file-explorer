#!/usr/bin/env python3
"""Create or check deterministic, owned, non-destructive Explorer fixtures.

Only writes under this repository's test-data/fixture-v1. Existing unexpected
entries or changed contents/metadata cause an error before further writes.
Does not delete, chmod, change ACLs, create links, or touch system/user files.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import stat
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


SCHEMA_VERSION = 1
FIXTURE_ID = "rea-explorer-fixture-v1"
BASE_EPOCH = 1_577_836_800  # 2020-01-01T00:00:00Z
ROOT = Path(__file__).resolve().parent.parent / "test-data" / "fixture-v1"
OWNER_NAME = ".fixture-owner.json"
MANIFEST_NAME = "manifest.json"
CHECKSUM_NAME = "manifest.sha256"
REPARSE_FLAG = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)


@dataclass(frozen=True)
class FileSpec:
    path: str
    content: bytes
    modified_epoch: int

    def manifest_entry(self) -> dict[str, object]:
        return {
            "path": self.path,
            "bytes": len(self.content),
            "sha256": sha256(self.content),
            "modified_epoch_seconds": self.modified_epoch,
            "modified_utc": datetime.fromtimestamp(
                self.modified_epoch, timezone.utc
            ).strftime("%Y-%m-%dT%H:%M:%SZ"),
        }


def sha256(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def json_bytes(value: object) -> bytes:
    return (json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n").encode(
        "utf-8"
    )


def fixture_specs() -> tuple[list[FileSpec], set[str]]:
    files: list[FileSpec] = []
    directories = {
        "read-only",
        "read-only/empty",
        "read-only/bulk",
        "read-only/unicode",
        "read-only/navigation/alpha/beta/gamma",
        "read-only/navigation/peer",
        "read-only/errors",
        "mutation",
        "mutation/collision/source",
        "mutation/collision/destination",
        "mutation/rename",
        "mutation/external-change",
    }

    def add(path: str, content: bytes, offset: int = 0) -> None:
        files.append(FileSpec(path, content, BASE_EPOCH + offset))

    for index in range(1_200):
        size = 1 + (index % 97) * 17
        seed = f"REA fixture-v1 bulk file {index:04d}\n".encode("ascii")
        content = (seed * ((size + len(seed) - 1) // len(seed)))[:size]
        add(
            f"read-only/bulk/file-{index:04d}.bin",
            content,
            (index % 113) * 60,
        )

    unicode_names = [
        "한글_파일.txt",
        "한글_특수문자_é_😀.txt",
        "공백 있는 이름.txt",
        "special_!@#$%^&()[]{}+=,;'.txt",
        "café.txt",
        "long_" + "a" * 160 + ".txt",
    ]
    for index, name in enumerate(unicode_names):
        add(
            "read-only/unicode/" + name,
            ("UTF-8 fixture: " + name + "\n").encode("utf-8"),
            7_200 + index * 60,
        )

    add("read-only/navigation/alpha/level-one.txt", b"level one\n", 10_800)
    add("read-only/navigation/alpha/beta/level-two.txt", b"level two\n", 10_860)
    add("read-only/navigation/alpha/beta/gamma/level-three.txt", b"level three\n", 10_920)
    add("read-only/navigation/peer/peer.txt", b"independent tab path\n", 10_980)
    add("read-only/errors/normal-readable.txt", b"Not an access-denied fixture.\n", 11_040)
    add("mutation/collision/source/duplicate.txt", b"SOURCE duplicate content\n", 14_400)
    add("mutation/collision/source/unique.txt", b"SOURCE unique content\n", 14_460)
    add(
        "mutation/collision/destination/duplicate.txt",
        b"DESTINATION original content MUST be preserved before confirmation\n",
        14_520,
    )
    add("mutation/rename/existing.txt", b"rename collision target\n", 14_580)
    add("mutation/rename/rename-me.txt", b"rename source\n", 14_640)
    add("mutation/external-change/disappearing.txt", b"external deletion test input\n", 14_700)

    for item in list(directories) + [spec.path for spec in files]:
        relative = PurePosixPath(item)
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError(f"Unsafe specification path: {item!r}")
        for parent in relative.parents:
            if str(parent) != ".":
                directories.add(parent.as_posix())
    files.sort(key=lambda spec: spec.path)
    if len({spec.path for spec in files}) != len(files):
        raise ValueError("Duplicate fixture path")
    return files, directories


def expected_documents(files: list[FileSpec], directories: set[str]) -> dict[str, bytes]:
    bulk = [spec for spec in files if spec.path.startswith("read-only/bulk/")]
    unicode_files = [spec for spec in files if spec.path.startswith("read-only/unicode/")]

    def names(items: list[FileSpec]) -> list[str]:
        return [PurePosixPath(item.path).name for item in items]

    bulk_name_order = sorted(bulk, key=lambda item: PurePosixPath(item.path).name)
    bulk_size_order = sorted(
        bulk, key=lambda item: (len(item.content), PurePosixPath(item.path).name)
    )
    bulk_date_order = sorted(
        bulk, key=lambda item: (item.modified_epoch, PurePosixPath(item.path).name)
    )
    manifest = {
        "schema_version": SCHEMA_VERSION,
        "fixture_id": FIXTURE_ID,
        "root_relative_to_repository": "test-data/fixture-v1",
        "content_encoding": "UTF-8 for text; deterministic bytes for bulk .bin",
        "metadata_policy": {
            "modified_time": "Exact integer UTC seconds, explicitly set on owned newly created files.",
            "created_time": "Not fixed or asserted; filesystem-dependent.",
            "directory_times": "Not fixed or asserted.",
            "permissions": "No chmod/ACL/read-only-attribute changes; read-only names identify test suites only.",
            "links": "No symlinks, junctions, or reparse points are created or accepted.",
        },
        "safety": {
            "read_only_suite": "read-only/ is for nonmutating tests, not OS-enforced denial.",
            "mutation_suite": "mutation/ is a baseline; run modifying tests on a separately owned disposable copy.",
            "generator_behavior": "No deletion or replacement; mismatches/unexpected entries fail closed.",
            "unsupported_fixtures": [
                "actual access-denied folder",
                "symlink/reparse-point or cyclic-link fixture",
                "locked file or permission-based rename failure",
                "paths beyond platform limits",
            ],
        },
        "counts": {
            "files": len(files),
            "directories_excluding_root": len(directories),
            "bulk_files": len(bulk),
            "data_bytes": sum(len(spec.content) for spec in files),
        },
        "directories": sorted(directories),
        "files": [spec.manifest_entry() for spec in files],
        "expected": {
            "provenance": "Synthetic fixture facts and explicit test comparators, not Explorer++ reverse-engineering findings.",
            "listing": {
                "scope": "Immediate children only; hidden manifest/owner files are outside scenario folders.",
                "read-only/empty": [],
                "read-only/bulk": names(bulk_name_order),
                "read-only/unicode": sorted(names(unicode_files)),
                "read-only/navigation/alpha": ["beta", "level-one.txt"],
                "mutation/collision/source": ["duplicate.txt", "unique.txt"],
                "mutation/collision/destination": ["duplicate.txt"],
            },
            "sort": {
                "scope": "read-only/bulk; files only",
                "name_comparator": "Ascending Unicode codepoint name order; zero-padded ASCII names avoid locale/natural-sort ambiguity.",
                "tie_break": "Ascending exact filename for size/date ties; verify original policy separately.",
                "descending": "Reverse the complete corresponding ascending list for this synthetic comparator.",
                "name_ascending": names(bulk_name_order),
                "size_bytes_ascending": names(bulk_size_order),
                "modified_utc_ascending": names(bulk_date_order),
            },
            "search": {
                "comparator": "Nonrecursive casefolded literal substring of filename; no wildcard or normalization. Original app semantics require observation.",
                "cases": [
                    {
                        "folder": "read-only/bulk",
                        "query": "FILE-00",
                        "matches_name_ascending": names(
                            [
                                spec
                                for spec in bulk_name_order
                                if "file-00" in PurePosixPath(spec.path).name.casefold()
                            ]
                        ),
                    },
                    {
                        "folder": "read-only/unicode",
                        "query": "한글",
                        "matches_name_ascending": sorted(
                            name for name in names(unicode_files) if "한글" in name
                        ),
                    },
                    {
                        "folder": "read-only/bulk",
                        "query": "no-such-fixture-name",
                        "matches_name_ascending": [],
                    },
                ],
            },
            "nonexistent_path": "read-only/errors/this-path-must-not-exist",
            "navigation_sequence": [
                "read-only/navigation/alpha",
                "read-only/navigation/alpha/beta",
                "read-only/navigation/alpha/beta/gamma",
                "read-only/navigation/peer",
            ],
            "independent_tab_paths": [
                "read-only/navigation/alpha/beta",
                "read-only/navigation/peer",
            ],
        },
    }
    manifest_content = json_bytes(manifest)
    return {
        OWNER_NAME: json_bytes(
            {"fixture_id": FIXTURE_ID, "schema_version": SCHEMA_VERSION}
        ),
        MANIFEST_NAME: manifest_content,
        CHECKSUM_NAME: f"{sha256(manifest_content)}  {MANIFEST_NAME}\n".encode("ascii"),
    }


def assert_plain(path: Path, *, directory: bool) -> os.stat_result:
    info = path.lstat()
    if stat.S_ISLNK(info.st_mode) or getattr(info, "st_file_attributes", 0) & REPARSE_FLAG:
        raise RuntimeError(f"Refusing link/reparse point: {path}")
    expected_type = stat.S_ISDIR if directory else stat.S_ISREG
    if not expected_type(info.st_mode):
        raise RuntimeError(f"Unexpected entry type: {path}")
    return info


def assert_existing_parent_chain(path: Path) -> None:
    for parent in [*reversed(path.parents), path]:
        if os.path.lexists(parent):
            assert_plain(parent, directory=True)


def preflight(
    files: list[FileSpec], directories: set[str], documents: dict[str, bytes]
) -> tuple[set[str], set[str], set[str]]:
    assert_existing_parent_chain(ROOT)
    if not ROOT.exists():
        return set(), set(), set()

    owner = ROOT / OWNER_NAME
    entries = list(ROOT.iterdir())
    if entries and not os.path.lexists(owner):
        raise RuntimeError("Existing nonempty fixture root has no ownership marker")
    if os.path.lexists(owner):
        assert_plain(owner, directory=False)
        if owner.read_bytes() != documents[OWNER_NAME]:
            raise RuntimeError("Existing fixture ownership marker does not match")

    # An interrupted initial assembly has an owner but no committed manifest.
    # Only exact expected bytes can have their pending timestamp completed.
    incomplete_owned_assembly = (
        os.path.lexists(owner)
        and not os.path.lexists(ROOT / MANIFEST_NAME)
        and not os.path.lexists(ROOT / CHECKSUM_NAME)
    )

    content_by_path = {spec.path: spec for spec in files}
    present_files: set[str] = set()
    present_dirs: set[str] = set()
    unfinished_times: set[str] = set()
    stack = [ROOT]
    while stack:
        directory = stack.pop()
        for entry in directory.iterdir():
            relative = entry.relative_to(ROOT).as_posix()
            info = entry.lstat()
            if stat.S_ISLNK(info.st_mode) or getattr(info, "st_file_attributes", 0) & REPARSE_FLAG:
                raise RuntimeError(f"Refusing link/reparse point: {entry}")
            if stat.S_ISDIR(info.st_mode):
                if relative not in directories:
                    raise RuntimeError(f"Unexpected existing directory: {entry}")
                present_dirs.add(relative)
                stack.append(entry)
            elif stat.S_ISREG(info.st_mode):
                expected_document = documents.get(relative)
                expected_file = content_by_path.get(relative)
                if expected_document is not None:
                    if entry.read_bytes() != expected_document:
                        raise RuntimeError(f"Changed existing metadata document: {entry}")
                elif expected_file is not None:
                    if entry.read_bytes() != expected_file.content:
                        raise RuntimeError(f"Changed existing fixture contents: {entry}")
                    if info.st_mtime_ns != expected_file.modified_epoch * 1_000_000_000:
                        if incomplete_owned_assembly:
                            unfinished_times.add(relative)
                        else:
                            raise RuntimeError(f"Changed existing fixture modified time: {entry}")
                else:
                    raise RuntimeError(f"Unexpected existing file: {entry}")
                present_files.add(relative)
            else:
                raise RuntimeError(f"Unexpected existing entry type: {entry}")
    return present_files, present_dirs, unfinished_times


def set_modified_time(path: Path, modified_epoch: int) -> None:
    # Windows Python does not implement utime(follow_symlinks=False). The
    # single-writer fixture operation checks the owned entry before utime.
    assert_plain(path, directory=False)
    timestamp = modified_epoch * 1_000_000_000
    os.utime(path, ns=(timestamp, timestamp))


def write_exclusive(path: Path, content: bytes, modified_epoch: int | None = None) -> None:
    # xb prevents replacing even an unexpected entry introduced after preflight.
    with path.open("xb") as stream:
        stream.write(content)
    if modified_epoch is not None:
        set_modified_time(path, modified_epoch)


def run(check_only: bool) -> dict[str, object]:
    files, directories = fixture_specs()
    documents = expected_documents(files, directories)
    present_files, present_dirs, unfinished_times = preflight(files, directories, documents)
    expected_paths = {spec.path for spec in files} | set(documents)
    missing_files = expected_paths - present_files
    missing_dirs = directories - present_dirs
    if check_only:
        if not ROOT.exists() or missing_files or missing_dirs or unfinished_times:
            raise RuntimeError(
                f"Fixture incomplete: {len(missing_files)} files, {len(missing_dirs)} directories missing; "
                f"{len(unfinished_times)} timestamps pending"
            )
    else:
        ROOT.mkdir(parents=True, exist_ok=True)
        if OWNER_NAME not in present_files:
            write_exclusive(ROOT / OWNER_NAME, documents[OWNER_NAME])
        for relative in sorted(directories, key=lambda item: (item.count("/"), item)):
            if relative not in present_dirs:
                (ROOT / relative).mkdir(exist_ok=False)
        for spec in files:
            if spec.path not in present_files:
                write_exclusive(ROOT / spec.path, spec.content, spec.modified_epoch)
            elif spec.path in unfinished_times:
                set_modified_time(ROOT / spec.path, spec.modified_epoch)
        for name in (MANIFEST_NAME, CHECKSUM_NAME):
            if name not in present_files:
                write_exclusive(ROOT / name, documents[name])
        verified_files, verified_dirs, pending_times = preflight(files, directories, documents)
        if verified_files != expected_paths or verified_dirs != directories or pending_times:
            raise RuntimeError("Post-generation fixture coverage differs from specification")

    loaded = json.loads((ROOT / MANIFEST_NAME).read_text(encoding="utf-8"))
    unicode_names = loaded["expected"]["listing"]["read-only/unicode"]
    if "한글_파일.txt" not in unicode_names or not any("😀" in name for name in unicode_names):
        raise RuntimeError("Unicode manifest roundtrip did not preserve source filenames")
    return {
        "mode": "check" if check_only else "create-or-verify",
        "fixture_id": FIXTURE_ID,
        "root": str(ROOT),
        "data_files": len(files),
        "bulk_files": loaded["counts"]["bulk_files"],
        "data_bytes": loaded["counts"]["data_bytes"],
        "created_files": 0 if check_only else len(missing_files),
        "completed_interrupted_timestamps": 0 if check_only else len(unfinished_times),
        "reused_files": len(present_files),
        "manifest_sha256": sha256(documents[MANIFEST_NAME]),
        "unicode_roundtrip": "verified",
        "permissions_changed": False,
        "links_created": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check", action="store_true", help="Verify complete owned fixtures without writes"
    )
    args = parser.parse_args()
    try:
        result = run(args.check)
    except (OSError, RuntimeError, ValueError) as error:
        print(json.dumps({"status": "error", "message": str(error)}, ensure_ascii=True), file=sys.stderr)
        return 1
    print(json.dumps({"status": "ok", **result}, ensure_ascii=True, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
