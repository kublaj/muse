//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ticksynth.h,v 1.1.2.3 2009/12/06 10:05:00 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#ifndef __TICKSYNTH_H__
#define __TICKSYNTH_H__

#include "synth.h"

namespace MusECore {
class PendingOperationList;

extern void initMetronome();
extern void exitMetronome();
class MetronomeSynthI : public SynthI
{
  virtual bool hasAuxSend() const  { return false; }
   
public:
  void initSamplesOperation(PendingOperationList& operations);
  
  //------------------------------------------------------------------------
  // The metronome synth is special - it cannot be routed like other tracks, 
  //  and thus cannot be muted, soloed, recorded, or monitored.
  //------------------------------------------------------------------------
  virtual void setMute(bool)         { }
  virtual void setOff(bool)          { }
  virtual void setSolo(bool)         { }
  virtual bool isMute() const        { return false; }
  virtual unsigned int internalSolo() const { return 0; }
  virtual bool soloMode() const      { return false; }
  virtual bool solo() const          { return false; }
  virtual bool mute() const          { return false; }
  virtual bool off() const           { return false; }
  virtual bool recordFlag() const    { return false; }
  virtual void setRecMonitor(bool)   { }
  virtual bool recMonitor() const    { return false; }
};
extern MetronomeSynthI* metronome;

} // namespace MusECore

#endif

