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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a H264/H265 video live.
// Implementation

#include "H26XVideoLiveServerMediaSubsession.hh"
#include "H26XLiveFramedSource.hh"
#include "H264VideoRTPSink.hh"
#include "H264VideoStreamFramer.hh"
#include "H265VideoRTPSink.hh"
#include "H265VideoStreamFramer.hh"

extern "C" void ipcam_request_idr  (void *ctxt);
extern "C" int  ipcam_video_bitrate(void *ctxt);
extern "C" int  ipcam_video_ish265 (void *ctxt) { return 1; }

H26XVideoLiveServerMediaSubsession*
H26XVideoLiveServerMediaSubsession::createNew(UsageEnvironment& env, void *ctxt, Boolean reuseFirstSource) {
  return new H26XVideoLiveServerMediaSubsession(env, ctxt, reuseFirstSource);
}

H26XVideoLiveServerMediaSubsession::H26XVideoLiveServerMediaSubsession(UsageEnvironment& env, void *ctxt, Boolean reuseFirstSource)
  : OnDemandServerMediaSubsession(env, reuseFirstSource),
    fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), mContext(ctxt) {
}

H26XVideoLiveServerMediaSubsession::~H26XVideoLiveServerMediaSubsession() {
  delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  H26XVideoLiveServerMediaSubsession* subsess = (H26XVideoLiveServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void H26XVideoLiveServerMediaSubsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  H26XVideoLiveServerMediaSubsession* subsess = (H26XVideoLiveServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void H26XVideoLiveServerMediaSubsession::checkForAuxSDPLine1() {
  nextTask() = NULL;

  char const* dasl;
  if (fAuxSDPLine != NULL) {
    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;

    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (!fDoneFlag) {
    // try again after a brief delay:
    int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
                  (TaskFunc*)checkForAuxSDPLine, this);
  }
}

char const* H26XVideoLiveServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

  if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
    // Note: For H264/H265 video files, the 'config' information (used for several payload-format
    // specific parameters in the SDP description) isn't known until we start reading the file.
    // This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
    // and we need to start reading data from our file until this changes.
    fDummyRTPSink = rtpSink;

    // Start reading the file:
    fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

    // Check whether the sink's 'auxSDPLine()' is ready:
    checkForAuxSDPLine(this);
  }

  envir().taskScheduler().doEventLoop(&fDoneFlag);

  return fAuxSDPLine;
}

FramedSource* H26XVideoLiveServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = ipcam_video_bitrate(mContext); // kbps, estimate

  // Create the video source:
  H26XLiveFramedSource* source = H26XLiveFramedSource::createNew(envir(), mContext);
  if (source == NULL) return NULL;

  // Create a framer for the Video Elementary Stream:
  if (ipcam_video_ish265(mContext)) {
    return H265VideoStreamFramer::createNew(envir(), source);
  } else {
    return H264VideoStreamFramer::createNew(envir(), source);
  }
}

RTPSink* H26XVideoLiveServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* /*inputSource*/) {
  if (ipcam_video_ish265(mContext)) {
    return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
  } else {
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
  }
}

void H26XVideoLiveServerMediaSubsession::getStreamParameters(
            unsigned clientSessionId,
            netAddressBits clientAddress,
            Port const& clientRTPPort,
            Port const& clientRTCPPort,
            int tcpSocketNum,
            unsigned char rtpChannelId,
            unsigned char rtcpChannelId,
            netAddressBits& destinationAddress,
            u_int8_t& destinationTTL,
            Boolean& isMulticast,
            Port& serverRTPPort,
            Port& serverRTCPPort,
            void*& streamToken) {
  OnDemandServerMediaSubsession::getStreamParameters(clientSessionId, clientAddress, clientRTPPort, clientRTCPPort,
        tcpSocketNum, rtpChannelId, rtcpChannelId, destinationAddress, destinationTTL, isMulticast, serverRTPPort, serverRTCPPort, streamToken);
  ipcam_request_idr(mContext);
}
