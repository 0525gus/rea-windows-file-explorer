# Qt 6.8.3 attribution and notice collection

This directory preserves notice inputs from the official **qt/qtbase v6.8.3** tag, not the moving Qt 6.8 documentation pages. The tag resolves to commit `c07c2d5a527a644d36e7853d55132ae38921682f`, whose Git tree is `1a28ee71f7490a7b4c80841545438d740a5759c3`.

Collected: **48 qt_attribution.json files, 58 attribution entries, and 49 directly referenced license/copyright/copying/notice files**. All 97 downloaded files are preserved byte-for-byte under `upstream/`; `manifest.json` records their pinned raw source URLs, SHA-256 hashes, byte lengths and entry-to-notice relationships. `NOTICES.md` renders all upstream attribution/copyright fields with links to the preserved records and texts. `validation.json` records successful local rehash verification and parsing limitations. `collection-seal.json` binds all local deliverables after collection; its hash is in `collection-seal.sha256`.

Sources are limited to [the official Qt repository tag](https://github.com/qt/qtbase/tree/v6.8.3), its Git metadata endpoints, attribution JSON and the text files referenced by the license/notice fields. The collection script does not fetch the JSON `Files` or `DownloadLocation` implementation inputs, external component sources, Qt implementation files, or any Explorer++ files. Both LICENSE and the equivalent British LICENCE spelling are accepted. Original notices remain unchanged; generated inventory/README files are separate.

## Relation to the local package

The collection intentionally covers the entire qtbase tree and may be a **superset** of the actual local package's Core, Gui, Widgets, Concurrent and plugins. It includes optional/platform-specific components and developer/test tooling, plus records for Qt modules such as Network, SQL and D-Bus that may not be shipped by this application. An upstream `QtUsage` or `QDocModule` label is retained as supplied; it is not proof that a particular component was compiled into a packaged DLL. No binary-to-object mapping, build-feature configuration, or authoritative per-binary SBOM was inspected in this follow-up. The packaging owner should map the actual shipped files/configuration before reducing this superset.

Nine entries supply no separate `LicenseFile`: md4, md5, sha1, siphash, sqlite, wintab, tika-mimetypes, unicode-character-database and unicode-cldr. Their inline copyright/license metadata is retained in both the raw attribution JSON and NOTICES.md. This directory does not invent an upstream notice file for them. The packaging owner already has a separately collected qtbase LICENSES text set; those generic SPDX/custom license texts complement this attribution-specific set.

FreeType's referenced `LICENSE.txt` already embeds the FTL and GPLv2 texts in this exact Qt tag; no FreeType implementation or secondary source-header notice was fetched. The direct-reference scope does not assert that every transitive notice reference or every optional component's distribution obligation has been audited.

## Reproduction and verification

`collect.py` resolves the pinned commit tree, downloads the selected attribution records and only their referenced notice-named files, and writes the manifest. Existing cached bytes are reused only when their hash matches the same-commit manifest. `summarize.py` verifies every raw file's hash and size before producing the reviewable inventory and validation record. Run with the existing Python interpreter; no software is installed. The initial selection metadata and exact commit are in `tree-selection.json`.

Six upstream attribution files contain literal control characters inside quoted strings and are not strict JSON. Their bytes are preserved. The inventory parser uses Python's `json.loads(..., strict=False)` only to tolerate those upstream string values; `validation.json` names all six records and their strict-parser diagnostics. There were no excluded reference files or fetch errors in the completed collection.

This is a provenance-preserving input bundle for local packaging review. It does **not** declare blanket legal compliance, select a commercial/GPL/LGPL option, establish a complete corresponding-source/relinking arrangement, verify license compatibility, or replace review of the actual distribution and its obligations. Packaging changes and dependencies.md remain owned by the root agent; this follow-up modifies only this new attribution directory. The earlier independent A implementation freeze remains unchanged.
