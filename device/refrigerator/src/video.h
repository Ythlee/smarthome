#ifndef __DEVICE_VIDEO__
#define __DEVICE_VIDEO__

/* link video server*/
int ConnectVideoServer(const char* sIP, const int nPort, int &nFd);

/* start device video */
int StartVideo(int* fd);


/* stop device video */
int StopVideo(int fd);


#endif
