/***************************************************************************
 *   Copyright (C) 2003 by Roberto Raggi                                   *
 *   roberto@kdevelop.org                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __store_walker_h
#define __store_walker_h

#include "tree_parser.h"
#include <codemodel.h>
#include <qstringlist.h>
#include <qvaluestack.h>

class StoreWalker: public TreeParser
{
public:
    StoreWalker( const QString& fileName, CodeModel* store );
    virtual ~StoreWalker();

    // translation-unit
    virtual void parseTranslationUnit( TranslationUnitAST* );

    // declarations
    virtual void parseDeclaration( DeclarationAST* );
    virtual void parseLinkageSpecification( LinkageSpecificationAST* );
    virtual void parseNamespace( NamespaceAST* );
    virtual void parseNamespaceAlias( NamespaceAliasAST* );
    virtual void parseUsing( UsingAST* );
    virtual void parseUsingDirective( UsingDirectiveAST* );
    virtual void parseTypedef( TypedefAST* );
    virtual void parseTemplateDeclaration( TemplateDeclarationAST* );
    virtual void parseSimpleDeclaration( SimpleDeclarationAST* );
    virtual void parseFunctionDefinition( FunctionDefinitionAST* );
    virtual void parseLinkageBody( LinkageBodyAST* );
    virtual void parseAccessDeclaration( AccessDeclarationAST* );

    // type-specifier
    virtual void parseTypeSpecifier( TypeSpecifierAST* );
    virtual void parseClassSpecifier( ClassSpecifierAST* );
    virtual void parseEnumSpecifier( EnumSpecifierAST* );
    virtual void parseElaboratedTypeSpecifier( ElaboratedTypeSpecifierAST* );

    virtual void parseTypeDeclaratation( TypeSpecifierAST* typeSpec );
    virtual void parseDeclaration( GroupAST* funSpec, GroupAST* storageSpec, TypeSpecifierAST* typeSpec, InitDeclaratorAST* decl );
    virtual void parseFunctionDeclaration( GroupAST* funSpec, GroupAST* storageSpec, TypeSpecifierAST* typeSpec, InitDeclaratorAST* decl );
    virtual void parseFunctionArguments( DeclaratorAST* declarator, FunctionDom method );
    virtual void parseBaseClause( BaseClauseAST* baseClause, ClassDom klass );

private:
    NamespaceDom findOrInsertNamespace( NamespaceAST* ast, const QString& name );

    QString typeOfDeclaration( TypeSpecifierAST* typeSpec, DeclaratorAST* declarator );
    QStringList scopeOfName( NameAST* id, const QStringList& scope );
    QStringList scopeOfDeclarator( DeclaratorAST* d, const QStringList& scope );

private:
    FileDom m_file;
    QString m_fileName;
    QStringList m_currentScope;
    CodeModel* m_store;
    QValueList<QStringList> m_imports;
    int m_currentAccess;
    bool m_inSlots;
    bool m_inSignals;
    int m_anon;

    QValueStack<NamespaceDom> m_currentNamespace;
    QValueStack<ClassDom> m_currentClass;

private:
    StoreWalker( const StoreWalker& source );
    void operator = ( const StoreWalker& source );
};

#endif // __store_walker_h
