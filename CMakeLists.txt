cmake_minimum_required(VERSION 3.0)

# ToDo: fix find_library mess
# ToDo: add proper include dir handling

project(visuals)


file(GLOB FILES *.cpp *.hpp *.c *.h ox_src/*.cpp ox_src/*.h)

if (NOT WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -Wall")

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -Wall")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOFX_BEAT_STANDALONE")

    find_package(Threads)
    find_library(ATOMIC_LIBRARY NAMES atomic atomic.so.1 libatomic.so.1 HINTS /usr/lib/${CMAKE_C_LIBRARY_ARCHITECTURE} PATH_SUFFIXES )
    find_library(PORTAUDIO_LIBRARY NAME portaudio HINTS /usr/lib/${CMAKE_C_LIBRARY_ARCHITECTURE})
    find_library(VORBIS_LIBRARY NAME vorbis HINTS /usr/lib/${CMAKE_C_LIBRARY_ARCHITECTURE})
    find_library(VORBIS_FILE_LIBRARY NAME vorbisfile HINTS /usr/lib/${CMAKE_C_LIBRARY_ARCHITECTURE})
    find_library(BOOST_FILESYSTEM_LIBRARY NAME boost_filesystem HINTS /usr/lib/${CMAKE_C_LIBRARY_ARCHITECTURE})
    find_library(BOOST_PROGRAM_OPTIONS_LIBRARY NAME boost_program_options HINTS /usr/lib/${CMAKE_C_LIBRARY_ARCHITECTURE})
    find_library(BOOST_SYSTEM_LIBRARY NAME boost_system HINTS /usr/lib/${CMAKE_CLIBRARY_ARCHITECTURE})
endif()

if (WIN32)
    include_directories(libvorbis/include libogg/include portaudio/include)
endif()

add_executable(visuals ${FILES})

if (NOT WIN32)
    target_link_libraries(visuals
        ${CMAKE_DL_LIBS}
        ${CMAKE_THREAD_LIBS_INIT}
	${ATOMIC_LIBRARY}
	${BOOST_FILESYSTEM_LIBRARY}
	${BOOST_PROGRAM_OPTIONS_LIBRARY}
	${BOOST_SYSTEM_LIBRARY}
        ${PORTAUDIO_LIBRARY}
        ${VORBIS_LIBRARY}
        ${VORBIS_FILE_LIBRARY})
endif()

if (WIN32)
    target_link_libraries(visuals 
        C:/code/portaudiobuild/Debug/portaudio_static_x86.lib
        C:/code/visuals/libogg/win32/VS2010/Win32/Debug/libogg_static.lib
        C:/code/visuals/libvorbis/win32/VS2010/Win32/Debug/libvorbis_static.lib
        C:/code/visuals/libvorbis/win32/VS2010/Win32/Debug/libvorbisfile_static.lib
    )
endif()
