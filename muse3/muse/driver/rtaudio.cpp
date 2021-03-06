//=========================================================
//  MusE
//  Linux Music Editor
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

#include <QMessageBox>
#include <RtAudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.h"
#include "audio.h"
#include "audiodev.h"
#include "globals.h"
#include "song.h"
#include "driver/alsatimer.h"
#include "pos.h"
#include "gconfig.h"
#include "utils.h"

#define DEBUG_RTAUDIO 0

#define MASTER_LEFT (void*)1
#define MASTER_RIGHT (void*)2

namespace MusECore {

//---------------------------------------------------------
//   RtAudioDevice
//---------------------------------------------------------

struct MuseRtAudioPort {
  QString name;
  float* buffer;
};

class RtAudioDevice : public AudioDevice {

      RtAudio *dac;

   public:
      Audio::State state;
      int _framePos;
      unsigned _framesAtCycleStart;
      double _timeAtCycleStart;
      int playPos;
      bool seekflag;

      QList<MuseRtAudioPort*> outputPortsList;
      QList<MuseRtAudioPort*> inputPortsList;

      RtAudioDevice(bool forceDefault);
      virtual ~RtAudioDevice()
      {

        while (outputPortsList.size() > 0) {
          MuseRtAudioPort *port = outputPortsList.takeFirst();
          free (port->buffer);
          free (port);
        }

        while (inputPortsList.size() > 0) {
          MuseRtAudioPort *port = inputPortsList.takeFirst();
          free (port->buffer);
          free (port);
        }

      }

      virtual inline int deviceType() const { return RTAUDIO_AUDIO; }
      
      virtual bool start(int);
      
      virtual void stop ();
      virtual int framePos() const { 
            return _framePos; 
            }

      // These are meant to be called from inside process thread only.      
      virtual unsigned framesAtCycleStart() const { return _framesAtCycleStart; }
      virtual unsigned framesSinceCycleStart() const 
      { 
        unsigned f =  lrint((curTime() - _timeAtCycleStart) * MusEGlobal::sampleRate);
        // Safety due to inaccuracies. It cannot be after the segment, right?
        if(f >= MusEGlobal::segmentSize)
          f = MusEGlobal::segmentSize - 1;
        return f;
      }

      virtual float* getBuffer(void* port, unsigned long nframes)
      {
        if (nframes > MusEGlobal::segmentSize) {

          fprintf(stderr, "RtAudioDevice::getBuffer nframes > segment size\n");

          exit(-1);
        }

        return ((MuseRtAudioPort*)port)->buffer;
      }

      virtual std::list<QString> outputPorts(bool, int) {
        std::list<QString> outlist;

        foreach (MuseRtAudioPort *port, outputPortsList) {

          outlist.push_back(port->name);
        }

        return outlist;
      }

      virtual std::list<QString> inputPorts(bool, int) {
        std::list<QString> inlist;

        foreach (MuseRtAudioPort *port, inputPortsList) {

          inlist.push_back(port->name);
        }

        return inlist;
      }

      virtual void registerClient() {}

      virtual const char* clientName() { return "RtAudio"; }
      
      virtual void* registerOutPort(const char* name, bool) {

        fprintf(stderr, "register output port [%s] length %d char %c\n", name, int(strlen(name)), name[strlen(name)-1] );

        foreach (MuseRtAudioPort *port, outputPortsList) {
          if (port->name == name) {
            fprintf(stderr, "RtAudioDevice::registerOutPort - port [%s] already exists, return existing.", name);
            return port;
          }
        }

        MuseRtAudioPort *port = new MuseRtAudioPort();
        port->name = name;
        port->buffer = new float[MusEGlobal::segmentSize];
        memset(port->buffer, 0, MusEGlobal::segmentSize * sizeof(float));

        outputPortsList.push_back(port);
        return port;
      }

      virtual void* registerInPort(const char* name, bool) {

        fprintf(stderr, "register input port [%s] length %d char %c\n", name, int(strlen(name)), name[strlen(name)-1] );

        foreach (MuseRtAudioPort *port, inputPortsList) {
          if (port->name == name) {
            fprintf(stderr, "RtAudioDevice::registerInPort - port [%s] already exists, return existing.", name);
            return port;
          }
        }

        MuseRtAudioPort *port = new MuseRtAudioPort();
        port->name = name;
        port->buffer = new float[MusEGlobal::segmentSize];
        memset(port->buffer, 0, MusEGlobal::segmentSize * sizeof(float));

        inputPortsList.push_back(port);
        return port;
      }

      float getDSP_Load() {
        return 0.0f;
      }

      virtual AudioDevice::PortType portType(void*) const { return AudioPort; }
      virtual AudioDevice::PortDirection portDirection(void*) const { return OutputPort; }
      virtual void unregisterPort(void*) {}
      virtual bool connect(void* /*src*/, void* /*dst*/)  {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::connect\n");
        return false;
      }
      virtual bool connect(const char* /*src*/, const char* /*dst*/)  {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::connect2\n");
        return false;
      }
      virtual bool disconnect(void* /*src*/, void* /*dst*/)  { return false; }
      virtual bool disconnect(const char* /*src*/, const char* /*dst*/)  { return false; }
      virtual int connections(void* /* clientPort */) {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::connections\n");
        return 1; // always return nonzero, for now
      }
      virtual bool portConnectedTo(void*, const char*) {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::portConnectedTo\n");
        return false;
      }
      virtual bool portsCanDisconnect(void* /*src*/, void* /*dst*/) const {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::portCanDisconnect\n");
        return false;
      }
      virtual bool portsCanDisconnect(const char* /*src*/, const char* /*dst*/) const {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::portCanDisconnect2\n");
        return false;
      }
      virtual bool portsCanConnect(void* /*src*/, void* /*dst*/) const {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::portCanConnect\n");
        return false;
      }
      virtual bool portsCanConnect(const char* /*src*/, const char* /*dst*/) const {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::portCanConnect\n");
        return false;
      }
      virtual bool portsCompatible(void* /*src*/, void* /*dst*/) const {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::portCompatible\n");
        return false;
      }
      virtual bool portsCompatible(const char* /*src*/, const char* /*dst*/) const {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::portCompatible2\n");
        return false;
      }
      virtual void setPortName(void*, const char*) {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::setPortName\n");

      }
      virtual void* findPort(const char*) {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::findPort\n");
        return 0;
      }
      // preferred_name_or_alias: -1: No preference 0: Prefer canonical name 1: Prefer 1st alias 2: Prefer 2nd alias.
      virtual char*  portName(void*, char* str, int str_size, int /*preferred_name_or_alias*/ = -1) {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::portName %s\n", str);
        if(str_size == 0) {
          return 0;
        }
        str[0] = '\0';
        return str;
      }
      virtual const char* canonicalPortName(void*) {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::canonicalPortName\n");
        return 0;
      }
      virtual unsigned int portLatency(void* /*port*/, bool /*capture*/) const {
        if(DEBUG_RTAUDIO)
          fprintf(stderr, "RtAudio::portLatency\n");
        return 0;
      }

      virtual int getState() {
        return state;
      }

      virtual unsigned getCurFrame() const { 
        if(DEBUG_RTAUDIO)
            fprintf(stderr, "RtAudioDevice::getCurFrame %d\n", _framePos);
      
        return _framePos;
      }

      virtual unsigned frameTime() const {
        return lrint(curTime() * MusEGlobal::sampleRate);
      }

      virtual double systemTime() const {
        struct timeval t;
        gettimeofday(&t, 0);
        //fprintf(stderr, "%ld %ld\n", t.tv_sec, t.tv_usec);  // Note I observed values coming out of order! Causing some problems.
        return (double)((double)t.tv_sec + (t.tv_usec / 1000000.0));
      }

      virtual bool isRealtime() { return MusEGlobal::realTimeScheduling; }
      virtual int realtimePriority() const { return 40; }
      virtual void startTransport() {
            if(DEBUG_RTAUDIO)
                fprintf(stderr, "RtAudioDevice::startTransport playPos=%d\n", playPos);
            state = Audio::PLAY;
            }
      virtual void stopTransport() {
            if(DEBUG_RTAUDIO)
                fprintf(stderr, "RtAudioDevice::stopTransport, playPos=%d\n", playPos);
            state = Audio::STOP;
            }
      virtual int setMaster(bool) { return 1; }

      virtual void seekTransport(const Pos &p)
      {
            if(DEBUG_RTAUDIO)
                fprintf(stderr, "RtAudioDevice::seekTransport frame=%d topos=%d\n",playPos, p.frame());
            seekflag = true;
            //pos = n;
            playPos = p.frame();
            
      }
      virtual void seekTransport(unsigned pos) {
            if(DEBUG_RTAUDIO)
                fprintf(stderr, "RtAudioDevice::seekTransport frame=%d topos=%d\n",playPos,pos);
            seekflag = true;
            //pos = n;
            playPos = pos;
            }
      virtual void setFreewheel(bool) {}
};

RtAudioDevice* rtAudioDevice = 0;

RtAudioDevice::RtAudioDevice(bool forceDefault) : AudioDevice()
      {
      fprintf(stderr, "Init RtAudioDevice\n");
      MusEGlobal::sampleRate = MusEGlobal::config.deviceAudioSampleRate;
      MusEGlobal::segmentSize = MusEGlobal::config.deviceAudioBufSize;

      seekflag = false;
      state = Audio::STOP;
      //startTime = curTime();
      _framePos = 0;
      _framesAtCycleStart = 0;
      _timeAtCycleStart = 0.0;
      playPos = 0;

      RtAudio::Api api = RtAudio::UNSPECIFIED;

      switch (MusEGlobal::config.deviceAudioBackend) {
              case MusEGlobal::RtAudioChoice:
                api = RtAudio::UNSPECIFIED;
                break;
              case MusEGlobal::RtAudioAlsa:
                api = RtAudio::LINUX_ALSA;
              break;
              case MusEGlobal::RtAudioPulse:
                api = RtAudio::LINUX_PULSE;
              break;
              case MusEGlobal::RtAudioOss:
                api = RtAudio::LINUX_OSS;
              break;
              //case MusEGlobal::RtAudioJack:
              //  api = RtAudio::UNIX_JACK;
              //break;
          default:
            fprintf(stderr, "Error: RtAudio device selection illegal, setting up dummy audio backend!\n");
            api = RtAudio::RTAUDIO_DUMMY;
      }

      if (forceDefault) {

          api = RtAudio::LINUX_PULSE;
      }

      dac = new RtAudio(api);
      if ( dac->getDeviceCount() < 1 ) {

        fprintf (stderr, "\nNo audio devices found!\n");
        QMessageBox::warning(NULL,"No sound device.","RtAudio did not find any audio device - run muse in midi-only mode if there is audio capable device.", QMessageBox::Ok);
      }
}


//---------------------------------------------------------
//   exitRtAudio
//---------------------------------------------------------

void exitRtAudio()
{
  if(rtAudioDevice)
    delete rtAudioDevice;
  rtAudioDevice = NULL;
  MusEGlobal::audioDevice = NULL;
}


//---------------------------------------------------------
//   initRtAudio
//---------------------------------------------------------

bool initRtAudio(bool forceDefault = false)
{
  rtAudioDevice = new RtAudioDevice(forceDefault);
  MusEGlobal::audioDevice = rtAudioDevice;
  return false;
}

//---------------------------------------------------------
//   processAudio
//---------------------------------------------------------
int processAudio( void * outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double /* streamTime */, RtAudioStreamStatus /* status */, void * /* userData */ )
{
//  fprintf(stderr, "RtAduioDevice::processAudio %d\n", nBufferFrames);

  float *floatOutputBuffer = (float*)outputBuffer;
  float *floatInputBuffer = (float*)inputBuffer;

  rtAudioDevice->_framePos += nBufferFrames;
  rtAudioDevice->_framesAtCycleStart += nBufferFrames;

  if(rtAudioDevice->seekflag)
  {
    MusEGlobal::audio->sync(Audio::STOP, rtAudioDevice->playPos);

    rtAudioDevice->seekflag = false;
  }

  if(rtAudioDevice->state == Audio::PLAY) {

    rtAudioDevice->playPos += nBufferFrames;
  }

  if (MusEGlobal::audio->isRunning()) {

    MusEGlobal::audio->process((unsigned long)nBufferFrames);
  }

  if (rtAudioDevice->outputPortsList.size() >= 2) {

    MuseRtAudioPort *left = rtAudioDevice->outputPortsList.at(0);
    MuseRtAudioPort *right= rtAudioDevice->outputPortsList.at(1);

    // copy buffers into output
    for (unsigned int i = 0; i < nBufferFrames; i++ ) {

      floatOutputBuffer[i*2] = left->buffer[i];
      floatOutputBuffer[i*2+1] = right->buffer[i];
    }
  } else {

    //fprintf(stderr, "Too few ports in list, won't copy any data\n");
  }

  if (rtAudioDevice->inputPortsList.size() >= 1) {

    MuseRtAudioPort *left = rtAudioDevice->inputPortsList.at(0);
    MuseRtAudioPort *right = NULL;
    if (rtAudioDevice->inputPortsList.size() >= 2) {
       right= rtAudioDevice->inputPortsList.at(1);
    }

//    if (left->buffer[0] > 0.001) {
//      fprintf(stderr, "Got non zero buffer value %f\n", left->buffer[0]);
//    }

    // copy buffers into input
    for (unsigned int i = 0; i < nBufferFrames; i++ ) {

      left->buffer[i] = floatInputBuffer[i*2];

      if (right != NULL) {
        right->buffer[i] = floatInputBuffer[i*2+1];
      }
    }

  } else {

    //fprintf(stderr, "Too few ports in list, won't copy any data\n");
  }


  return 0;
}

//---------------------------------------------------------
//   start
//---------------------------------------------------------
bool RtAudioDevice::start(int /* priority */)
{
  if (dac->isStreamRunning()) {
    stop();
  }

  RtAudio::StreamParameters outParameters;
  outParameters.deviceId = dac->getDefaultOutputDevice();
  outParameters.nChannels = 2;
  outParameters.firstChannel = 0;

  RtAudio::StreamParameters inParameters;
  inParameters.deviceId = dac->getDefaultInputDevice();
  inParameters.nChannels = 2;
  inParameters.firstChannel = 0;

  unsigned int fin_sr = MusEGlobal::sampleRate;
  
  RtAudio::DeviceInfo in_di  = dac->getDeviceInfo(inParameters.deviceId);
  RtAudio::DeviceInfo out_di = dac->getDeviceInfo(outParameters.deviceId);
  
  if(!in_di.probed || !out_di.probed)
  {
    fprintf(stderr, "Error: RtAudioDevice: Could not probe device info.\n");
  }
  else
  {
    typedef std::vector<unsigned int>::iterator t_isr;

    std::set<unsigned int> sr_set;
    typedef std::set<unsigned int>::iterator t_isr_set;

    if(in_di.sampleRates.empty())
    {
      for(t_isr isr_o = out_di.sampleRates.begin(); isr_o != out_di.sampleRates.end(); ++isr_o)
        sr_set.insert(*isr_o);
    }
    else
    {
      if(out_di.sampleRates.empty())
      {
        for(t_isr isr_i = in_di.sampleRates.begin(); isr_i != in_di.sampleRates.end(); ++isr_i)
          sr_set.insert(*isr_i);
      }
      else
      {
        std::vector<unsigned int> out_sr_tmp = out_di.sampleRates;
        for(t_isr isr_i = in_di.sampleRates.begin(); isr_i != in_di.sampleRates.end(); ++isr_i)
        {
          for(t_isr isr_o = out_sr_tmp.begin(); isr_o != out_sr_tmp.end(); ++isr_o)
          {
            // Since we currently just use one openStream for both input and output,
            //  and openStream takes only one samplerate value, then we can only openStream
            //  if the samplerate is supported for both input and output... I guess?
            if(*isr_o == *isr_i)
            {
              sr_set.insert(*isr_i);
              // Done with this output samplerate. Remove it to speed up as we go.
              out_sr_tmp.erase(isr_o);
              break;
            }
          }
        }
      }
    }
    
    if(sr_set.find(fin_sr) == sr_set.end())
    {
      unsigned int near_low = 0;
      unsigned int near_high = 0;
      unsigned int sr;
      for(t_isr_set isr = sr_set.begin(); isr != sr_set.end(); ++isr)
      {
        sr = *isr;
        // Include the desired samplerate.
        if(sr > fin_sr)
          continue;
        if(sr > near_low)
          near_low = sr;
      }
      for(t_isr_set isr = sr_set.begin(); isr != sr_set.end(); ++isr)
      {
        sr = *isr;
        // Include the desired samplerate.
        if(sr < fin_sr)
          continue;
        if(near_high == 0 || sr < near_high)
          near_high = sr;
      }
      
      // Prefer the closest lower rate rather than highest to be safe, I suppose.
      if(near_low == 0 && near_high == 0)
      {
        fprintf(stderr, "Error: RtAudioDevice: Unsupported samplerate for both in/out:%d. No other samplerates found! Trying 44100 anyway...\n",
                MusEGlobal::sampleRate);
        fin_sr = 44100;
      }
      else
      {
        if(near_low == 0)
          fin_sr = near_high;
        else
          fin_sr = near_low;
        fprintf(stderr, "Warning: RtAudioDevice: Unsupported samplerate for both in/out:%d. Using closest:%d\n", MusEGlobal::sampleRate, fin_sr);
      }
    }
  }

  MusEGlobal::sampleRate = fin_sr;
  
  double data[2];

  try {

    dac->openStream( &outParameters, &inParameters, RTAUDIO_FLOAT32, MusEGlobal::sampleRate, &MusEGlobal::segmentSize, &processAudio, (void *)&data );
    dac->startStream();

  } catch ( RtAudioError& e ) {

    e.printMessage();
    fprintf(stderr, "Error: RtAudioDevice: Cannot open device for streaming!.\n");
    return false;
  }

  return true;
}

void RtAudioDevice::stop ()
{
  try {

    if (dac->isStreamRunning()) {
      dac->stopStream();
    }

  } catch (RtAudioError& e) {

    e.printMessage();
  }
  if ( dac->isStreamOpen() ) {

    dac->closeStream();
  }
}


} // namespace MusECore
