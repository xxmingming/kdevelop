/***************************************************************************
                          kdevarghint.cpp  -  description
                             -------------------
	begin		: Sam Jul 14 18:20:00 CEST 2001
	copyright	: (C) 2001 by Victor R�der
	email		: Victor_Roeder@GMX.de
 ***************************************************************************/

/******** Partly based on the ArgHintWidget of Qt3 by Trolltech AS *********/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>

#include <kdebug.h>

#include "codecompletion_arghint.h"

static const char* left_xpm[] = {
"16 16 3 1",
"a c #000000",
"# c #c5c2c5",
". c #ffffee",
"................",
"................",
"...########.....",
"...########.....",
"...#####a##.....",
"...####aa##.....",
"...###aaa##.....",
"...##aaaa##.....",
"...##aaaa##.....",
"...###aaa##.....",
"...####aa##.....",
"...#####a##.....",
"...########.....",
"...########.....",
"................",
"................"};

static const char* right_xpm[] = {
"16 16 3 1",
"a c #000000",
"# c #c5c2c5",
". c #ffffee",
"................",
"................",
".....########...",
".....########...",
".....##a#####...",
".....##aa####...",
".....##aaa###...",
".....##aaaa##...",
".....##aaaa##...",
".....##aaa###...",
".....##aa####...",
".....##a#####...",
".....########...",
".....########...",
"................",
"................"};


ArgHintArrow::ArgHintArrow ( QWidget *parent, Dir d )
    : QButton( parent, 0, WStyle_NormalBorder )
{
	setFixedSize ( 16, 16 );

	if ( d == Left )
		pix = QPixmap ( left_xpm );
	else
		pix = QPixmap ( right_xpm );
}

void ArgHintArrow::drawButton ( QPainter *p )
{
	if ( isEnabled() )
		p->drawPixmap ( 0, 0, pix );
}


KDevArgHint::KDevArgHint ( QWidget *parent ) : QFrame ( parent, 0,  WType_Popup )
{
	setFrameStyle ( QFrame::Box | QFrame::Plain );
	setLineWidth ( 1 );
	setBackgroundColor ( QColor ( 255, 255, 238 ) );
	
	QHBoxLayout* hbox = new QHBoxLayout ( this );
	hbox->setMargin ( 1 );

	hbox->addWidget ( ( m_pPrev = new ArgHintArrow ( this , ArgHintArrow::Left ) ) );
	hbox->addWidget ( ( m_pStateLabel = new QLabel ( this ) ) );
	hbox->addWidget ( ( m_pNext = new ArgHintArrow ( this, ArgHintArrow::Right ) ) );
	hbox->addWidget ( ( m_pFuncLabel = new QLabel ( this ) ) );

	setFocusPolicy ( StrongFocus );
	setFocusProxy ( parent );

	m_pStateLabel->setBackgroundColor ( QColor ( 255, 255, 238 ) );
	m_pStateLabel->setAlignment ( AlignCenter );
	m_pStateLabel->setFont ( QFont ( "Arial", 9 ) );
	m_pFuncLabel->setBackgroundColor ( QColor ( 255, 255, 238 ) );
	m_pFuncLabel->setAlignment ( AlignCenter );
	m_pFuncLabel->setFont ( QFont ( "Arial", 9 ) );

	m_pStateLabel->setFixedSize ( 30, 16 );

	connect ( m_pPrev, SIGNAL ( clicked() ), this, SLOT ( gotoPrev() ) );
	connect ( m_pNext, SIGNAL ( clicked() ), this, SLOT ( gotoNext() ) );

	m_nNumFunc = m_nCurFunc = m_nCurLine = 0;

	m_nCurArg = 1;

	m_bMarkingEnabled = false;

	updateState();
}

KDevArgHint::~KDevArgHint()
{
	delete m_pPrev;
	delete m_pNext;
	delete m_pStateLabel;
	delete m_pFuncLabel;
}

/** No descriptions */
void KDevArgHint::gotoPrev()
{
	if ( m_nCurFunc > 0 )
		m_nCurFunc--;
	else
		m_nCurFunc = m_nNumFunc - 1;

	updateState();
}

/** No descriptions */
void KDevArgHint::gotoNext()
{
	if ( m_nCurFunc < m_nNumFunc - 1 )
		m_nCurFunc++;
	else
		m_nCurFunc = 0;

	updateState();
}

/** No descriptions */
void KDevArgHint::updateState()
{
	QString strState;
	strState = strState.sprintf ( i18n (  "%i of %i" ), m_nCurFunc + 1, m_nNumFunc );

	m_pStateLabel->setText ( strState );

	m_pFuncLabel->setText ( markCurArg() );

	if ( m_nNumFunc <= 1 )
	{
		m_pPrev->hide();
		m_pNext->hide();
		m_pStateLabel->hide();
	}
	else
	{
		m_pPrev->show();
		m_pNext->show();
		m_pStateLabel->show();
	}
}

void KDevArgHint::reset()
{
	m_funcList.clear();

	m_nNumFunc = m_nCurFunc = m_nCurLine = 0;
	m_nCurArg = 1;

	updateState();
}

void KDevArgHint::cursorPositionChanged ( KEditor::Document* pDoc, int nLine, int nCol )
{
	if ( m_nCurLine == 0 )
		m_nCurLine = nLine;

	if ( m_nCurLine > 0 && m_nCurLine != nLine)
	{
		hide();
		return;
	}
	else if ( m_nCurLine == nLine )
		show();

	KEditor::EditDocumentIface* pEditIface = KEditor::EditDocumentIface::interface ( pDoc );
	if ( !pEditIface )
	{
		kdDebug ( 12001 ) << "Editor doesn't support the EditDocumentIface" << endl;
		return;
	}

	QString strCurLine = pEditIface->line ( nLine );
	//strCurLine = strCurLine.left ( nCol );
	QString strLineToCol = strCurLine.left ( nCol );

	// only for testing
	strCurLine = strLineToCol;

	int nBegin = strCurLine.findRev ( m_strArgWrapping[0] );

	if ( nBegin == -1 || nBegin != -1 && strCurLine.findRev ( m_strArgWrapping[1] ) != -1)
		hide();
	else if ( nBegin != -1 )
		show();

	int nNumDelimiter = 0;

	while ( nBegin != -1 )
	{
		nBegin = strCurLine.find ( m_strArgDelimiter, nBegin + 1 );

		if ( nBegin != -1 )
			nNumDelimiter++;
	}

	setCurArg ( nNumDelimiter + 1 );
}

void KDevArgHint::setFunctionText ( int nFunc, const QString& strText )
{
	m_funcList.replace ( nFunc, strText );

	m_nNumFunc++;

	if ( nFunc == m_nCurFunc )
		m_pFuncLabel->clear();

	updateState();
}

void KDevArgHint::setArgMarkInfos ( QString strWrapping, QString strDelimiter )
{
	if ( strWrapping.isEmpty() || strDelimiter.isEmpty() )
		return;

	m_strArgWrapping = strWrapping;
	m_strArgDelimiter = strDelimiter;
	m_bMarkingEnabled = true;
}

void KDevArgHint::nextArg()
{
	m_nCurArg++;

	updateState();
}

void KDevArgHint::prevArg()
{
	m_nCurArg--;

	updateState();
}

void KDevArgHint::setCurArg ( int nCurArg )
{
	if ( m_nCurArg == nCurArg )
		return;

	m_nCurArg = nCurArg;

	updateState();
}

QString KDevArgHint::markCurArg()
{
	QString strFuncText = m_funcList [ m_nCurFunc ];

	if ( !m_bMarkingEnabled )
		return strFuncText;

	if ( strFuncText.isEmpty() )
		return "\0";

	strFuncText = strFuncText.prepend ( "<qt>&nbsp;" );
	strFuncText = strFuncText.append ( "</qt>" );

	int nBegin = strFuncText.find ( m_strArgWrapping[0] ) + 1;
	int nEnd = nBegin;

	for ( int i = 0; i <= m_nCurArg; i++ )
	{
		if ( i > 1 )
			nBegin = nEnd + 1;

		if ( strFuncText.find ( m_strArgDelimiter , nBegin ) == -1 )
		{
			nEnd = strFuncText.find ( m_strArgWrapping[1], nBegin );

			break;
		}
		else
			nEnd = strFuncText.find ( m_strArgDelimiter, nBegin );
	}

	strFuncText = strFuncText.insert ( nBegin, "<b>" );
	strFuncText = strFuncText.insert ( ( nEnd + 3 ), "</b>" );

	while ( strFuncText.find ( ' ', 0 ) != -1 ) // replace ' ' with "&ndsp;" so that there's no wrap
		strFuncText = strFuncText.replace ( ( strFuncText.find ( ' ', 0 ) ), 1, "&nbsp;" );

	kdDebug ( 12001 ) << strFuncText <<endl;

	return strFuncText;
}

