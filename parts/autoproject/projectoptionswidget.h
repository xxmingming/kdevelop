/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _PROJECTOPTIONSWIDGET_H_
#define _PROJECTOPTIONSWIDGET_H_

#include <qtabwidget.h>

#include "kdevcompileroptions.h"

class QCheckBox;
class QLineEdit;
class QSpinBox;
class ServiceComboBox;
class AutoProjectPart;


class ProjectOptionsWidget : public QTabWidget
{
    Q_OBJECT
    
public:
    ProjectOptionsWidget( AutoProjectPart *part, QWidget *parent=0, const char *name=0 );
    ~ProjectOptionsWidget();

public slots:
    void accept();
    
private slots:
    void cflagsClicked();
    void cxxflagsClicked();
    void f77flagsClicked();

private:
    QWidget *createCompilerTab();
    QWidget *createConfigureTab();
    QWidget *createMakeTab();
    QWidget *createMiscTab();
    KDevCompilerOptions *createCompilerOptions(const QString &lang);

    void init();
    
    ServiceComboBox *cservice_combo;
    ServiceComboBox *cxxservice_combo;
    ServiceComboBox *f77service_combo;
    QLineEdit *cbinary_edit;
    QLineEdit *cxxbinary_edit;
    QLineEdit *f77binary_edit;
    QLineEdit *cflags_edit;
    QLineEdit *cxxflags_edit;
    QLineEdit *f77flags_edit;

    QCheckBox *abort_box;
    QCheckBox *dontact_box;
    QSpinBox *jobs_box;
    QLineEdit *makebin_edit;

    QLineEdit *configargs_edit;
    QLineEdit *mainbin_edit;
    QLineEdit *progargs_edit;
    
    AutoProjectPart *m_part;
};

#endif
