#cmake_minimum_required(VERSION 2.8.9)
set(PANDORASERVER_OLDEST_VALIDATED_POLICIES_VERSION "3.0.0") # versions of cmake 
set(PANDORASERVER_NEWEST_VALIDATED_POLICIES_VERSION "3.13.2")
cmake_minimum_required(VERSION ${PANDORASERVER_OLDEST_VALIDATED_POLICIES_VERSION})

set(CMAKE_BUILD_TYPE Release)

if("${CMAKE_VERSION}" VERSION_LESS "${PANDORASERVER_NEWEST_VALIDATED_POLICIES_VERSION}")
    #Set and use the newest available cmake policies that are validated to work
    set(PANDORASERVER_CMAKE_POLICY_VERSION "${CMAKE_VERSION}")
else()
    set(PANDORASERVER_CMAKE_POLICY_VERSION "${PANDORASERVER_NEWEST_VALIDATED_POLICIES_VERSION}")
endif()

cmake_policy(VERSION ${PANDORASERVER_CMAKE_POLICY_VERSION})

foreach(pnew "") # Currently Empty
    if(POLICY ${pnew})
        cmake_policy(SET ${pnew} NEW)
    endif()
endforeach()
foreach(pold "") # Currently Empty
    if(POLICY ${pold})
        cmake_policy(SET ${pold} OLD)
    endif()

endforeach()

project(PandoraServer
        VERSION 1.0.0 # <major>[.<minor>[.<patch>[.<tweak>]]]0
        LANGUAGES CXX)
		
		
# ==== Define language standard configurations requiring at least c++11 standard
if(CMAKE_CXX_STANDARD EQUAL "98" )
    message(FATAL_ERROR "CMAKE_CXX_STANDARD:STRING=98 is not supported.")
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Coverage.")
endif()

if(UNIX)
set(CMAKE_CXX_FLAGS "-pthread -lboost_system -lboost_filesystem -lboost_thread -lboost_regex -lssl -lcrypto")
endif()

find_package(Threads REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD On)

#OPENSSL 

#find_library(OpenSSL_LIB)
if(WIN32)
find_package(OpenSSL)

if (OPENSSL_FOUND)
	message (STATUS "OPENSSL found")
	message (STATUS "OpenSSL Include directories:" ${OPENSSL_INCLUDE_DIR})
	message (STATUS "OpenSSL Include directories:" ${OPENSSL_INCLUDE_DIR})
else()
	message("OpenSSL not found")
	FATAL_ERROR("OpenSSL not found")
endif()
endif()

include_directories(${OPENSSL_INCLUDE_DIR})
link_directories(${OPENSSL_LIBRARIES})
#SET(OPENSSL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3dparty/)
#SET(OPENSSL_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3dparty/openssl/lib)

include_directories(${OPENSSL_INCLUDE_DIR})

link_directories(${OPENSSL_LIB_DIR})

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0501)
    #target_compile_definitions(PandoraServer PUBLIC -D_WIN32_WINNT=0x0501)
endif()

#if(WIN32)
#    set(BOOST_ROOT "C:/MinGW32/MinGW")
#endif ()

#SET (Boost_FIND_REQUIRED TRUE)
#SET (Boost_FIND_QUIETLY TRUE)
#SET (Boost_DEBUG FALSE)
#set (Boost_USE_MULTITHREADED TRUE)
#set (Boost_USE_STATIC_LIBS TRUE)
#SET (Boost_ADDITIONAL_VERSIONS "1.64" "1.64.0")

#SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "E:/boost_1_64_0")
#SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${CMAKE_CURRENT_SOURCE_DIR}3dparty/boost/release")

# use forward slashed in cmake !!! C:/Users/Shreedhar/Desktop/test_CL/CMakeLists


#set(BOOST_ROOT "E:/Programme/Boost/boost_1_64_0")
#set(BOOST_LIBRARYDIR "E:/Programme/Boost/boost_1_64_0/lib")

#set(Boost_USE_STATIC_LIBS ON)
#set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_RUNTIME OFF)
#set(Boost_DEBUG ON)
#set(Boost_DETAILED_FAILURE_MSG ON)
#set(Boost_COMPILER "-vc141")
#set(Boost_ARCHITECTURE "-x64")

#set(Boost_INCLUDE_DIR "E:/Programme/Boost/boost_1_64_0/include")
#set(Boost_LIBRARY_DIR "E:1. Job tuts/libs/boost/release")

if(NOT DEFINED BOOST_INCLUDE_DIR)
    message(FATAL_ERROR "-- Please Include path to Boost inlude dir. set Boost_INCLUDE_DIR=PATH_TO_INCLUDE ")
else()
	message("-- BOOST_INCLUDE_DIR = " ${BOOST_INCLUDE_DIR})
endif()
if(NOT DEFINED BOOST_LIBRARY_DIR)
    message(FATAL_ERROR "-- Please Include boost library set Boost_LIBRARY_DIR=PATH_TO_DIR")
else()
	message("-- BOOST_LIBRARY_DIR = " ${BOOST_LIBRARY_DIR})
endif()

#find_package(Boost COMPONENTS system filesystem regex thread chrono REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})


include_directories("3dparty/boost/")
include_directories("3dparty/openssl/")
include_directories( "/include" )

include_directories(include)
#However, the file(GLOB...) allows for wildcard additions:
#set(SOURCES src/*.cpp src/http/*.cpp)
FILE(GLOB SOURCES src/*.cpp
				 include/*.h
				 src/http/*.cpp
				 include/http/*.hpp
				 include/http/*.h)


add_executable(PandoraServer ${SOURCES}) 
target_link_libraries( PandoraServer LINK_PUBLIC ${Boost_LIBRARIES} )
target_link_libraries( PandoraServer ${CMAKE_THREAD_LIBS_INIT})

	set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bin)
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
	
	set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bin)

endif()

if(WIN32)
	target_link_libraries(PandoraServer "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/openssl/lib/libeay32.lib" 
                	                    "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/openssl/lib/ssleay32.lib"
        	                            "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/openssl/lib/libssl.lib" 
                        	            "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/openssl/lib/libcrypto.lib")

install(TARGETS PandoraServer EXPORT PandoraServer.exe RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

 endif()


message("Default installation folder is " ${CMAKE_INSTALL_PREFIX})

configure_file("ok-server.conf" ${CMAKE_INSTALL_PREFIX}/bin/ok-server.conf COPYONLY)

#link_directories("E:/Programme/Boost/boost_1_64_0/lib")

# Build the different applications
#add_subdirectory( src ) will be looking for CMakeLists.txt files in src directory

#install the includes
#add_subdirectory( include )


