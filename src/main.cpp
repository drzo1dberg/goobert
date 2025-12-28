#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>
#include <iostream>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Goobert");
    app.setApplicationVersion(GOOBERT_VERSION);
    app.setOrganizationName("drzo1dberg");

    // Dark palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(26, 26, 26));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(42, 42, 42));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(58, 58, 58));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(darkPalette);

    // Command line parsing
    QCommandLineParser parser;
    parser.setApplicationDescription("Goobert - Video Wall for MPV");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("source", "Source directory containing media files");

    QCommandLineOption broadcastOption(
        "broadcast",
        "Broadcast action to all instances (next|shuffle)",
        "action"
    );
    parser.addOption(broadcastOption);

    parser.process(app);

    // Handle broadcast mode
    if (parser.isSet(broadcastOption)) {
        QString action = parser.value(broadcastOption);
        // TODO: Implement broadcast via Unix sockets
        std::cout << "Broadcast: " << action.toStdString() << std::endl;
        return 0;
    }

// Get source directory
QString sourceDir;
const QStringList args = parser.positionalArguments();

if (!args.isEmpty()) {
    sourceDir = args.first();
} else {
    QStringList defaultPaths = {
        "/storage/media02/m02",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        QDir::homePath() + "/Videos",
        QDir::homePath() + "/Movies"
    };

    for (const QString &path : defaultPaths) {
        if (QDir(path).exists()) {
            sourceDir = path;
            break;
        }
    }
}

    MainWindow window(sourceDir);
    window.show();

    return app.exec();
}
