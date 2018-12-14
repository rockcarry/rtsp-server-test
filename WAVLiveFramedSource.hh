/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2018 Live Networks, Inc.  All rights reserved.
// A WAV audio live source
// C++ header

#ifndef _WAV_FRAMED_LIVE_SOURCE_HH
#define _WAV_FRAMED_LIVE_SOURCE_HH

#ifndef _AUDIO_INPUT_DEVICE_HH
#include "AudioInputDevice.hh"
#endif

typedef enum {
  WA_PCM = 0x01,
  WA_PCMA = 0x06,
  WA_PCMU = 0x07,
  WA_IMA_ADPCM = 0x11,
  WA_UNKNOWN
} WAV_AUDIO_FORMAT;

class WAVLiveFramedSource: public AudioInputDevice {
public:
  static WAVLiveFramedSource* createNew(UsageEnvironment& env, void* ctxt);

protected:
  WAVLiveFramedSource(UsageEnvironment& env, void* ctxt);
  virtual ~WAVLiveFramedSource();

protected:
  void* mContext;

private:
  virtual void doGetNextFrame();
  virtual Boolean setInputPort(int portIndex);
  virtual double getAverageLevel() const;
};

#endif