# Shared development environment (no REA analysis)

Windows11 x64; existing MSVC19.51 (VS2026 BuildTools) and bundled CMake4.3.1/Ninja1.13.2, Qt6.8.3 installed at tools/qt/6.8.3/msvc2022_64. scripts/build-windows.ps1 accepts -SourceDirectory <Aroot> -BuildDirectory <Aroot/build-windows> -RunTests and uses child-only environment, hidden supervised build stages. It forces QT_QPA_PLATFORM=offscreen for test invocations, not normal launched UI. scripts/run-build-stage.py retains process-family resource metrics and stage logs. Same tools available to both arms.

WSL Ubuntu24.04.1 existing Qt6.4.2 Core/Widgets/Test/Concurrent, CMake3.28.3, Ninja1.11.1, g++13.3.0. Linux builds through wsl.exe -d Ubuntu -- bash -lc using /mnt/c/dev/Windows_fileexplore/experiments/A paths; no installation needed. Windows LLVM clang-format/tidy22.1.3 at C:/Program Files/Microsoft Visual Studio/18/Insiders/VC/Tools/Llvm/x64/bin. Git2.55.

Original official executable1.4.0.2293 4,100,096bytes SHA25622ac20341c5525658ebe5e9c4b9ac9f7c152b18080ff347861fac3f04d56a2d6. Do not use original source. Pure runtime original observations and actual Windows11 design screenshot in this inputs directory are allowed. Root will adjudicate additional runtime observations if needed.

Fixture C:/dev/Windows_fileexplore/test-data/fixture-v1: 1217datafiles, bulk1200files,962374bytes, manifestSHA256278d49240d795cabcd15be949cdc4f235722a0c8d92236854c503efc2dd14bc1. Immutable; don't mutate. Create disposable copies under own A artifacts for mutation tests. "read-only" is suite name not ACLdenial. C:/dev/Windows_fileexplore/verification/source is actually List/Read denied for currentuser, contains ONLY non-sourceprobe, may be used as explicit denied-read path but NEVER enumerate via shell or change permissions; exercise solely own platform error test. No source exists there.

ExperimentA freshcontext noREA vs B withREA, same inherited model/settings (specificdeploymentID/billingunknown), samefullfeaturesFR01–20/testscope and optionaldarkUI. Workallocation bothunbounded untilsameprooflevel; actualtime/cost/resources reportunknownwhereunavailable. A must independentlydesign; B docs/code inaccessiblebyinstruction. Sharedsystemuser ACLdoesn't enforceA/Bfilesystemisolation, reportthislimitation; freshcontext/whitelist/trace is actualcontrol. Originalsourcegate has realACL.

