#include "explorer/main_window.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

int main(int argc, char **argv) {
    QElapsedTimer lifetime;
    lifetime.start();
    QApplication app(argc, argv);
    if (app.arguments().size() != 3)
        return 2;
    explorer::MainWindow window;
    const auto initMs = lifetime.elapsed();
    const auto location = explorer::Location::folder(app.arguments()[1]);
    const auto output = app.arguments()[2];
    QElapsedTimer request;
    QElapsedTimer beat;
    QJsonArray runs;
    qint64 maxGap = 0;
    int ticks = 0;
    QTimer heartbeat;
    QObject::connect(&heartbeat, &QTimer::timeout, &app, [&] {
        maxGap = qMax(maxGap, beat.restart());
        ++ticks;
    });
    heartbeat.start(5);
    QObject::connect(window.controller(), &explorer::BrowserController::tabChanged, &app, [&](const QUuid &) {
        const auto *state = window.controller()->current();
        if (!state || state->loading)
            return;
        const auto elapsed = request.elapsed();
        if (state->error) {
            app.exit(3);
            return;
        }
        runs.push_back(QJsonObject{{"label", runs.isEmpty() ? "first-process-listing" : "warm-refresh"},
                                   {"elapsed_ms", elapsed},
                                   {"entries", window.fileModel()->rowCount()},
                                   {"heartbeat_ticks", ticks},
                                   {"max_heartbeat_gap_ms", qMax(maxGap, beat.elapsed())}});
        if (runs.size() < 4) {
            QTimer::singleShot(20, &app, [&] {
                ticks = 0;
                maxGap = 0;
                beat.restart();
                request.restart();
                window.controller()->refresh();
            });
            return;
        }
        QJsonObject report{{"qt", qVersion()},
                           {"initial_ui_ms", initMs},
                           {"total_process_ms", lifetime.elapsed()},
                           {"fixture_path", location.path},
                           {"runs", runs},
                           {"cache_limitation",
                            "First process listing does not establish a cold filesystem cache. Widgets "
                            "are rendered offscreen/hidden; no desktop paint latency claim."}};
        QFile file(output);
        if (!file.open(QIODevice::WriteOnly)) {
            app.exit(4);
            return;
        }
        file.write(QJsonDocument(report).toJson());
        file.close();
        app.exit(0);
    });
    QTimer::singleShot(20000, &app, [&] { app.exit(5); });
    request.start();
    beat.start();
    window.openInitial(location);
    return app.exec();
}
