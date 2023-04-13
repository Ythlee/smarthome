#ifndef __DEVICE_VIDEO__
#define __DEVICE_VIDEO__

/* link video server*/
int ConnectVideoServer(const char* sIP, const int nPort, int* nFd);

/* request video url*/
int RequestVideoUrl(const int nFd, char* url);

/* close link */
int UnlinkVideoServer(const int nFd);

#endif
