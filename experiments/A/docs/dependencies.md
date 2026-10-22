# Dependencies and attribution

Direct runtime libraries: Qt Core, Gui, Widgets and Concurrent (Windows 6.8.3; Linux 6.4.2). Qt Test is test-only. C++ standard library and platform SDK are supplied by the approved MSVC/GCC toolchains. No additional third-party dependency was installed or vendored.

Qt offers commercial and LGPLv3/GPL licensing options; its third-party components have their own licenses. The local PoC package is copied by the existing SDK's `windeployqt`, including its selected Qt runtime/plugins. Official license inventory: https://doc.qt.io/qt-6.8/licensing.html and linked third-party/SBOM pages. A public redistribution would need the corresponding Qt notices and license/source/relinking obligations assessed for the chosen distribution; this local checkpoint does not claim that review completed. The MSVC runtime is not bundled or installed by this project.

UI assets are generic Qt/system folder/file/drive icons plus independently written widget styling and Unicode navigation symbols. No original executable resources, Microsoft screenshot crops or Explorer++ source/assets are included. The supplied Win11 screenshot is an observational design reference only.

Official implementation references consulted: Qt QFutureWatcher and QtConcurrent::run documentation (https://doc.qt.io/qt-6/qfuturewatcher.html and https://doc.qt.io/qt-6/qtconcurrentrun.html). All used methods were compiled against actual Qt 6.8.3 and 6.4.2 headers; current web documentation is not treated as proof of old-version compatibility.
