/* This file is part of KDevelop
    Copyright 2017 René J.V. Bertin <rjvbertin@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <project/abstractfilemanagerplugin.h>
#include <project/projectmodel.h>

#include <tests/autotestshell.h>
#include <tests/testcore.h>
#include <tests/testproject.h>

#include <util/path.h>

#include <KJob>
#include <KDirWatch>

#include <QCoreApplication>
#include <QList>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QMap>
#include <QDebug>
#include <QTextStream>

using namespace KDevelop;

namespace KDevelop {

class AbstractFileManagerPluginImportBenchmark : public QObject
{
    Q_OBJECT
public:
    AbstractFileManagerPluginImportBenchmark(AbstractFileManagerPlugin* manager, const QString& path,
        QObject* parent)
        : QObject(parent)
        , m_out(stdout)
    {
        m_manager = manager;
        m_project = new TestProject(Path(path));
    }

    void start()
    {
        m_projectNumber = s_numBenchmarksRunning++;
        m_out << "Starting import of project " << m_project->path().toLocalFile() << endl;
        m_timer.start();
        auto root = m_manager->import(m_project);
        int elapsed = m_timer.elapsed();
        m_out << "\tcreating dirwatcher took "
            << elapsed / 1000.0 << " seconds" << endl;
        auto import = m_manager->createImportJob(root);
        connect(import, &KJob::finished,
            this, &AbstractFileManagerPluginImportBenchmark::projectImportDone);
        m_timer.restart();
        import->start();
    }

    AbstractFileManagerPlugin* m_manager;
    TestProject* m_project;
    QElapsedTimer m_timer;
    int m_projectNumber;
    QTextStream m_out;

    static int s_numBenchmarksRunning;

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void projectImportDone(KJob* job)
    {
        Q_UNUSED(job);
        int elapsed = m_timer.elapsed();
        m_out << "importing project " << m_projectNumber << " took "
            << elapsed / 1000.0 << " seconds" << endl;

        s_numBenchmarksRunning -= 1;
        if (s_numBenchmarksRunning <= 0) {
            emit finished();
        }
    }

};

int AbstractFileManagerPluginImportBenchmark::s_numBenchmarksRunning = 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        qWarning() << "Usage:" << argv[0] << "projectDir1 [...projectDirN]";
        return 1;
    }
    QCoreApplication app(argc, argv);
    QTextStream qout(stdout);
    // measure the total test time, this provides an indication
    // of overhead and how well multiple projects are imported in parallel
    // (= how different is the total time from the import time of the largest
    // project). When testing a single project the difference between this
    // value and total runtime will provide an estimate of the time required
    // to destroy the dirwatcher.
    QElapsedTimer runTimer;

    AutoTestShell::init({"no plugins"});
    auto core = TestCore::initialize(Core::NoUi);
    auto manager = new AbstractFileManagerPlugin({}, core);

    const char *kdwMethod[] = {"FAM", "Inotify", "Stat", "QFSWatch"};
    qout << "KDirWatch backend: " << kdwMethod[KDirWatch().internalMethod()] << endl;

    QList<AbstractFileManagerPluginImportBenchmark*> benchmarks;

    for (int i = 1 ; i < argc ; ++i) {
        const QString path = QString::fromUtf8(argv[i]);
        if (QFileInfo(path).isDir()) {
            const auto benchmark = new AbstractFileManagerPluginImportBenchmark(manager, path, core);
            benchmarks << benchmark;
            QObject::connect(benchmark, &AbstractFileManagerPluginImportBenchmark::finished,
                             &app, [&runTimer, &qout] {
                                qout << "Done in " << runTimer.elapsed() / 1000.0
                                    << " seconds total\n";
                                QCoreApplication::instance()->quit();
                             });
        }
    }

    if (benchmarks.isEmpty()) {
        qWarning() << "no projects to import (arguments must be directories)";
        return 1;
    }

    runTimer.start();
    for (auto benchmark : benchmarks) {
        benchmark->start();
    }

    return app.exec();
}

#include "abstractfilemanagerpluginimportbenchmark.moc"