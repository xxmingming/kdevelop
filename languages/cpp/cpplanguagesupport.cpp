/*
* KDevelop C++ Language Support
*
* Copyright (c) 2005 Matt Rogers <mattr@kde.org>
* Copyright (c) 2006 Adam Treat <treat@kde.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Library General Public License as
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

#include <QMutex>
#include <QMutexLocker>

#include <kdebug.h>
#include <kinstance.h>
#include <kstandarddirs.h>
#include <kgenericfactory.h>

#include <kdevcore.h>
#include <kdevproject.h>
#include <kdevfilemanager.h>
#include <kdevprojectmodel.h>
#include <kdevprojectcontroller.h>
#include <kdevdocumentcontroller.h>
#include <kdevbackgroundparser.h>

#include "cpplanguagesupport.h"
#include "cpphighlighting.h"

#include "parser/codemodel.h"
#include "parser/ast.h"
#include "parser/parsesession.h"

#include "duchain/duchain.h"

#include "parsejob.h"
#include "codeproxy.h"
#include "codedelegate.h"

#include <kdebug.h>

typedef KGenericFactory<CppLanguageSupport> KDevCppSupportFactory;
K_EXPORT_COMPONENT_FACTORY( kdevcpplanguagesupport, KDevCppSupportFactory( "kdevcppsupport" ) )

CppLanguageSupport::CppLanguageSupport( QObject* parent,
                                        const QStringList& /*args*/ )
        : KDevLanguageSupport( KDevCppSupportFactory::instance(), parent )
{
    QString types =
        QLatin1String( "text/x-chdr,text/x-c++hdr,text/x-csrc,text/x-c++src" );
    m_mimetypes = types.split( "," );

    m_codeProxy = new CodeProxy( this );
    m_codeDelegate = new CodeDelegate( this );
    m_highlights = new CppHighlighting( this );

    connect( KDevCore::documentController(),
             SIGNAL( documentLoaded( KDevDocument* ) ),
             this, SLOT( documentLoaded( KDevDocument* ) ) );
    connect( KDevCore::documentController(),
             SIGNAL( documentClosed( KDevDocument* ) ),
             this, SLOT( documentClosed( KDevDocument* ) ) );
    connect( KDevCore::documentController(),
             SIGNAL( documentActivated( KDevDocument* ) ),
             this, SLOT( documentActivated( KDevDocument* ) ) );
    connect( KDevCore::projectController(),
             SIGNAL( projectOpened() ),
             this, SLOT( projectOpened() ) );
    connect( KDevCore::projectController(),
             SIGNAL( projectClosing() ),
             this, SLOT( projectClosing() ) );
}

CppLanguageSupport::~CppLanguageSupport()
{
}

KDevCodeModel *CppLanguageSupport::codeModel( const KUrl &url ) const
{
    if ( url.isValid() )
        return m_codeProxy->codeModel( url );
    else
        return m_codeProxy->codeModel( KDevCore::documentController() ->activeDocumentUrl() );
}

KDevCodeProxy *CppLanguageSupport::codeProxy() const
{
    return m_codeProxy;
}

KDevCodeDelegate *CppLanguageSupport::codeDelegate() const
{
    return m_codeDelegate;
}

KDevCodeRepository *CppLanguageSupport::codeRepository() const
{
    return 0;
}

KDevParseJob *CppLanguageSupport::createParseJob( const KUrl &url )
{
    return new CPPParseJob( url, this );
}

KDevParseJob *CppLanguageSupport::createParseJob( KDevDocument *document )
{
    return new CPPParseJob( document, this );
}

QStringList CppLanguageSupport::mimeTypes() const
{
    return m_mimetypes;
}

void CppLanguageSupport::documentLoaded( KDevDocument *document )
{
    if ( supportsDocument( document ) )
        KDevCore::backgroundParser() ->addDocument( document );
}

void CppLanguageSupport::documentClosed( KDevDocument *document )
{
    if ( supportsDocument( document ) )
        KDevCore::backgroundParser() ->removeDocument( document );
}

void CppLanguageSupport::documentActivated( KDevDocument *document )
{
    Q_UNUSED( document );
}

KDevCodeHighlighting *CppLanguageSupport::codeHighlighting() const
{
    return m_highlights;
}

void CppLanguageSupport::projectOpened()
{
    // FIXME Add signals slots from the filemanager for:
    //       1. filesAddedToProject
    //       2. filesRemovedFromProject
    //       3. filesChangedInProject

    // FIXME this should be moved to the project itself
    KUrl::List documentList;
    QList<KDevProjectFileItem*> files = KDevCore::activeProject()->allFiles();
    foreach ( KDevProjectFileItem * file, files )
    {
        if ( supportsDocument( file->url() ) )
        {
            documentList.append( file->url() );
        }
    }
    KDevCore::backgroundParser() ->addDocumentList( documentList );
}

void CppLanguageSupport::projectClosing()
{
    KDevCore::backgroundParser()->clear(this);

    KUrl::List documentList;
    QList<KDevProjectFileItem*> files = KDevCore::activeProject()->allFiles();
    foreach ( KDevProjectFileItem * file, files )
    {
        if ( supportsDocument( file->url() ) )
        {
            KDevCore::backgroundParser() ->removeDocument( file->url() );
        }
    }

    lockAllParseMutexes();

    // Now we can do destructive stuff

    DUChain::self()->clear();

    unlockAllParseMutexes();
}

void CppLanguageSupport::releaseAST( KDevAST *ast)
{
    TranslationUnitAST* t = static_cast<TranslationUnitAST*>(ast);
    delete t->session;
    // The ast is in the memory pool which has been deleted as part of the session.
}

#include "cpplanguagesupport.moc"

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
