//=============================================================================
//  AL
//  Audio Utility Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#ifndef __SIG_H__
#define __SIG_H__

#include <map>

#include "../muse/xml.h"

namespace MusECore {
class Xml;
class PendingOperationList;
struct PendingOperationItem;
}

namespace AL {

#ifndef MAX_TICK
#define MAX_TICK (0x7fffffff/100)
#endif

//---------------------------------------------------------
//   TimeSignature
//---------------------------------------------------------

struct TimeSignature {
      int z, n;
      TimeSignature() { z = 4; n = 4; }
      TimeSignature(int a, int b) { z = a; n = b; }
      bool isValid() const;
     };

//---------------------------------------------------------
//   Signature Event
//---------------------------------------------------------

struct SigEvent {
      TimeSignature sig;
      unsigned tick;    // signature valid from this position
      int bar;          // precomputed
      int read(MusECore::Xml&);
      void write(int, MusECore::Xml&, int) const;

      SigEvent() { }
      SigEvent(const TimeSignature& s, unsigned tk) {
            sig = s;
            tick = tk;
            bar = 0;
            }
      };

//---------------------------------------------------------
//   SigList
//---------------------------------------------------------

typedef std::map<unsigned, SigEvent*, std::less<unsigned> > SIGLIST;
typedef SIGLIST::iterator iSigEvent;
typedef SIGLIST::const_iterator ciSigEvent;
typedef SIGLIST::reverse_iterator riSigEvent;
typedef SIGLIST::const_reverse_iterator criSigEvent;

class SigList : public SIGLIST {
   friend struct MusECore::PendingOperationItem;
   
      int ticks_beat(int N) const;
      int ticksMeasure(const TimeSignature&) const;
      int ticksMeasure(int z, int n) const;
      void add(unsigned tick, SigEvent* e, bool do_normalize = true);
      void del(iSigEvent, bool do_normalize = true);

   public:
      SigList();
      ~SigList();
      void clear();
      void add(unsigned tick, const TimeSignature& s);
      void del(unsigned tick);
      void normalize();
      
      void read(MusECore::Xml&);
      void write(int, MusECore::Xml&) const;
      
      void dump() const;

      TimeSignature timesig(unsigned tick) const;
      void timesig(unsigned tick, int& z, int& n) const;
      void tickValues(unsigned t, int* bar, int* beat, unsigned* tick) const;
      unsigned bar2tick(int bar, int beat, unsigned tick) const;

      int ticksMeasure(unsigned tick) const;
      int ticksBeat(unsigned tick) const;
      unsigned raster(unsigned tick, int raster) const;
      unsigned raster1(unsigned tick, int raster) const;    // round down
      unsigned raster2(unsigned tick, int raster) const;    // round up
      int rasterStep(unsigned tick, int raster) const;
      
      void addOperation(unsigned tick, const TimeSignature& s, MusECore::PendingOperationList& ops); 
      void delOperation(unsigned tick, MusECore::PendingOperationList& ops);
      };

extern SigList sigmap;

}

#endif
