#include "explorer/main_window.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <cstdio>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("REA Explorer");
    app.setOrganizationName("REA Reconstruction PoC");
    QCommandLineParser parser;
    parser.setApplicationDescription("Source-blind Explorer++ reconstruction with Windows 11 inspired UI");
    parser.addHelpOption();
    parser.addOptions(
        {{{"p", "path"}, "Initial local directory or drives.", "path"},
         {"screenshot", "Save this application's rendered QWidget image to a caller-selected path.", "path"},
         {"snapshot", "Save exact loaded model facts to JSON.", "path"},
         {"quit-after-render", "Close once screenshot/snapshot is saved."}});
    parser.process(app);
    explorer::MainWindow window(explorer::makePlatform());
    window.show();
    if (parser.isSet("path"))
        window.openPath(parser.value("path"));
    QTimer capture;
    int ticks = 0;
    if (parser.isSet("screenshot") || parser.isSet("snapshot")) {
        capture.setInterval(100);
        QObject::connect(&capture, &QTimer::timeout, &window, [&] {
            if (++ticks > 300) {
                std::fprintf(stderr, "Capture timed out\n");
                app.exit(2);
                return;
            }
            const auto state = window.snapshot();
            if (state["loading"].toBool() || !window.session()->idle() || state.isEmpty())
                return;
            if (parser.isSet("screenshot") && !window.grab().save(parser.value("screenshot"))) {
                std::fprintf(stderr, "Screenshot save failed\n");
                app.exit(3);
                return;
            }
            if (parser.isSet("snapshot")) {
                QFile file(parser.value("snapshot"));
                if (!file.open(QIODevice::WriteOnly)) {
                    app.exit(4);
                    return;
                }
                file.write(QJsonDocument(state).toJson());
            }
            capture.stop();
            if (parser.isSet("quit-after-render"))
                window.close();
        });
        capture.start();
    }
    return app.exec();
}
