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
// on demand, from a H265 video live.
// Implementation

#include "H265VideoLiveServerMediaSubsession.hh"
#include "H265VideoRTPSink.hh"
#include "H265LiveFramedSource.hh"
#include "H265VideoStreamFramer.hh"

extern "C" void ipcam_request_idr(void *ctxt);

H265VideoLiveServerMediaSubsession*
H265VideoLiveServerMediaSubsession::createNew(UsageEnvironment& env, void *ctxt, Boolean reuseFirstSource) {
  return new H265VideoLiveServerMediaSubsession(env, ctxt, reuseFirstSource);
}

H265VideoLiveServerMediaSubsession::H265VideoLiveServerMediaSubsession(UsageEnvironment& env, void *ctxt, Boolean reuseFirstSource)
  : OnDemandServerMediaSubsession(env, reuseFirstSource),
    fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), mContext(ctxt) {
}

H265VideoLiveServerMediaSubsession::~H265VideoLiveServerMediaSubsession() {
  delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  H265VideoLiveServerMediaSubsession* subsess = (H265VideoLiveServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void H265VideoLiveServerMediaSubsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  H265VideoLiveServerMediaSubsession* subsess = (H265VideoLiveServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void H265VideoLiveServerMediaSubsession::checkForAuxSDPLine1() {
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

char const* H265VideoLiveServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

  if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
    // Note: For H265 video files, the 'config' information (used for several payload-format
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

FramedSource* H265VideoLiveServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 500; // kbps, estimate

  // Create the video source:
  H265LiveFramedSource* source = H265LiveFramedSource::createNew(envir(), mContext);
  if (source == NULL) return NULL;

  // Create a framer for the Video Elementary Stream:
  return H265VideoStreamFramer::createNew(envir(), source);
}

RTPSink* H265VideoLiveServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* /*inputSource*/) {
  return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

void H265VideoLiveServerMediaSubsession::getStreamParameters(
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
