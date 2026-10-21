"""Read-only, bounded PE32+ inventory; supplemental byte evidence, not REA output.

Format reference: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
The parser deliberately rejects ambiguous section mappings and unterminated tables.
It does not load, execute, disassemble, or change the input image.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path


MAX_INPUT_BYTES = 128 * 1024 * 1024
MAX_HASH_BYTES = 4 * MAX_INPUT_BYTES
MAX_ITEMS = 65536
MAX_STRING_BYTES = 65536
DIRECTORIES = (
    "export", "import", "resource", "exception", "certificate", "base_relocation",
    "debug", "architecture", "global_pointer", "tls", "load_config", "bound_import",
    "import_address_table", "delay_import", "clr_runtime", "reserved",
)


class InvalidPE(ValueError):
    pass


class PE:
    def __init__(self, data: bytes):
        self.data = data
        self.sections: list[dict] = []
        self.headers_size = 0
        self.image_base = 0
        self.work_items = 0
        self.string_bytes = 0
        self.hash_bytes = 0

    def charge(self, count: int = 1) -> None:
        self.work_items += count
        if self.work_items > MAX_ITEMS:
            raise InvalidPE("inventory item limit exceeded")

    def region(self, offset: int, size: int) -> bytes:
        if offset < 0 or size < 0 or offset > len(self.data) - size:
            raise InvalidPE(f"file range outside image: {offset:#x}+{size:#x}")
        return self.data[offset:offset + size]

    def unpack(self, fmt: str, offset: int) -> tuple:
        return struct.unpack(fmt, self.region(offset, struct.calcsize(fmt)))

    def location(self, offset: int, size: int, rva: int | None = None) -> dict:
        self.hash_bytes += size
        if self.hash_bytes > MAX_HASH_BYTES:
            raise InvalidPE("aggregate hashed source-byte budget exceeded")
        self.region(offset, 0)
        if size < 0 or offset > len(self.data) - size:
            raise InvalidPE("hashed range outside input")
        value = {"file_offset": offset, "size": size,
                 "sha256": hashlib.sha256(memoryview(self.data)[offset:offset + size]).hexdigest()}
        if rva is not None:
            value.update(rva=rva, preferred_va=hex(self.image_base + rva))
        return value

    def map_rva(self, rva: int, size: int = 1) -> tuple[int, int, str]:
        if rva < 0 or size < 0 or rva + size > 0x100000000:
            raise InvalidPE("RVA range outside 32-bit address space")
        candidates = []
        if rva < self.headers_size and rva + size <= self.headers_size:
            candidates.append((rva, self.headers_size - rva, "Headers"))
        for section in self.sections:
            delta = rva - section["rva"]
            if 0 <= delta < section["raw_size"] and size <= section["raw_size"] - delta:
                candidates.append((section["raw_offset"] + delta,
                                   section["raw_size"] - delta, section["name"]))
        if len(candidates) != 1:
            raise InvalidPE(f"RVA has {len(candidates)} file mappings: {rva:#x}+{size:#x}")
        result = candidates[0]
        self.region(result[0], size)
        return result

    def ascii_at(self, rva: int, byte_limit: int = MAX_STRING_BYTES) -> dict:
        offset, available, _ = self.map_rva(rva)
        end = self.data.find(b"\0", offset, offset + min(available, byte_limit, MAX_STRING_BYTES))
        if end < 0:
            raise InvalidPE(f"unterminated or oversized ASCII string at RVA {rva:#x}")
        self.string_bytes += end - offset + 1
        if self.string_bytes > MAX_INPUT_BYTES:
            raise InvalidPE("aggregate decoded string budget exceeded")
        try:
            text = self.data[offset:end].decode("ascii", errors="strict")
        except UnicodeDecodeError as error:
            raise InvalidPE(f"non-ASCII import/export name at RVA {rva:#x}") from error
        return {"value": text, "location": self.location(offset, end - offset + 1, rva)}

    def directory(self, index: int) -> dict:
        if index >= len(self.directories):
            return {"declared": False, "rva": 0, "size": 0, "present": False}
        return self.directories[index]

    @staticmethod
    def no_overlap(ranges: list[tuple[int, int, str]], label: str) -> None:
        ordered = sorted(item for item in ranges if item[0] != item[1])
        for previous, current in zip(ordered, ordered[1:]):
            if previous[1] > current[0]:
                raise InvalidPE(f"overlapping {label}: {previous[2]} / {current[2]}")

    def headers(self) -> dict:
        self.region(0, 64)
        if self.data[:2] != b"MZ":
            raise InvalidPE("missing MZ signature")
        pe_offset, = self.unpack("<I", 0x3C)
        if pe_offset < 64 or self.region(pe_offset, 4) != b"PE\0\0":
            raise InvalidPE("invalid PE signature or offset")
        coff_offset = pe_offset + 4
        machine, count, timestamp, symtab, symbols, optional_size, flags = self.unpack(
            "<HHIIIHH", coff_offset)
        if not 1 <= count <= 96:
            raise InvalidPE("section count outside Windows PE loader limit")
        optional_offset = coff_offset + 20
        self.region(optional_offset, optional_size)
        if optional_size < 112 or self.unpack("<H", optional_offset)[0] != 0x20B:
            raise InvalidPE("only PE32+ optional headers are supported")
        linker_major, linker_minor, code_size, initialized_size, uninitialized_size = self.unpack(
            "<BBIII", optional_offset + 2)
        entry, = self.unpack("<I", optional_offset + 16)
        code_base, = self.unpack("<I", optional_offset + 20)
        self.image_base, = self.unpack("<Q", optional_offset + 24)
        section_alignment, file_alignment = self.unpack("<II", optional_offset + 32)
        os_major, os_minor, image_major, image_minor, subsystem_major, subsystem_minor = self.unpack(
            "<HHHHHH", optional_offset + 40)
        win32_version, = self.unpack("<I", optional_offset + 52)
        image_size, self.headers_size = self.unpack("<II", optional_offset + 56)
        checksum, = self.unpack("<I", optional_offset + 64)
        subsystem, dll_flags = self.unpack("<HH", optional_offset + 68)
        stack_reserve, stack_commit, heap_reserve, heap_commit, loader_flags = self.unpack(
            "<QQQQI", optional_offset + 72)
        directory_count, = self.unpack("<I", optional_offset + 108)
        if directory_count > (optional_size - 112) // 8:
            raise InvalidPE("directory count exceeds optional header")
        self.region(0, self.headers_size)
        section_table = optional_offset + optional_size
        if section_table + count * 40 > self.headers_size:
            raise InvalidPE("section table exceeds SizeOfHeaders")
        self.directories = []
        for index in range(directory_count):
            offset = optional_offset + 112 + index * 8
            address, size = self.unpack("<II", offset)
            if (address == 0) != (size == 0) and index != 8:
                raise InvalidPE(f"inconsistent directory RVA/size at index {index}")
            self.directories.append({"index": index,
                                     "name": DIRECTORIES[index] if index < 16 else f"unknown_{index}",
                                     "address_kind": "file_offset" if index == 4 else "rva",
                                     "rva": address if index != 4 else None,
                                     "address": address, "size": size,
                                     "declared": True, "present": address != 0,
                                     "header_location": self.location(offset, 8)})
        for index in range(count):
            offset = section_table + index * 40
            name, vsize, rva, raw_size, raw_offset, reloc, lines, nreloc, nlines, characteristics = self.unpack(
                "<8sIIIIIIHHI", offset)
            name = name.split(b"\0", 1)[0].decode("ascii", errors="backslashreplace")
            if raw_size:
                self.region(raw_offset, raw_size)
            span = max(vsize, raw_size)
            if rva + span > image_size:
                raise InvalidPE(f"section {name} exceeds SizeOfImage")
            section = {"index": index, "name": name, "rva": rva,
                       "preferred_va": hex(self.image_base + rva), "virtual_size": vsize,
                       "raw_offset": raw_offset, "raw_size": raw_size,
                       "mapped_span": span, "characteristics": hex(characteristics),
                       "executable": bool(characteristics & 0x20000000),
                       "readable": bool(characteristics & 0x40000000),
                       "writable": bool(characteristics & 0x80000000),
                       "header_location": self.location(offset, 40),
                       "raw_location": self.location(raw_offset, raw_size),
                       "pointer_to_relocations": reloc, "pointer_to_linenumbers": lines,
                       "number_of_relocations": nreloc, "number_of_linenumbers": nlines}
            self.sections.append(section)
        self.no_overlap([(0, self.headers_size, "Headers")] + [
            (s["rva"], s["rva"] + s["mapped_span"], s["name"]) for s in self.sections], "virtual ranges")
        self.no_overlap([(0, self.headers_size, "Headers")] + [
            (s["raw_offset"], s["raw_offset"] + s["raw_size"], s["name"]) for s in self.sections], "file ranges")
        for directory in self.directories:
            if directory["present"] and directory["size"]:
                if directory["index"] == 4:
                    offset = directory["address"]
                else:
                    offset, _, _ = self.map_rva(directory["rva"], directory["size"])
                directory["data_location"] = self.location(offset, directory["size"], directory["rva"])
        if entry:
            entry_offset, _, entry_section = self.map_rva(entry)
            entry_info = {"rva": entry, "preferred_va": hex(self.image_base + entry),
                          "section": entry_section, "location": self.location(entry_offset, 1, entry),
                          "file_prefix_location": self.location(entry_offset, min(16, self.map_rva(entry)[1]), entry),
                          "file_bytes_prefix": self.region(entry_offset, min(16, self.map_rva(entry)[1])).hex()}
        else:
            entry_info = {"rva": 0, "preferred_va": None, "section": None, "location": None}
        return {"dos": {"magic": "MZ", "e_lfanew": pe_offset,
                        "header_location": self.location(0, 64)},
                "pe_signature_location": self.location(pe_offset, 4),
                "coff": {"machine": hex(machine), "architecture": {0x8664: "x86_64", 0xAA64: "arm64"}.get(machine, "unknown"),
                         "number_of_sections": count, "timestamp_raw": timestamp,
                         "pointer_to_symbol_table": symtab, "number_of_symbols": symbols,
                         "size_of_optional_header": optional_size, "characteristics": hex(flags),
                         "header_location": self.location(coff_offset, 20)},
                "optional": {"format": "PE32+", "magic": "0x20b", "image_base": hex(self.image_base),
                             "linker_version": [linker_major, linker_minor],
                             "size_of_code": code_size, "size_of_initialized_data": initialized_size,
                             "size_of_uninitialized_data": uninitialized_size, "base_of_code_rva": code_base,
                             "section_alignment": section_alignment, "file_alignment": file_alignment,
                             "os_version": [os_major, os_minor], "image_version": [image_major, image_minor],
                             "subsystem_version": [subsystem_major, subsystem_minor],
                             "win32_version_value": win32_version, "checksum_raw": checksum,
                             "size_of_image": image_size, "size_of_headers": self.headers_size,
                             "subsystem": subsystem, "dll_characteristics": hex(dll_flags),
                             "size_of_stack_reserve": stack_reserve, "size_of_stack_commit": stack_commit,
                             "size_of_heap_reserve": heap_reserve, "size_of_heap_commit": heap_commit,
                             "loader_flags": loader_flags,
                             "number_of_rva_and_sizes": directory_count,
                             "header_location": self.location(optional_offset, optional_size)},
                "entry_point": entry_info, "sections": self.sections, "directories": self.directories}

    def imports(self) -> dict:
        directory = self.directory(1)
        if not directory["present"]:
            return {"present": False, "dlls": [], "dll_count": 0, "symbol_count": 0}
        offset, _, _ = self.map_rva(directory["rva"], directory["size"])
        dlls = []
        terminated = False
        for relative in range(0, directory["size"] - 19, 20):
            self.charge()
            original, timestamp, chain, name_rva, first = self.unpack("<IIIII", offset + relative)
            if not any((original, timestamp, chain, name_rva, first)):
                terminated = True
                break
            if not name_rva or not first:
                raise InvalidPE("import descriptor has no DLL name or IAT")
            lookup = original or first
            thunk_offset, available, _ = self.map_rva(lookup, 8)
            symbols = []
            thunk_terminated = False
            for index in range(available // 8):
                self.charge()
                value, = self.unpack("<Q", thunk_offset + index * 8)
                if value == 0:
                    thunk_terminated = True
                    break
                symbol = {"lookup_location": self.location(thunk_offset + index * 8, 8, lookup + index * 8),
                          "thunk_value_raw": hex(value), "iat_rva": first + index * 8}
                iat_offset, _, _ = self.map_rva(symbol["iat_rva"], 8)
                symbol["iat_location"] = self.location(iat_offset, 8, symbol["iat_rva"])
                if value & (1 << 63):
                    if value & 0x7FFFFFFFFFFF0000:
                        raise InvalidPE("reserved bits set in ordinal import")
                    symbol.update(kind="ordinal", ordinal=value & 0xFFFF, name=None)
                else:
                    if value > 0xFFFFFFFF:
                        raise InvalidPE("named import does not contain a 32-bit RVA")
                    hint_offset, _, _ = self.map_rva(value, 2)
                    hint, = self.unpack("<H", hint_offset)
                    symbol.update(kind="name", ordinal=None, hint=hint,
                                  hint_location=self.location(hint_offset, 2, value),
                                  name=self.ascii_at(value + 2))
                symbols.append(symbol)
            if not thunk_terminated:
                raise InvalidPE("unterminated import lookup table")
            dlls.append({"name": self.ascii_at(name_rva), "original_first_thunk": original,
                         "first_thunk": first, "timestamp_raw": timestamp, "forwarder_chain_raw": chain,
                         "lookup_source": "original_first_thunk" if original else "first_thunk",
                         "descriptor_location": self.location(offset + relative, 20, directory["rva"] + relative),
                         "lookup_terminator_location": self.location(thunk_offset + len(symbols) * 8, 8, lookup + len(symbols) * 8),
                         "symbols": symbols, "symbol_count": len(symbols)})
        if not terminated:
            raise InvalidPE("import descriptor directory has no bounded null terminator")
        return {"present": True, "dll_count": len(dlls),
                "symbol_count": sum(d["symbol_count"] for d in dlls), "dlls": dlls,
                "descriptor_terminator_location": self.location(offset + relative, 20, directory["rva"] + relative)}

    def exports(self) -> dict:
        directory = self.directory(0)
        if not directory["present"]:
            return {"present": False, "directory_declared": directory["declared"],
                    "rva": 0, "size": 0, "symbols": [], "symbol_count": 0,
                    "basis": "export data-directory RVA and size are both zero" if directory["declared"] else "export directory not declared"}
        if directory["size"] < 40:
            raise InvalidPE("export directory shorter than its header")
        offset, _, _ = self.map_rva(directory["rva"], 40)
        flags, timestamp, major, minor, name, base, count, named_count, functions, names, ordinals = self.unpack(
            "<IIHHIIIIIII", offset)
        self.charge(count + named_count)
        function_offset, _, _ = self.map_rva(functions, count * 4) if count else (0, 0, "")
        name_offset, _, _ = self.map_rva(names, named_count * 4) if named_count else (0, 0, "")
        ordinal_offset, _, _ = self.map_rva(ordinals, named_count * 2) if named_count else (0, 0, "")
        by_index: dict[int, list] = {}
        for index in range(named_count):
            name_rva, = self.unpack("<I", name_offset + index * 4)
            slot, = self.unpack("<H", ordinal_offset + index * 2)
            if slot >= count:
                raise InvalidPE("export name ordinal exceeds function table")
            by_index.setdefault(slot, []).append(self.ascii_at(name_rva))
        symbols = []
        for index in range(count):
            target, = self.unpack("<I", function_offset + index * 4)
            forwarded = directory["rva"] <= target < directory["rva"] + directory["size"]
            target_info = None
            if target and not forwarded:
                owners = [s for s in self.sections if s["rva"] <= target < s["rva"] + s["mapped_span"]]
                if len(owners) != 1:
                    raise InvalidPE("export target has no unique section mapping")
                owner = owners[0]
                backed = target - owner["rva"] < owner["raw_size"]
                target_info = {"preferred_va": hex(self.image_base + target), "section": owner["name"],
                               "section_executable": owner["executable"], "file_backed": backed,
                               "location": self.location(self.map_rva(target)[0], 1, target) if backed else None}
            symbols.append({"ordinal": base + index, "rva": target,
                            "unused_slot": target == 0, "names": by_index.get(index, []),
                            "target": target_info,
                            "forwarder": self.ascii_at(target, directory["rva"] + directory["size"] - target) if forwarded else None,
                            "address_table_location": self.location(function_offset + index * 4, 4, functions + index * 4)})
        return {"present": True, "name": self.ascii_at(name), "ordinal_base": base,
                "flags_raw": flags, "timestamp_raw": timestamp, "version": [major, minor],
                "header_location": self.location(offset, 40, directory["rva"]),
                "function_address_table_location": self.location(function_offset, count * 4, functions) if count else None,
                "name_pointer_table_location": self.location(name_offset, named_count * 4, names) if named_count else None,
                "ordinal_table_location": self.location(ordinal_offset, named_count * 2, ordinals) if named_count else None,
                "symbol_count": count, "named_count": named_count, "symbols": symbols}


def main() -> int:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("target", type=Path)
    cli.add_argument("--output", type=Path, required=True)
    cli.add_argument("--manifest", type=Path, required=True)
    args = cli.parse_args()
    if args.output.resolve() in (args.target.resolve(), args.manifest.resolve()):
        raise InvalidPE("output must not overwrite the input image or target manifest")
    with args.target.open("rb") as stream:
        data = stream.read(MAX_INPUT_BYTES + 1)
    if len(data) > MAX_INPUT_BYTES:
        raise InvalidPE("input exceeds 128 MiB safety limit")
    digest = hashlib.sha256(data).hexdigest()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8-sig"))
    expected = manifest["target"]
    if digest != expected["sha256"] or len(data) != expected["bytes"]:
        raise InvalidPE("input digest/length differs from the pinned target manifest")
    pe = PE(data)
    result = {"schema": "rea-explorer-supplemental-pe-inventory-v1",
              "authority": "supplemental-byte-derived; not REA Evidence",
              "generator": "scripts/inspect_pe.py", "python": sys.version.split()[0],
              "format_reference": "https://learn.microsoft.com/en-us/windows/win32/debug/pe-format",
              "target": {"path": str(args.target.resolve()), "bytes": len(data), "sha256": digest},
              "manifest_match": True, "headers": pe.headers(), "imports": pe.imports(), "exports": pe.exports(),
              "coverage": {"parsed": ["DOS", "COFF", "PE32+ optional header", "section table", "ordinary imports", "export directory"],
                           "resource_tree": "not parsed here; use separate REA inspect_pe_resources Evidence",
                           "delay_imports": "directory presence reported; descriptors not parsed",
                           "runtime_behavior": "not observed; imports do not prove invocation",
                           "limitations": ["Preferred VAs are image-base plus RVA, not observed process addresses.",
                                           "Not a Windows-loader validation or malware-safety verdict.",
                                           "No function recovery, string discovery, resource decoding, or call graph is performed."]},
              "bounds": {"max_input_bytes": MAX_INPUT_BYTES, "max_inventory_items": MAX_ITEMS,
                         "max_aggregate_hashed_source_bytes": MAX_HASH_BYTES,
                         "max_ascii_string_bytes": MAX_STRING_BYTES,
                         "max_aggregate_string_bytes": MAX_INPUT_BYTES,
                         "work_items_used": pe.work_items, "string_bytes_used": pe.string_bytes,
                         "hashed_source_bytes_used": pe.hash_bytes}}
    # Build the entire inventory before writing; parser failures leave no partial output.
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"sha256": digest, "bytes": len(data), "sections": len(pe.sections),
                      "entry_point": result["headers"]["entry_point"],
                      "import_dlls": result["imports"]["dll_count"],
                      "import_symbols": result["imports"]["symbol_count"],
                      "export_directory_present": result["exports"]["present"]}))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (InvalidPE, OSError, KeyError, json.JSONDecodeError) as error:
        print(f"PE inventory failed: {error}", file=sys.stderr)
        raise SystemExit(1)
