//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: action.h,v 1.1.1.1.2.1 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __ACTION_H__
#define __ACTION_H__

#include <QAction>

namespace MusEGui {

//---------------------------------------------------------
//   Action
//---------------------------------------------------------

class Action : public QAction {
      Q_OBJECT
      int _id;

   public:
      Action(QObject* parent, int i, const char* name = 0, bool toggle = false)
         : QAction(name, parent) {
            _id = i;
            setCheckable(toggle);
            }
      void setId(int i) { _id = i; }
      int id() const    { return _id; }
      };

}

#endif

