#ifndef __PARTCONTROLLER_H__
#define __PARTCONTROLLER_H__


#include <qwidget.h>
#include <qptrlist.h>


class QTabWidget;


#include <kurl.h>

namespace KParts
{
  class Part;
  class Factory;
  class PartManager;
};

class KAction;


#include "kdevpartcontroller.h"


class PartListEntry;

class PartController : public KDevPartController
{
  Q_OBJECT

public:

  PartController(QWidget *toplevel);

  void editDocument(const KURL &url, int lineNum=-1);
  void showDocument(const KURL &url, int lineNum=-1);

  bool closeDocuments(const QStringList &list);

  static void createInstance(QWidget *parent);
  static PartController *getInstance();

  void saveAllFiles();
  void revertAllFiles();


public slots:

  void slotCurrentChanged(QWidget *w);


protected:

  PartController(QWidget *parent, QWidget *mainwindow, const char *name=0);
  ~PartController();


private slots:

  void slotPartAdded(KParts::Part *part);
  void slotPartRemoved(KParts::Part *part);
  void slotActivePartChanged(KParts::Part *part);

  void slotBufferSelected();
  void slotSaveAllFiles();
  void slotRevertAllFiles();

  void slotOpenFile();

  void slotCloseWindow();
  void slotCloseAllWindows();
  void slotCloseOtherWindows();

  void slotPopupAboutToShow();
  void slotPopupAboutToHide();

private:

  void setLineNumber(int lineNum);

  void setupActions();

  void closeActivePart();
  void closePart(KParts::Part *part);

  KParts::Factory *findPartFactory(const QString &mimeType, const QString &partType);

  void integratePart(KParts::Part *part, const KURL &url);

  KParts::Part *partForURL(const KURL &url);
  void activatePart(KParts::Part *part);

  void updateMenuItems();
  void updateBufferMenu();

  void editText(const KURL &url, int num);

  QTabWidget *m_tabWidget;

  static PartController *s_instance;

  KParts::PartManager *m_partManager;

  QPtrList<PartListEntry> m_partList;

  KAction *m_closeWindowAction, *m_saveAllFilesAction, *m_revertAllFilesAction;
  KAction *m_closeAllWindowsAction, *m_closeOtherWindowsAction;

};



#endif
