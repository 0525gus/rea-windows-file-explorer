#include "explorer/main_window.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

int main(int argc, char **argv) {
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Explorer A"));
    application.setStyle(QStringLiteral("Fusion"));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("path"),
                                 QStringLiteral("Initial local folder; defaults to This PC."));
    parser.addOption(
        {QStringLiteral("render"),
         QStringLiteral("Render this application's window to a PNG and exit (offscreen supported)."),
         QStringLiteral("png")});
    parser.addOption({QStringLiteral("smoke"), QStringLiteral("Exit after a successful initial listing.")});
    parser.process(application);
    explorer::MainWindow window;
    if (!parser.isSet(QStringLiteral("render")) && !parser.isSet(QStringLiteral("smoke")))
        window.show();
    const auto args = parser.positionalArguments();
    const auto initial = args.isEmpty() ? explorer::Location{} : explorer::Location::folder(args.front());
    bool finished = false;
    QObject::connect(
        window.controller(), &explorer::BrowserController::tabChanged, &application, [&](const QUuid &) {
            const auto *state = window.controller()->current();
            if (!state || state->loading || finished)
                return;
            if (!parser.isSet(QStringLiteral("render")) && !parser.isSet(QStringLiteral("smoke")))
                return;
            finished = true;
            QTimer::singleShot(100, &application, [&] {
                const bool ok = !window.controller()->current()->error &&
                                (!parser.isSet(QStringLiteral("render")) ||
                                 window.grab().save(parser.value(QStringLiteral("render"))));
                application.exit(ok ? 0 : 1);
            });
        });
    QTimer::singleShot(15000, &application, [&] {
        if (parser.isSet(QStringLiteral("render")) || parser.isSet(QStringLiteral("smoke")))
            application.exit(2);
    });
    window.openInitial(initial);
    return application.exec();
}
