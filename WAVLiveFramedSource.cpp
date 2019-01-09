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
// Implementation

#include "WAVLiveFramedSource.hh"

extern "C" int ipcam_get_astream(void *ctxt, uint8_t *buf, int size, int *ndata, uint64_t *pts, int *duration);

////////// ADTSLiveFramedSource //////////

WAVLiveFramedSource* WAVLiveFramedSource::createNew(UsageEnvironment& env, void* ctxt) {
  return new WAVLiveFramedSource(env, ctxt);
}

WAVLiveFramedSource::WAVLiveFramedSource(UsageEnvironment& env, void* ctxt)
  : FramedSource(env), mContext(ctxt), mMaxFrameSize(0) {
  fAudioFormat = WA_UNKNOWN;
  fBitsPerSample = 16;
  fNumChannels = 1;
  fSamplingFrequency = 8000;
}

WAVLiveFramedSource::~WAVLiveFramedSource() {
}

void WAVLiveFramedSource::doGetNextFrame() {
    uint64_t pts  ;
    int      ndata;
    int      ncopy;
    int      duration;

    ncopy = ipcam_get_astream(mContext, fTo, fMaxSize, &ndata, &pts, &duration);
    if (ncopy > 0) {
        fFrameSize = ncopy;
        fNumTruncatedBytes = ndata - ncopy;
        fPresentationTime.tv_sec  = pts / 1000000;
        fPresentationTime.tv_usec = pts % 1000000;
        fDurationInMicroseconds   = duration;
        if (mMaxFrameSize < fFrameSize) mMaxFrameSize = fFrameSize;
    }

    // To avoid possible infinite recursion, we need to return to the event loop to do this:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc*)FramedSource::afterGetting, this);
}
