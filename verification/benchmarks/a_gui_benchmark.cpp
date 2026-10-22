// Phase 6 validation instrumentation. Links A's existing libraries without
// changing either arm's frozen implementation or prior benchmark evidence.
#include "explorer/main_window.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QEvent>
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
    app.setApplicationName(QStringLiteral("Explorer A"));
    app.setStyle(QStringLiteral("Fusion")); // Same app-level style as A's frozen main.
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Phase 6 A shown-GUI first + five refresh comparison"));
    parser.addHelpOption();
    parser.addOptions({{{"p", "path"}, "Bulk fixture directory (exactly 1200 expected names).", "path"},
                       {"benchmark", "Write retained measurement JSON and close.", "json-path"}});
    parser.process(app);
    if (!parser.isSet("path") || !parser.isSet("benchmark")) {
        std::fprintf(stderr, "Both --path and --benchmark are required.\n");
        return 2;
    }
    const auto requested = explorer::Location::folder(parser.value("path"));
    constexpr int expectedCount = 1200;
    const QByteArray expectedNamesHash("ff84bde034b35b40f6a7d8ef0e85c04966d069644ac12ea9db0408591d4fa8b0");
    app.profile = true;
    explorer::MainWindow window;
    window.show();
    QElapsedTimer workload, heartbeat;
    workload.start();
    heartbeat.start();
    window.openInitial(requested);
    QTimer measurement, deadline;
    QJsonArray samples;
    qint64 maximumGap = 0, previousBeat = 0, sampleGap = 0;
    bool finished = false;
    auto finish = [&](const QString &status, int exitCode, const QString &error = {}) {
        if (finished)
            return;
        finished = true;
        measurement.stop();
        deadline.stop();
        QJsonObject report{
            {"samples", samples},
            {"slow_gui_events", app.slowEvents},
            {"maximum_gui_heartbeat_gap_ms", maximumGap},
            {"heartbeat_interval_ms", 5},
            {"qt_version", qVersion()},
            {"os_file_cache", "not flushed; coldness unknown"},
            {"scope", "GUI ready listing including model derivation; excludes process/Qt initialization; no "
                      "other applications included"},
            {"status", status},
            {"fixture_path", requested.path},
            {"shown_window", true},
            {"style", "Fusion as in A frozen main"},
            {"measurement_origin", "phase6 validation-only harness; existing A libraries and frozen headers"},
            {"ready_boundary", "correct current location, !current.loading, !operations.busy, exact "
                               "model/proxy names; controller global worker-idle API unavailable"},
            {"global_controller_idle", QJsonValue::Null},
            {"expected_count", expectedCount},
            {"expected_names_sha256", QString::fromLatin1(expectedNamesHash)},
            {"timeout_ms", 30000}};
        if (!error.isEmpty())
            report["error"] = error;
        QFile output(parser.value("benchmark"));
        const auto bytes = QJsonDocument(report).toJson();
        if (!output.open(QIODevice::WriteOnly | QIODevice::NewOnly) || output.write(bytes) != bytes.size() ||
            !output.flush()) {
            std::fprintf(stderr, "Measurement output failed: %s\n", qPrintable(output.errorString()));
            app.exit(5);
            return;
        }
        output.close();
        if (exitCode)
            app.exit(exitCode);
        else
            window.close();
    };
    measurement.setTimerType(Qt::PreciseTimer);
    measurement.setInterval(5);
    QObject::connect(&measurement, &QTimer::timeout, &window, [&] {
        const auto beat = heartbeat.elapsed();
        maximumGap = std::max(maximumGap, beat - previousBeat);
        sampleGap = std::max(sampleGap, beat - previousBeat);
        previousBeat = beat;
        const auto *state = window.controller()->current();
        if (!state || state->loading || window.operations()->busy())
            return;
        if (state->error) {
            finish(QStringLiteral("failed"), 3, state->error->message());
            return;
        }
        if (state->location != requested || state->listing.location != requested)
            return;
        QJsonArray names;
        for (int row = 0; row < window.proxy()->rowCount(); ++row)
            names.append(window.proxy()
                             ->data(window.proxy()->index(row, explorer::FileModel::Name), Qt::DisplayRole)
                             .toString());
        const auto digest = QCryptographicHash::hash(QJsonDocument(names).toJson(QJsonDocument::Compact),
                                                     QCryptographicHash::Sha256)
                                .toHex();
        QJsonObject sample{{"kind", samples.isEmpty() ? "first-listing" : "warm-refresh"},
                           {"elapsed_ms", workload.elapsed()},
                           {"count", names.size()},
                           {"names_sha256", QString::fromLatin1(digest)},
                           {"maximum_gui_gap_ms", sampleGap},
                           {"derive_ms", QJsonValue::Null},
                           {"render_ms", QJsonValue::Null}};
        samples.append(sample);
        if (names.size() != expectedCount || window.fileModel()->rowCount() != expectedCount ||
            digest != expectedNamesHash) {
            finish(QStringLiteral("failed"), 4,
                   QStringLiteral("Model/proxy names differ from pinned 1200-entry bulk oracle"));
            return;
        }
        if (samples.size() < 6) {
            sampleGap = 0;
            workload.restart();
            window.controller()->refresh();
        } else
            finish(QStringLiteral("complete"), 0);
    });
    deadline.setSingleShot(true);
    deadline.setTimerType(Qt::PreciseTimer);
    QObject::connect(&deadline, &QTimer::timeout, &window, [&] {
        finish(QStringLiteral("timeout"), 6,
               QStringLiteral("30-second whole-workload deadline exceeded; partial samples retained"));
    });
    deadline.start(30000);
    measurement.start();
    return app.exec();
}
