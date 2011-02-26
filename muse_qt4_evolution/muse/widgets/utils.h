//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __UTILS_H__
#define __UTILS_H__

#include "gui.h"

class SimpleButton;

extern QString bitmap2String(int bm);
extern int string2bitmap(const QString& str);

extern int num2cols(int min, int max);
extern QFrame* hLine(QWidget* parent);
extern QFrame* vLine(QWidget* parent);
extern void dump(const unsigned char* p, int n);
extern double curTime();
extern SimpleButton* newMuteButton();
extern SimpleButton* newSoloButton();
extern SimpleButton* newMonitorButton();
extern SimpleButton* newDrumMapButton();
extern SimpleButton* newOffButton();
extern SimpleButton* newRecordButton();
extern SimpleButton* newAutoWriteButton();
extern SimpleButton* newAutoReadButton();
extern SimpleButton* newSyncButton();
extern SimpleButton* newPlusButton();
extern SimpleButton* newMinusButton();
extern SimpleButton* newStereoButton();
QToolButton* newOutRouteButton();
QToolButton* newInRouteButton();

extern void fatalError(const char*);

extern QColor lineColor[splitWidth];
extern void paintHLine(QPainter& p, int x1, int x2, int y);
extern void paintVLine(QPainter& p, int y1, int y2, int x);

#endif
