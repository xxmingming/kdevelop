/***************************************************************************
 *   Copyright (C) 1999-2001 by Matthias Hoelzer-Kluepfel                  *
 *   hoelzer@kde.org                                                       *
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qapplication.h>
#include <qdir.h>
#include <qfile.h>
#include <qlayout.h>
#include <qtextstream.h>
#include <kaboutdata.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstddirs.h>

#define INDEXER
#include "misc.cpp"
#include "htdigindex.moc"


ProgressDialog::ProgressDialog(QWidget *parent, const char *name)
    : KDialogBase(KDialogBase::Plain, i18n("Generating search index"), Cancel, Cancel,
                  parent, name, false)
{
    proc = 0;
    indexdir = kapp->dirs()->saveLocation("data", "gideon/helpindex");
    QDir d; d.mkdir(indexdir);
    
    QGridLayout *grid = new QGridLayout(plainPage(), 5,3, spacingHint());
    
    QLabel *l = new QLabel(i18n("Scanning for files"), plainPage());
    grid->addMultiCellWidget(l, 0, 0, 1, 2);
    
    filesLabel = new QLabel(plainPage());
    grid->addWidget(filesLabel, 1, 2);
    setFilesScanned(0);
    
    check1 = new QLabel(plainPage());
    grid->addWidget(check1, 0, 0);
    
    l = new QLabel(i18n("Extracting search terms"), plainPage());
    grid->addMultiCellWidget(l, 2,2, 1,2);
    
    bar = new KProgress(plainPage());
    grid->addWidget(bar, 3,2);
    
    check2 = new QLabel(plainPage());
    grid->addWidget(check2, 2,0);
    
    l = new QLabel(i18n("Generating index..."), plainPage());
    grid->addMultiCellWidget(l, 4,4, 1,2);
    
    check3 = new QLabel(plainPage());
    grid->addWidget(check3, 4,0);
    
    setState(0);
    
    setMinimumWidth(300);
}


ProgressDialog::~ProgressDialog()
{}


void ProgressDialog::done(int r)
{
    if (!r)
        proc->kill();
    KDialogBase::done(r);
}


void ProgressDialog::setFilesScanned(int n)
{
    filesLabel->setText(i18n("Files processed: %1").arg(n));
}


void ProgressDialog::setFilesToDig(int n)
{
    bar->setRange(0, n);
}


void ProgressDialog::setFilesDigged(int n)
{
    bar->setValue(n);
}


void ProgressDialog::setState(int n)
{
    QPixmap unchecked = QPixmap(locate("data", "khelpcenter/pics/unchecked.xpm"));
    QPixmap checked = QPixmap(locate("data", "khelpcenter/pics/checked.xpm"));
    
    check1->setPixmap( n > 0 ? checked : unchecked);  
    check2->setPixmap( n > 1 ? checked : unchecked);  
    check3->setPixmap( n > 2 ? checked : unchecked);  
}


void ProgressDialog::addDir(const QString &dir)
{
    QDir d(dir, "*.html", QDir::Name|QDir::IgnoreCase, QDir::Files | QDir::Readable);
    QStringList list = d.entryList();
    
    QStringList::ConstIterator it;
    for ( it=list.begin(); it!=list.end(); ++it ) {
        files.append(dir + "/" + *it);
        setFilesScanned(++filesScanned);
    }
    
    QDir d2(dir, QString::null, QDir::Name|QDir::IgnoreCase, QDir::Dirs);
    QStringList dlist = d2.entryList();
    
    for ( it=dlist.begin(); it != dlist.end(); ++it ) {
        if (*it != "." && *it != "..") {
            addDir(dir + "/" + *it);
            kapp->processEvents();
        }
    }
}


void ProgressDialog::addKdocDir(FILE *f)
{
    char buf[1024];
    while (fgets(buf, sizeof buf, f)) {
        QString s = buf;
        if (s.left(11) == "<BASE URL=\"") {
            int pos2 = s.find("\">", 11);
            if (pos2 != -1) {
                addDir(s.mid(11, pos2-11));
                return;
            }
        }
    }
}


void ProgressDialog::scanDirectories()
{
    bool indexShownLibs, indexHiddenLibs, indexBookmarks;
    DocTreeViewTool::getIndexOptions(&indexShownLibs, &indexHiddenLibs, &indexBookmarks);

    QStringList itemNames, fileNames, hiddenNames;
    DocTreeViewTool::getAllLibraries(&itemNames, &fileNames);
    DocTreeViewTool::getHiddenLibraries(&hiddenNames);

    QStringList::Iterator it1, it2;
    for (it1 = itemNames.begin(), it2 = fileNames.begin();
         it1 != itemNames.end() && it2 != fileNames.end();
         ++it1, ++it2) {
        bool ishidden = hiddenNames.contains(*it2);
        if ( (indexHiddenLibs && ishidden) || (indexShownLibs && !ishidden) ) {
            FILE *f;
            if ((*it2).right(3) != QString::fromLatin1(".gz")) {
                if ( (f = fopen(*it2, "r")) != 0) {
                    addKdocDir(f);
                    fclose(f);
                }
            } else {
                if ( (f = popen(QString("gzip -c -d ")
                                + (*it2) + " 2>/dev/null", "r")) != 0) {
                    addKdocDir(f);
                    pclose(f);
                }
            }
        }
    }
    
    if (indexBookmarks) {
        QStringList bookmarksTitle, bookmarksURL;
        DocTreeViewTool::getBookmarks(&bookmarksTitle, &bookmarksURL);
        QStringList::Iterator it3;
        for (it3 = bookmarksURL.begin(); it3 != bookmarksURL.end(); ++it3) {
            // FIXME: Perhaps one should consider indexing the whole directory the file
            // lives in
            files.append(*it3);
            setFilesScanned(++filesScanned);
        }
    }
}


bool ProgressDialog::createConfig()
{
    // locate the common dir
    QString language = KGlobal::locale()->language();
    if (language == "C")
        language = "en";

    QString wrapper = locate("data", QString("khelpcenter/%1/wrapper.html").arg(language));
    if (wrapper.isEmpty())
        wrapper = locate("data", QString("khelpcenter/en/wrapper.html"));
    if (wrapper.isEmpty())
        return false;
    wrapper = wrapper.left(wrapper.length()-12);
    
    // locate the image dir
    QString images = locate("data", "khelpcenter/pics/star.png");
    if (images.isEmpty())
        return false;
    images = images.left(images.length()-8);

    QFile f(indexdir + "/htdig.conf");
    if (f.open(IO_WriteOnly)) {
        QTextStream ts(&f);
        
        ts << "database_dir:\t\t" << indexdir << endl;
        ts << "start_url:\t\t`" << indexdir << "/files`" << endl;
        ts << "local_urls:\t\thttp://localhost/=/" << endl;
        ts << "local_urls_only:\ttrue" << endl;
        ts << "maximum_pages:\t\t1" << endl;
        ts << "image_url_prefix:\t\t" << images << endl;
        ts << "star_image:\t\t" << images << "star.png" << endl;
        ts << "star_blank:\t\t" << images << "star_blank.png" << endl;
        ts << "compression_level:\t\t6" << endl;
        ts << "max_hop_count:\t\t0" << endl;
        
        ts << "search_results_wrapper:\t" << wrapper << "wrapper.html" << endl;
        ts << "nothing_found_file:\t" << wrapper << "nomatch.html" << endl;
        ts << "syntax_error_file:\t" << wrapper << "syntax.html" << endl;
        ts << "bad_word_list:\t\t" << wrapper << "bad_words" << endl;
        
        f.close();
        return true;
    }
    
    return false;
}


#define CHUNK_SIZE 100


bool ProgressDialog::generateIndex()
{
    setState(1);

    // run htdig
    KConfig config("khelpcenterrc", true);
    config.setGroup("htdig");
    QString exe = config.readEntry("htdig", kapp->dirs()->findExe("htdig"));
    if (exe.isEmpty())
        return false;

    bool initial = true;
    bool done = false;
    int  count = 0;
    
    filesToDig = files.count();
    setFilesToDig(filesToDig);
    filesDigged = 0;
    
    //    QDir d; d.mkdir(indexdir);
    
    while (!done) {
        // kill old process
        delete proc;
        
        // prepare new process
        proc = new KProcess();
        *proc << exe << "-c" << (indexdir + "/htdig.conf");
        if (initial) {
            *proc << "-i";
            initial = false;
        }
        
        kdDebug() << "Running htdig" << endl;
        
        //      connect(_proc, SIGNAL(receivedStdout(KProcess *,char*,int)),
        //	      this, SLOT(htdigStdout(KProcess *,char*,int)));
        connect(proc, SIGNAL(processExited(KProcess *)),
                this, SLOT(htdigExited(KProcess *)));
        
        htdigRunning = true;
        
        // write out file
        QFile f(indexdir+"/files");
        if (!f.open(IO_WriteOnly)) {
            kdDebug() << "Could not open `files` for writing" << endl;
            return false;
	}
        
        QTextStream ts(&f);
        for (int i=0; i<CHUNK_SIZE; ++i, ++count) {
            if (count >= filesToDig) {
                done = true;
                break;
            }
            ts << "http://localhost/" + files[count] << endl;
        }
        
        // execute htdig
        proc->start(KProcess::NotifyOnExit, KProcess::Stdout);      
        while (htdigRunning && proc->isRunning())
            kapp->processEvents();
        
        if (!proc->normalExit() || proc->exitStatus() != 0) {
            delete proc;
            return false;
        }
        
        filesDigged += CHUNK_SIZE;
        setFilesDigged(filesDigged);
        kapp->processEvents();
    }

    setState(2);

    // run htmerge -----------------------------------------------------
    exe = config.readEntry("htmerge", kapp->dirs()->findExe("htmerge"));
    if (exe.isEmpty())
        return false;
    
    delete proc;
    proc = new KProcess();
    *proc << exe << "-c" << (indexdir + "/htdig.conf");

    kdDebug() << "Running htmerge" << endl;
    
    connect(proc, SIGNAL(processExited(KProcess *)),
            this, SLOT(htmergeExited(KProcess *)));
    
    htmergeRunning = true;
    
    proc->start(KProcess::NotifyOnExit, KProcess::Stdout);
    
    while (htmergeRunning && proc->isRunning())
        kapp->processEvents();
    
    if (!proc->normalExit() || proc->exitStatus() != 0) {
        delete proc;
        return false;
    }

    delete proc;
    
    setState(3);
    kapp->processEvents();
    
    return true;
}



void ProgressDialog::htdigStdout(KProcess *, char *buffer, int len)
{
    QString line = QString(buffer).left(len);
    
    int cnt=0, index=-1;
    while ( (index = line.find("http://", index+1)) > 0)
        cnt++;
    filesDigged += cnt;
    
    cnt=0, index=-1;
    while ( (index = line.find("not changed", index+1)) > 0)
        cnt++;
    filesDigged -= cnt;
    
    setFilesDigged(filesDigged);
}


void ProgressDialog::htdigExited(KProcess *)
{
    kdDebug() << "htdig terminated" << endl;
    htdigRunning = false;
}


void ProgressDialog::htmergeExited(KProcess *)
{
    kdDebug() << "htmerge terminated" << endl;
    htmergeRunning = false;
}


int main(int argc, char *argv[])
{
#if 0
    static KCmdLineOptions options[] = {
        { "+dirs", I18N_NOOP("The directories to index."), 0 },
        { 0, 0, 0 }
    };
#endif
    
    KAboutData aboutData("kdevdoctreeview", I18N_NOOP("Gideon"),
                         "0.1", I18N_NOOP("KDE Index generator for help files."));
    
    KCmdLineArgs::init(argc, argv, &aboutData);
    //    KCmdLineArgs::addCmdLineOptions(options);
    
    KGlobal::locale()->setMainCatalogue("htmlsearch");
    KApplication app;

    ProgressDialog *search = new ProgressDialog(0, "progress dialog");
    search->show();
    kapp->processEvents();
    QApplication::syncX();
    
    //    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    // Assemble directory list
    //    for (int i=0; i < args->count(); ++i)
    //        search->scanDir(args->arg(i));
    search->scanDirectories();

    // Write htdig.conf file
    if (!search->createConfig())
        return 1;

    // Do it :-)
    search->generateIndex();
    
    return 0;
}
