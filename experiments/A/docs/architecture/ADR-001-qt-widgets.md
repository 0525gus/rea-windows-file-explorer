# ADR-001 — Qt Widgets and owned asynchronous listings

Date: 2026-10-21. Status: accepted for independent Phase 3 gate; implementation pending root authorization.

Use Qt6 Widgets rather than QML. Widgets provides mature tree/table/list views, extended selection, keyboard focus, desktop drag/drop, dialogs and accessibility with fewer custom interaction components. A Win11-inspired dark layout can be drawn using palette/style rules while retaining OS frame, system font and permitted icons. QML offers flexible animation/layout but would add model/interaction plumbing without a requirement needing it. Both are portable; actual Windows/Linux builds will establish compatibility.

Own a value snapshot model and asynchronous adapter enumeration instead of allowing QFileSystemModel to own navigation/history/error semantics. This makes denied/invalid-path errors explicit, keeps tab transactions deterministic, and allows fake-adapter tests. QFileSystemModel remains acceptable for a read-only lazy sidebar tree if its values only propose navigation through the shared controller; it is not the authority for committed content or mutation safety. The extra model code is bounded by common facts and one sort/filter representation.

Use Qt Core value types and bounded QtConcurrent workers to keep C++20 core portable and match Qt 6.4 in Linux. Native details belong in adapter translation units. Real filesystem integration supplements pure core tests; mocks do not prove OS behavior.

Consequences: list icons and metadata must avoid costly repeated UI-thread work, enumeration cancellation needs generations and safe worker ownership, and mutation consistency must be implemented once in a service shared by buttons/clipboard/drop. None of those requirements is delegated to cosmetic UI code.
