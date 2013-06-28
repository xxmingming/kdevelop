/*
 * GDB Debugger Support
 *
 * Copyright 2000 John Birch <jbb@kdevelop.org>
 * Copyright 2006 Vladimir Prus  <ghost@cs.msu.su>
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "disassemblewidget.h"
#include "gdbcommand.h"
#include "debuggerplugin.h"

#include <kdebug.h>
#include <kdeversion.h>
#include <ktextedit.h>
#include <kglobalsettings.h>

#include <QShowEvent>
#include <QHideEvent>
#include <QAction>
#include <QMenu>
#include <QBoxLayout>
#include <QComboBox>
#include <QPushButton>

#include <stdlib.h>
#include <klocale.h>

#include <interfaces/icore.h>
#include <interfaces/idebugcontroller.h>
#include <debugger/interfaces/idebugsession.h>
#include "debugsession.h"

using namespace GDBMI;

namespace GDBDebugger
{

SelectAddrDialog::SelectAddrDialog(QWidget* parent)
    : KDialog(parent)
{
    QWidget *widget = new QWidget;
    m_ui.setupUi(widget);
    setMainWidget(widget);
    setCaption(i18n("Address Selector"));
    
    connect(m_ui.comboBox, SIGNAL(editTextChanged(QString)), 
            this, SLOT(validateInput()) );
    connect(m_ui.comboBox, SIGNAL(returnPressed()),
            this, SLOT(itemSelected()) );
    connect(this, SIGNAL(okClicked()), this, SLOT(itemSelected()) );
}

bool SelectAddrDialog::hasValidAddress() const
{
    bool ok;
    m_ui.comboBox->currentText().toLongLong(&ok, 16);

    return ok;
}

void SelectAddrDialog::updateOkState()
{
    enableButtonOk( hasValidAddress() );
}

void SelectAddrDialog::validateInput()
{
    updateOkState();
}

void SelectAddrDialog::itemSelected()
{
    QString text = m_ui.comboBox->currentText();
    if( hasValidAddress() && m_ui.comboBox->findText(text) < 0 )
        m_ui.comboBox->addItem(text);
}



DisassembleWindow::DisassembleWindow(QWidget *parent)
    : QTreeWidget(parent)
{
    /*context menu commands */{
    m_selectAddrAction = new QAction(i18n("Change &address"), this);
    m_selectAddrAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_selectAddrAction, SIGNAL(triggered()), this->parent(), SLOT(slotChangeAddress()));

    m_jumpToLocation = new QAction(KIcon("debug-execute-to-cursor"), i18n("&Jump to Cursor"), this);
    m_jumpToLocation->setWhatsThis(i18n("Sets the execution pointer to the current cursor position."));
    connect(m_jumpToLocation,SIGNAL(triggered()), this->parent(), SLOT(jumpToCursor()));
    }
}

void DisassembleWindow::contextMenuEvent(QContextMenuEvent *e)
{
        QMenu popup(this);
        popup.addAction(m_selectAddrAction);
        popup.addAction(m_jumpToLocation);
        popup.exec(e->globalPos());
}
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
const KIcon DisassembleWidget::icon_=KIcon("go-next");

DisassembleWidget::DisassembleWidget(CppDebuggerPlugin* plugin, QWidget *parent)
        : QWidget(parent),
        active_(false),
        lower_(0),
        upper_(0),
        address_(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    
    {   // initialize controls
        QHBoxLayout* controlsLayout = new QHBoxLayout;
        QVBoxLayout* startLayout = new QVBoxLayout;
        QVBoxLayout* endLayout = new QVBoxLayout;

        QLabel* startAddr = new QLabel(i18n("Start address:"), this);
        QLabel* endAddr   = new QLabel(i18n("End Address:"), this);
        
        m_startAddress = new QComboBox(this);
        m_startAddress->setEditable(true);
        m_startAddress->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_startAddress->setMinimumContentsLength( 2+2*sizeof(void*) );
        m_startAddress->setInsertPolicy(QComboBox::InsertAtTop);
        
        m_endAddress = new QComboBox(this);
        m_endAddress->setEditable(true);
        m_endAddress->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_endAddress->setMinimumContentsLength( 2+2*sizeof(void*) );
        m_endAddress->setInsertPolicy(QComboBox::InsertAtTop);
        
        m_evalButton = new QPushButton(i18nc("@action:button", "Display"), this);

        startLayout->addWidget(startAddr);
        startLayout->addWidget(m_startAddress);
        endLayout->addWidget(endAddr);
        endLayout->addWidget(m_endAddress);

        controlsLayout->addLayout(startLayout);
        controlsLayout->addLayout(endLayout);

        controlsLayout->addWidget(m_evalButton);
        controlsLayout->addStretch(0);

        topLayout->addSpacing(5);
        topLayout->addLayout(controlsLayout);

        connect(m_startAddress, SIGNAL(editTextChanged(QString)), 
                this, SLOT(slotValidateEdits()) );
        connect(m_endAddress, SIGNAL(editTextChanged(QString)), 
                this, SLOT(slotValidateEdits()) );

        connect(m_evalButton, SIGNAL(clicked(bool)),
                this, SLOT(slotShowAddrRange()) );
    }

    
    {   // initialize disasm view
        m_disassembleWindow = new DisassembleWindow(this);

        m_disassembleWindow->setWhatsThis(i18n("<b>Machine code display</b><p>"
                        "A machine code view into your running "
                        "executable with the current instruction "
                        "highlighted. You can step instruction by "
                        "instruction using the debuggers toolbar "
                        "buttons of \"step over\" instruction and "
                        "\"step into\" instruction."));

        m_disassembleWindow->setFont(KGlobalSettings::fixedFont());
        m_disassembleWindow->setSelectionMode(QTreeWidget::SingleSelection);
        m_disassembleWindow->setColumnCount(ColumnCount);
        m_disassembleWindow->setUniformRowHeights(true);
        m_disassembleWindow->setRootIsDecorated(false);

        m_disassembleWindow->setHeaderLabels(QStringList() << "" << i18n("Address") << i18n("Function")
            << i18n("Offset") << i18n("Instruction"));

        topLayout->addWidget(m_disassembleWindow);
        topLayout->setStretchFactor(m_disassembleWindow, 1);
        topLayout->setMargin(0);
    }
    
    setLayout(topLayout);
    
    setWindowIcon( KIcon("system-run") );
    setWindowTitle(i18n("Disassemble View"));
    
    KDevelop::IDebugController* pDC=KDevelop::ICore::self()->debugController();
    Q_ASSERT(pDC);
    
    connect(pDC, 
            SIGNAL(currentSessionChanged(KDevelop::IDebugSession*)),
            SLOT(currentSessionChanged(KDevelop::IDebugSession*)));

    connect(plugin, SIGNAL(reset()), this, SLOT(slotDeactivate()));
    
    m_dlg = new SelectAddrDialog(this);
    
    // show the data if debug session is active
    KDevelop::IDebugSession* pS = pDC->currentSession();

    currentSessionChanged(pS);
    
    if(pS && !pS->currentAddr().isEmpty())
        slotShowStepInSource(pS->currentUrl(), pS->currentLine(), pS->currentAddr());
}

void DisassembleWidget::jumpToCursor() {
    DebugSession *s = qobject_cast<DebugSession*>(KDevelop::ICore::
            self()->debugController()->currentSession());
    if (s && s->isRunning()) {
        kDebug() <<  m_disassembleWindow->selectedItems().size();
        QString address = m_disassembleWindow->selectedItems().at(0)->text(1);
        s->jumpToMemoryAddress(address);
    }
}


void DisassembleWidget::currentSessionChanged(KDevelop::IDebugSession* s)
{
    DebugSession *session = qobject_cast<DebugSession*>(s);
    
    enableControls( session != NULL ); // disable if session closed

    if (!session) return;

    connect(session, SIGNAL(showStepInSource(KUrl,int,QString)),
                SLOT(slotShowStepInSource(KUrl,int,QString)));
}


/***************************************************************************/

DisassembleWidget::~DisassembleWidget()
{}

/***************************************************************************/

bool DisassembleWidget::displayCurrent()
{
    if(address_ < lower_ || address_ > upper_) return false;

    bool bFound=false;
    for (int line=0; line < m_disassembleWindow->topLevelItemCount(); line++)
    {
        QTreeWidgetItem* item = m_disassembleWindow->topLevelItem(line);
        unsigned long address = strtoul(item->text(Address).toLatin1(), 0, 0);

        if (address == address_)
        {
            // put cursor at start of line and highlight the line
            m_disassembleWindow->setCurrentItem(item);
            item->setIcon(Icon, icon_);
            bFound = true;  // need to process all items to clear icons
        }
        else if(!item->icon(Icon).isNull()) item->setIcon(Icon, QIcon());
    }

    return bFound;
}

/***************************************************************************/

void DisassembleWidget::slotActivate(bool activate)
{
    kDebug(9012) << "Disassemble widget active: " << activate;

    if (active_ != activate)
    {
        active_ = activate;
        if (active_)
        {
            if (address_ < lower_ || address_ > upper_ || !displayCurrent())
                disassembleMemoryRegion();
        }
    }
}

/***************************************************************************/

void DisassembleWidget::slotShowStepInSource(   const KUrl &, int,
                                                const QString &currentAddress)
{
    address_ = strtoul(currentAddress.toLatin1(), 0, 0);
    if (!active_)
        return;

    if (address_ < lower_ || address_ > upper_ || !displayCurrent())
        disassembleMemoryRegion();
}

void DisassembleWidget::updateExecutionAddressHandler(const GDBMI::ResultRecord& r)
{
    const GDBMI::Value& content = r["asm_insns"];
    const GDBMI::Value& pc = content[0];
    if( pc.hasField("address") ){
        QString addr = pc["address"].literal();
        address_ = strtoul(addr.toLatin1(), 0, 0);

        disassembleMemoryRegion(addr);
    }
}

void DisassembleWidget::slotShowAddrRange()
{
    if( !hasValidAddrRange() ) return;
    
    QString addr1 = m_startAddress->currentText();
    QString addr2 = m_endAddress->currentText();
    
    if ( m_startAddress->findText(addr1) < 0 ) m_startAddress->addItem(addr1);
    if ( m_endAddress->findText(addr2) < 0 ) m_endAddress->addItem(addr2);
    
    disassembleMemoryRegion(addr1, addr2);
}


/***************************************************************************/

void DisassembleWidget::disassembleMemoryRegion(const QString& from, const QString& to)
{
    DebugSession *s = qobject_cast<DebugSession*>(KDevelop::ICore::
            self()->debugController()->currentSession());
    if(!s || !s->isRunning()) return;

    if (from.isEmpty()){
        s->addCommandToFront(
                    new GDBCommand(DataDisassemble, "-s \"$pc\" -e \"$pc+1\" -- 0", this, &DisassembleWidget::updateExecutionAddressHandler ) );
    }else{

        QString cmd = (to.isEmpty())?
        QString("-s %1 -e \"%1 + 256\" -- 0").arg(from ):
        QString("-s %1 -e %2+1 -- 0").arg(from).arg(to); // if both addr set
        
        s->addCommandToFront(
        new GDBCommand(DataDisassemble, cmd, this, &DisassembleWidget::disassembleMemoryHandler ) );

   }
}

/***************************************************************************/

void DisassembleWidget::disassembleMemoryHandler(const GDBMI::ResultRecord& r)
{
  const GDBMI::Value& content = r["asm_insns"];
  QString rawdata;

  m_disassembleWindow->clear();

  for(int i = 0; i < content.size(); ++i)
  {
    const GDBMI::Value& line = content[i];

    QString addr, fct, offs, inst;
    
    if( line.hasField("address") )   addr = line["address"].literal();
    if( line.hasField("func-name") ) fct  = line["func-name"].literal();
    if( line.hasField("offset") )    offs = line["offset"].literal();
    if( line.hasField("inst") )      inst = line["inst"].literal();

    m_disassembleWindow->addTopLevelItem(new QTreeWidgetItem(m_disassembleWindow,
                    QStringList() << QString() << addr << fct << offs << inst));

    if (i == 0) {
      lower_ = strtoul(addr.toLatin1(), 0, 0);
    } else  if (i == content.size()-1) {
      upper_ = strtoul(addr.toLatin1(), 0, 0);
    }
  }
  
  // show addresses
  int i = 0;
  
  while ( i < content.size() && !content[i].hasField("address") )
      ++i; // find first with address set
  
  if ( i < content.size() ) m_startAddress->setEditText(content[i]["address"].literal());  
  
  i = content.size() - 1;
  
  while ( i > 0 && !content[i].hasField("address") )
      --i; // find last with address set
      
  if ( i > 0 ) m_endAddress->setEditText(content[i]["address"].literal());

  displayCurrent();

  m_disassembleWindow->resizeColumnToContents(Icon);       // make Icon always visible
  m_disassembleWindow->resizeColumnToContents(Address);    // make entire address always visible
}


void DisassembleWidget::showEvent(QShowEvent*)
{
    slotActivate(true);

    for (int i = 0; i < m_disassembleWindow->model()->columnCount(); ++i)
        m_disassembleWindow->resizeColumnToContents(i);
}

void DisassembleWidget::hideEvent(QHideEvent*)
{
    slotActivate(false);
}

void DisassembleWidget::slotDeactivate()
{
    slotActivate(false);
}

bool DisassembleWidget::hasValidAddrRange()
{
    bool ok;
    qlonglong start = m_startAddress->currentText().toLongLong(&ok, 16);
    if(!ok) return false;
    
    qlonglong end = m_endAddress->currentText().toLongLong(&ok, 16);
    if(!ok) return false;
    
    if( start < end && end-start < 128*1024 ) // don't show more than 128K at once
        return true;
    else
        return false;
}

void DisassembleWidget::slotValidateEdits()
{
    m_evalButton->setEnabled( active_ && hasValidAddrRange() );
}

void DisassembleWidget::enableControls(bool enabled)
{
    m_startAddress->setEnabled(enabled);
    m_endAddress->setEnabled(enabled);
    m_evalButton->setEnabled(enabled && hasValidAddrRange());
    m_disassembleWindow->setEnabled(enabled);
}

void DisassembleWidget::slotChangeAddress()
{
    if(!m_dlg) return;
    m_dlg->updateOkState();
    
    if( m_dlg->exec() == KDialog::Rejected) return;

    unsigned long addr = strtoul(m_dlg->getAddr().toLatin1(), 0, 0);

    if (addr < lower_ || addr > upper_ || !displayCurrent())
        disassembleMemoryRegion(m_dlg->getAddr());
}
    
/***************************************************************************/

}

#include "disassemblewidget.moc"
