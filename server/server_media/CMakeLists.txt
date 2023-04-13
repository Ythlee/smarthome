#设置CMake支持的最低版本
cmake_minimum_required(VERSION 3.0)


SET(CMAKE_CXX_COMPILER g++)
SET(CMAKE_C_COMPILER gcc)
SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "set build type to Release")

SET(CMAKE_C_FLAGS "-fexceptions -fPIE -fPIC")
SET(CMAKE_CXX_FLAGS "-fexceptions -fpermissive -frtti -fPIE -fPIC -O3 -std=c++11")

#live so
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/live/BasicUsageEnvironment/include)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/live/BasicUsageEnvironment BasicUsageEnvironmentSrcS)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/live/groupsock/include)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/live/groupsock groupsockSrcS)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/live/hlsProxy/include)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/live/hlsProxy hlsProxySrcS)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/live/liveMedia/include)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/live/liveMedia liveMediaSrcS)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/live/mediaServer)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/live/mediaServer mediaServerSrcS)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/live//UsageEnvironment/include)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/live/UsageEnvironment UsageEnvironmentSrcS)


#设置库路径
SET(LIBRARY_OUTPUT_PATH     ${PROJECT_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin)

#实现文件
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/server)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/server ServerSrcS)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src SrcS)

#source directory
ADD_EXECUTABLE(server_media ${SrcS} ${ServerSrcS} ${BasicUsageEnvironmentSrcS} ${groupsockSrcS} ${liveMediaSrcS} ${UsageEnvironmentSrcS})

TARGET_LINK_LIBRARIES(server_media
			ssl
			crypto
			sqlite3
                        )
