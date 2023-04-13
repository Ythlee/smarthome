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
// Copyright (c) 1996-2021, Live Networks, Inc.  All rights reserved
// LIVE555 Proxy Server
// main program
#include <pthread.h>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

char const* progName;
UsageEnvironment* env;
UserAuthenticationDatabase* authDB = NULL;
UserAuthenticationDatabase* authDBForREGISTER = NULL;

// Default values of command-line parameters:
int verbosityLevel = 0;
Boolean streamRTPOverTCP = False;
portNumBits tunnelOverHTTPPortNum = 0;
portNumBits rtspServerPortNum = 554;
char* username = NULL;
char* password = NULL;
Boolean proxyREGISTERRequests = False;
char* usernameForREGISTER = NULL;
char* passwordForREGISTER = NULL;

static RTSPServer* createRTSPServer(Port port) {
  if (proxyREGISTERRequests) {
    return RTSPServerWithREGISTERProxying::createNew(*env, port, authDB, authDBForREGISTER, 65, streamRTPOverTCP, verbosityLevel, username, password);
  } else {
    return RTSPServer::createNew(*env, port, authDB);
  }
}

void usage() {
  *env << "Usage: error "
       << " <rtsp-url-1> ... <rtsp-url-n>\n";
  exit(1);
}

bool bFinish = false;
char rtspurl[256] = {0}; 

static void* _ExecStartVideo(void* arg){
  char *url = (char*)arg;
  // Increase the maximum size of video frames that we can 'proxy' without truncation.
  // (Such frames are unreasonably large; the back-end servers should really not be sending frames this large!)
  OutPacketBuffer::maxSize = 100000; // bytes

  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  *env << "LIVE555 Proxy Server\n"
       << "\t(LIVE555 Streaming Media library version "
       << LIVEMEDIA_LIBRARY_VERSION_STRING
       << "; licensed under the GNU LGPL)\n\n";

  // Check command-line arguments: optional parameters, then one or more rtsp:// URLs (of streams to be proxied):
  // Make sure that the remaining arguments appear to be "rtsp://" (or "rtsps://") URLs:
  // Do some additional checking for invalid command-line argument combinations:
  if (authDBForREGISTER != NULL && !proxyREGISTERRequests) {
    *env << "The '-U <username-for-REGISTER> <password-for-REGISTER>' option can be used only with -R\n";
    usage();
  }
  if (streamRTPOverTCP) {
    if (tunnelOverHTTPPortNum > 0) {
      *env << "The -t and -T options cannot both be used!\n";
      usage();
    } else {
      tunnelOverHTTPPortNum = (portNumBits)(~0); // hack to tell "ProxyServerMediaSession" to stream over TCP, but not using HTTP
    }
  }

#ifdef ACCESS_CONTROL
  // To implement client access control to the RTSP server, do the following:
  authDB = new UserAuthenticationDatabase;
  authDB->addUserRecord("username1", "password1"); // replace these with real strings
      // Repeat this line with each <username>, <password> that you wish to allow access to the server.
#endif

  // Create the RTSP server. Try first with the configured port number,
  // and then with the default port number (554) if different,
  // and then with the alternative port number (8554):
  RTSPServer* rtspServer;
  rtspServer = createRTSPServer(rtspServerPortNum);
  if (rtspServer == NULL) {
    if (rtspServerPortNum != 554) {
      *env << "Unable to create a RTSP server with port number " << rtspServerPortNum << ": " << env->getResultMsg() << "\n";
      *env << "Trying instead with the standard port numbers (554 and 8554)...\n";

      rtspServerPortNum = 554;
      rtspServer = createRTSPServer(rtspServerPortNum);
    }
  }
  if (rtspServer == NULL) {
    rtspServerPortNum = 8554;
    rtspServer = createRTSPServer(rtspServerPortNum);
  }
  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }

  // Create a proxy for each "rtsp://" URL specified on the command line:
  char const* proxiedStreamURL = url;//argv[i];
  char streamName[30];
  sprintf(streamName, "%s" , "proxyStream"); // there's just one stream; give it this name
  ServerMediaSession* sms = ProxyServerMediaSession::createNew(*env, rtspServer,
					   proxiedStreamURL, streamName,
					   username, password, tunnelOverHTTPPortNum, verbosityLevel);
  rtspServer->addServerMediaSession(sms);

  char* proxyStreamURL = rtspServer->rtspURL(sms);
  *env << "RTSP stream, proxying the stream \"" << proxiedStreamURL << "\"\n";
  *env << "\tPlay this stream using the URL: " << proxyStreamURL << "\n";
  strcpy(rtspurl, proxyStreamURL);
  delete[] proxyStreamURL;

  if (proxyREGISTERRequests) {
    *env << "(We handle incoming \"REGISTER\" requests on port " << rtspServerPortNum << ")\n";
  }

  // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
  // Try first with the default HTTP port (80), and then with the alternative HTTP
  // port numbers (8000 and 8080).

  if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
    *env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
  } else {
    *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
  }

  bFinish = true;
  // Now, enter the event loop:
  env->taskScheduler().doEventLoop(); // does not return

  return 0; // only to prevent compiler warning
}

int StartVideo(char* url, char* proxyurl) {
	pthread_t thread_t;
        pthread_create(&thread_t, NULL,  _ExecStartVideo, url);
	while(!bFinish){usleep(100000);};
	strcpy(proxyurl, rtspurl);
	printf("proxyurl : %s \n", proxyurl);
	return 0;

}
