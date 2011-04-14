/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright (C) 2007 Andreas Pakulat <apaku@gmx.de>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "makeoutputdelegate.h"

#include "makeoutputmodel.h"
#include <QtGui/QPainter>

#include <kdebug.h>

MakeOutputDelegate::MakeOutputDelegate( QObject* parent )
    : QItemDelegate(parent), errorBrush( KColorScheme::View, KColorScheme::NegativeText ),
      warningBrush( KColorScheme::View, KColorScheme::NeutralText ),
      informationBrush( KColorScheme::View, KColorScheme::NeutralText ),
      builtBrush( KColorScheme::View, KColorScheme::PositiveText )
{
}

void MakeOutputDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItem opt = option;
    QVariant status = index.data(MakeOutputModel::MakeItemTypeRole);
    if( status.isValid() )
    {
        MakeOutputModel::OutputItemType type = status.value<MakeOutputModel::OutputItemType>();
        if( type == MakeOutputModel::ErrorItem )
        {
            opt.palette.setBrush( QPalette::Text, errorBrush.brush( option.palette ) );
            opt.font.setBold( true );
        }else if( type == MakeOutputModel::WarningItem )
        {
            opt.palette.setBrush( QPalette::Text, warningBrush.brush( option.palette ) );
        }else if( type == MakeOutputModel::InformationItem )
        {
            opt.palette.setBrush( QPalette::Text, informationBrush.brush( option.palette ) );
        }else if( type == MakeOutputModel::ActionItem )
        {
            opt.palette.setBrush( QPalette::Text, builtBrush.brush( option.palette ) );
            opt.font.setBold( true );
        }
    }
    QItemDelegate::paint(painter, opt, index);
}

