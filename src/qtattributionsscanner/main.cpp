// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsongenerator.h"
#include "logging.h"
#include "packagefilter.h"
#include "qdocgenerator.h"
#include "scanner.h"

#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>

#include <iostream>


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName(QStringLiteral("Qt Attributions Scanner"));
    a.setApplicationVersion(QStringLiteral("1.1"));

    QCommandLineParser parser;
    parser.setApplicationDescription(tr("Processes attribution files in Qt sources."));
    parser.addPositionalArgument(QStringLiteral("path"),
                                 tr("Path to a qt_attribution.json/README.chromium file, "
                                    "or a directory to be scannned recursively."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption generatorOption(QStringLiteral("output-format"),
                                       tr("Output format (\"qdoc\", \"json\")."),
                                       QStringLiteral("generator"),
                                       QStringLiteral("qdoc"));
    QCommandLineOption inputFormatOption(QStringLiteral("input-files"),
                                       tr("Input files (\"qt_attributions\" scans for qt_attribution.json, "
                                          "\"chromium_attributions\" for README.Chromium, \"all\" for both)."),
                                       QStringLiteral("input_format"),
                                       QStringLiteral("qt_attributions"));
    QCommandLineOption filterOption(QStringLiteral("filter"),
                                    tr("Filter packages according to <filter> (e.g. QDocModule=qtcore)"),
                                    QStringLiteral("expression"));
    QCommandLineOption baseDirOption(QStringLiteral("basedir"),
                                     tr("Paths in documentation are made relative to this "
                                        "directory."),
                                     QStringLiteral("directory"));
    QCommandLineOption outputOption({ QStringLiteral("o"), QStringLiteral("output") },
                                    tr("Write generated data to <file>."),
                                    QStringLiteral("file"));
    QCommandLineOption verboseOption(QStringLiteral("verbose"),
                                     tr("Verbose output."));
    QCommandLineOption silentOption({ QStringLiteral("s"), QStringLiteral("silent") },
                                    tr("Minimal output."));

    parser.addOption(generatorOption);
    parser.addOption(inputFormatOption);
    parser.addOption(filterOption);
    parser.addOption(baseDirOption);
    parser.addOption(outputOption);
    parser.addOption(verboseOption);
    parser.addOption(silentOption);

    parser.process(a.arguments());

    LogLevel logLevel = NormalLog;
    if (parser.isSet(verboseOption) && parser.isSet(silentOption)) {
        std::cerr << qPrintable(tr("--verbose and --silent cannot be set simultaneously.")) << std::endl;
        parser.showHelp(1);
    }

    if (parser.isSet(verboseOption))
        logLevel = VerboseLog;
    else if (parser.isSet(silentOption))
        logLevel = SilentLog;

    if (parser.positionalArguments().size() != 1)
        parser.showHelp(2);

    const QString path = parser.positionalArguments().constLast();

    QString inputFormat = parser.value(inputFormatOption);
    Scanner::InputFormats formats;
    if (inputFormat == QLatin1String("qt_attributions"))
        formats = Scanner::InputFormat::QtAttributions;
    else if (inputFormat == QLatin1String("chromium_attributions"))
        formats = Scanner::InputFormat::ChromiumAttributions;
    else if (inputFormat == QLatin1String("all"))
        formats = Scanner::InputFormat::QtAttributions | Scanner::InputFormat::ChromiumAttributions;
    else {
        std::cerr << qPrintable(tr("%1 is not a valid input-files argument").arg(inputFormat)) << std::endl << std::endl;
        parser.showHelp(8);
    }

    // Parse the attribution files
    QList<Package> packages;
    const QFileInfo pathInfo(path);
    if (pathInfo.isDir()) {
        if (logLevel == VerboseLog)
            std::cerr << qPrintable(tr("Recursively scanning %1 for attribution files...").arg(
                                        QDir::toNativeSeparators(path))) << std::endl;
        packages = Scanner::scanDirectory(path, formats, logLevel);
    } else if (pathInfo.isFile()) {
        packages = Scanner::readFile(path, logLevel);
    } else {
        std::cerr << qPrintable(tr("%1 is not a valid file or directory.").arg(
                                    QDir::toNativeSeparators(path))) << std::endl << std::endl;
        parser.showHelp(7);
    }

    // Apply the filter
    if (parser.isSet(filterOption)) {
        PackageFilter filter(parser.value(filterOption));
        if (filter.type == PackageFilter::InvalidFilter)
            return 4;
        packages.erase(std::remove_if(packages.begin(), packages.end(),
                                      [&filter](const Package &p) { return !filter(p); }),
                       packages.end());
    }

    if (logLevel == VerboseLog)
        std::cerr << qPrintable(tr("%1 packages found.").arg(packages.size())) << std::endl;

    // Prepare the output text stream
    QTextStream out(stdout);
    QFile outFile(parser.value(outputOption));
    if (!outFile.fileName().isEmpty()) {
        if (!outFile.open(QFile::WriteOnly)) {
            std::cerr << qPrintable(tr("Cannot open %1 for writing.").arg(
                                        QDir::toNativeSeparators(outFile.fileName())))
                      << std::endl;
            return 5;
        }
        out.setDevice(&outFile);
    }

    // Generate the output and write it
    QString generator = parser.value(generatorOption);
    if (generator == QLatin1String("qdoc")) {
        QString baseDirectory = parser.value(baseDirOption);
        if (baseDirectory.isEmpty()) {
            if (pathInfo.isDir()) {
                // include top level module name in printed paths
                baseDirectory = pathInfo.dir().absoluteFilePath(QStringLiteral(".."));
            } else {
                baseDirectory = pathInfo.absoluteDir().absoluteFilePath(QStringLiteral(".."));
            }
        }

        QDocGenerator::generate(out, packages, baseDirectory, logLevel);
    } else if (generator == QLatin1String("json")) {
        JsonGenerator::generate(out, packages, logLevel);
    } else {
        std::cerr << qPrintable(tr("Unknown output-format %1.").arg(generator)) << std::endl;
        return 6;
    }

    if (logLevel == VerboseLog)
        std::cerr << qPrintable(tr("Processing is done.")) << std::endl;

    return 0;
}
