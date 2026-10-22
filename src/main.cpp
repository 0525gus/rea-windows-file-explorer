#include "explorer/main_window.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <algorithm>
#include <cstdio>
class MeasuredApplication final : public QApplication {
  public:
    using QApplication::QApplication;
    bool profile = false;
    QJsonArray slowEvents;
    bool notify(QObject *receiver, QEvent *event) override {
        if (!profile)
            return QApplication::notify(receiver, event);
        QElapsedTimer timer;
        timer.start();
        const bool accepted = QApplication::notify(receiver, event);
        const auto elapsed = timer.elapsed();
        if (elapsed >= 20 && slowEvents.size() < 100)
            slowEvents.append(QJsonObject{{"elapsed_ms", elapsed},
                                          {"event_type", static_cast<int>(event->type())},
                                          {"class", receiver ? receiver->metaObject()->className() : "null"},
                                          {"object", receiver ? receiver->objectName() : QString()}});
        return accepted;
    }
};
int main(int argc, char **argv) {
    MeasuredApplication app(argc, argv);
    app.setApplicationName("REA Explorer");
    app.setOrganizationName("REA Reconstruction PoC");
    QCommandLineParser parser;
    parser.setApplicationDescription("Source-blind Explorer++ reconstruction with Windows 11 inspired UI");
    parser.addHelpOption();
    parser.addOptions(
        {{{"p", "path"}, "Initial local directory or drives.", "path"},
         {"screenshot", "Save this application's rendered QWidget image to a caller-selected path.", "path"},
         {"snapshot", "Save exact loaded model facts to JSON.", "path"},
         {"view", "Initial view: details, icons, or list.", "mode", "details"},
         {"benchmark", "Record first listing and five warm refreshes with GUI heartbeat, then close.",
          "json-path"},
         {"quit-after-render", "Close once screenshot/snapshot is saved."}});
    parser.process(app);
    app.profile = parser.isSet("benchmark");
    explorer::MainWindow window(explorer::makePlatform());
    const auto view = parser.value("view");
    if (view != "details" && view != "icons" && view != "list") {
        std::fprintf(stderr, "Invalid view\n");
        return 2;
    }
    window.setViewMode(view == "icons" ? 1 : (view == "list" ? 2 : 0));
    window.show();
    QElapsedTimer workload, heartbeat;
    workload.start();
    heartbeat.start();
    if (parser.isSet("path"))
        window.openPath(parser.value("path"));
    QTimer capture;
    QTimer measurement;
    QJsonArray samples;
    qint64 maximumGap = 0, previousBeat = 0, sampleGap = 0;
    if (parser.isSet("benchmark")) {
        if (!parser.isSet("path")) {
            std::fprintf(stderr, "Benchmark requires --path\n");
            return 2;
        }
        measurement.setTimerType(Qt::PreciseTimer);
        measurement.setInterval(5);
        QObject::connect(&measurement, &QTimer::timeout, &window, [&] {
            const auto beat = heartbeat.elapsed();
            maximumGap = std::max(maximumGap, beat - previousBeat);
            sampleGap = std::max(sampleGap, beat - previousBeat);
            previousBeat = beat;
            const auto *tab = window.session()->tab(window.currentTabId());
            if (!tab || tab->loading || !window.session()->idle())
                return;
            QJsonArray names;
            for (const auto &entry : tab->visible)
                names.append(entry.name);
            const auto digest = QCryptographicHash::hash(QJsonDocument(names).toJson(QJsonDocument::Compact),
                                                         QCryptographicHash::Sha256)
                                    .toHex();
            QJsonObject sample{{"kind", samples.isEmpty() ? "first-listing" : "warm-refresh"},
                               {"elapsed_ms", workload.elapsed()},
                               {"count", tab->visible.size()},
                               {"names_sha256", QString::fromLatin1(digest)}};
            sample["maximum_gui_gap_ms"] = sampleGap;
            sample["derive_ms"] = tab->deriveMilliseconds;
            sample["render_ms"] = window.lastRenderMilliseconds();
            if (tab->error)
                sample["error"] = tab->error->message;
            samples.append(sample);
            if (samples.size() < 6) {
                sampleGap = 0;
                workload.restart();
                window.session()->refresh(tab->id);
                return;
            }
            measurement.stop();
            const QJsonObject report{{"samples", samples},
                                     {"slow_gui_events", app.slowEvents},
                                     {"maximum_gui_heartbeat_gap_ms", maximumGap},
                                     {"heartbeat_interval_ms", 5},
                                     {"qt_version", qVersion()},
                                     {"os_file_cache", "not flushed; coldness unknown"},
                                     {"scope", "GUI ready listing including model derivation; excludes "
                                               "process/Qt initialization; no other applications included"}};
            QFile output(parser.value("benchmark"));
            if (!output.open(QIODevice::WriteOnly) || output.write(QJsonDocument(report).toJson()) < 0) {
                app.exit(5);
                return;
            }
            window.close();
        });
        measurement.start();
    }
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
            if (parser.isSet("quit-after-render") && !parser.isSet("benchmark"))
                window.close();
        });
        capture.start();
    }
    return app.exec();
}
