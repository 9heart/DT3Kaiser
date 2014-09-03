#==============================================================================
#	
#	File: CMakeLists.txt	(for Android)
#
#	Copyright (C) 2000-2013 by Smells Like Donkey, Inc. All rights reserved.
#
#	This file is subject to the terms and conditions defined in
#	file 'LICENSE.txt', which is part of this source code package.
#	
#==============================================================================

PROJECT (GameAndroid)
cmake_minimum_required(VERSION 2.8)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fno-short-enums -fno-strict-aliasing -std=c++11 -march=armv7-a -mfloat-abi=softfp -mfpu=neon -O3" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fno-short-enums -fno-strict-aliasing -finline-limit=64 -march=armv7-a -mfloat-abi=softfp -mfpu=neon -O3" )
set( CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}" )

set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_C_FLAGS}" )
set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_C_FLAGS}" )
set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_C_FLAGS}" )

set( NDK_PLATFORM_LIBS "/Developer/android-ndk-r9d/platforms/android-18/arch-${ANDROID_ARCH_NAME}/usr/lib")

set( NDK_PLATFORM_INCLUDES "/Developer/android-ndk-r9d/platforms/android-18/arch-${ANDROID_ARCH_NAME}/usr/include")
include_directories(SYSTEM ${NDK_PLATFORM_INCLUDES})

set( NDK_APP_GLUE "/Developer/android-ndk-r9d/sources/android/native_app_glue")

add_definitions(-DANDROID)
add_definitions(-D__STDC_CONSTANT_MACROS)
add_definitions(-DDT3_ANDROID_ACTIVITY_JNI_TYPE="com/smellslikedonkey/Kaiser/MainActivity")

add_definitions(-DJAVA_REQUEST_PRODUCT_INFO_CB=Java_com_smellslikedonkey_Kaiser_MainActivity_requestProductsInfoCB)
add_definitions(-DJAVA_RESTORE_PRODUCT_CB=Java_com_smellslikedonkey_Kaiser_MainActivity_restoreProductCB)
add_definitions(-DJAVA_PURCHASE_PRODUCT_CB=Java_com_smellslikedonkey_Kaiser_MainActivity_purchaseProductCB)

#
# Useful variables and macros
# 

enable_language(ASM)

MACRO(HEADER_DIRECTORIES new_list return_list)
    SET(dir_list "")
    
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

set (PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ENGINE_ROOT "${PROJECT_SOURCE_DIR}/../..")

#
# build dependencies
#

#
# Build DT3Android
#

set (PROJECT_MODULE "${ENGINE_ROOT}/src/DT3Android/")
if (EXISTS "${PROJECT_MODULE}")

	# Compile files
	file(GLOB_RECURSE PROJECT_SRCS 
		"${ENGINE_ROOT}/src/DT3Android/HAL.hpp"
		"${ENGINE_ROOT}/src/DT3Android/HAL.cpp"
	)
	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})

	# Build include directories
	HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
	include_directories(${HEADER_INCLUDES})

else (EXISTS "${PROJECT_MODULE}")
	MESSAGE(FATAL_ERROR "DT3Android is required for building")
	
endif (EXISTS "${PROJECT_MODULE}")

#
# Build DT3Core
#

set (PROJECT_MODULE "${ENGINE_ROOT}/src/DT3Core/")
if (EXISTS "${PROJECT_MODULE}")

	# DT3Core/3rdParty
	set (PROJECT_SUBDIR "${ENGINE_ROOT}/src/DT3Core/3rdParty")
	add_subdirectory(${PROJECT_SUBDIR} "${CMAKE_CURRENT_BINARY_DIR}/DT3Core/3rdParty")
	set (PROJECT_SOURCE ${PROJECT_SUBDIR} ${PROJECT_SOURCE})

	#Add libraries
	set(MODULE_LIBS ${MODULE_LIBS} "freetype" "zlib" "lpng" "md5" "jenkins" "libjpeg.a")
	set(MODULE_LIBS_DIRS ${MODULE_LIBS_DIRS} "${ENGINE_ROOT}/src/DT3Core/3rdParty/libjpeg-turbo-1.3.0/libjpeg-turbo-android/lib/${ANDROID_NDK_ABI_NAME}" )

	# Library includes
	include_directories("${ENGINE_ROOT}/src/DT3Core/3rdParty/zlib-1.2.5")
	include_directories("${ENGINE_ROOT}/src/DT3Core/3rdParty/freetype-2.4.7/include")
	include_directories("${ENGINE_ROOT}/src/DT3Core/3rdParty/lpng151")
	include_directories("${ENGINE_ROOT}/src/DT3Core/3rdParty/md5-cc")
	include_directories("${ENGINE_ROOT}/src/DT3Core/3rdParty/libjpeg-turbo-1.3.0/libjpeg-turbo-android/include")

	# Compile files
	file(GLOB_RECURSE PROJECT_SRCS 
		"${ENGINE_ROOT}/src/DT3Core/Components/*"
		"${ENGINE_ROOT}/src/DT3Core/Devices/*"
		"${ENGINE_ROOT}/src/DT3Core/Entry/*"
		"${ENGINE_ROOT}/src/DT3Core/Objects/*"
		"${ENGINE_ROOT}/src/DT3Core/Resources/*"
		"${ENGINE_ROOT}/src/DT3Core/Scripting/*"
		"${ENGINE_ROOT}/src/DT3Core/System/*"
		"${ENGINE_ROOT}/src/DT3Core/Tools/*"
		"${ENGINE_ROOT}/src/DT3Core/Transitions/*"
		"${ENGINE_ROOT}/src/DT3Core/Types/*.hpp"
		"${ENGINE_ROOT}/src/DT3Core/Types/*.cpp"
		"${ENGINE_ROOT}/src/DT3Core/Types/*.S"
		"${ENGINE_ROOT}/src/DT3Core/World/*"
	)
	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})

	# Build include directories
	include_directories("${ENGINE_ROOT}/src")
	
else (EXISTS "${PROJECT_MODULE}")
	MESSAGE(FATAL_ERROR "DT3Core is required for building")
	
endif (EXISTS "${PROJECT_MODULE}")


#
# Build DTPortalSDK
#

set (PROJECT_MODULE "${ENGINE_ROOT}/src/DTPortalSDK/")
if (EXISTS "${PROJECT_MODULE}")

	# DTPortalSDK/DTPortalLib/3rdParty
	set (PROJECT_SUBDIR "${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/3rdParty")
	add_subdirectory(${PROJECT_SUBDIR} "${CMAKE_CURRENT_BINARY_DIR}/DTPortalSDK/DTPortalLib/3rdParty")
	set (PROJECT_SOURCE ${PROJECT_SUBDIR} ${PROJECT_SOURCE})

	#Add libraries
	set(MODULE_LIBS ${MODULE_LIBS} "jansson" "libtomcrypt")

	# Library includes
	include_directories("${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/3rdParty/jansson-2.5/src")
	include_directories("${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/3rdParty/libtomcrypt-1.17/src/headers")

	# Compile files
	file(GLOB_RECURSE PROJECT_SRCS 
		"${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/DTPortalAnalytics.cpp"
		"${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/DTPortalAppStoreValidator.cpp"
		"${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/DTPortalCommon.cpp"
		"${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/DTPortalHighScores.cpp"
		"${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/DTPortalNetwork.cpp"
		"${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/DTPortalSDK.cpp"
		"${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/DTPortalStrings.cpp"
		"${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/DTPortalTasks.cpp"
		"${ENGINE_ROOT}/src/DTPortalSDK/DTPortalLib/DTPortalTextBlob.cpp"
	)
	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})

	# Build include directories
	HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
	include_directories(${HEADER_INCLUDES})

endif (EXISTS "${PROJECT_MODULE}")

#
# Build Game specific files
#

set (PROJECT_MODULE "${ENGINE_ROOT}/src_game/")
if (EXISTS "${PROJECT_MODULE}")

	# Compile files
	file(GLOB_RECURSE PROJECT_SRCS 
		"${ENGINE_ROOT}/src_game/AppConfig.cpp"
		"${ENGINE_ROOT}/src_game/Kaiser/*.cpp"
	)
	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})

	# Build include directories
	HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
	include_directories(${HEADER_INCLUDES})
	
else (EXISTS "${PROJECT_MODULE}")
	MESSAGE(FATAL_ERROR "Game Files are required for building")
	
endif (EXISTS "${PROJECT_MODULE}")

#
# Build DT3OpenAL
#

set (PROJECT_MODULE "${ENGINE_ROOT}/src/DT3OpenAL/")
if (EXISTS "${PROJECT_MODULE}")

	# DT3OpenAL/3rdParty
	set (PROJECT_SUBDIR "${ENGINE_ROOT}/src/DT3OpenAL/3rdParty")
	add_subdirectory(${PROJECT_SUBDIR} "${CMAKE_CURRENT_BINARY_DIR}/DT3OpenAL/3rdParty")
	set (PROJECT_SOURCE ${PROJECT_SUBDIR} ${PROJECT_SOURCE})

	#Add libraries
	set(MODULE_LIBS ${MODULE_LIBS} "openal" )
	
	#Library includes
	include_directories("${ENGINE_ROOT}/src/DT3OpenAL/3rdParty/openal-soft-android/include")
	
	# Compile files
	file(GLOB_RECURSE PROJECT_SRCS 
		"${ENGINE_ROOT}/src/DT3OpenAL/DeviceAudioOpenAL.hpp"
		"${ENGINE_ROOT}/src/DT3OpenAL/DeviceAudioOpenAL.cpp"
	)
	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})

	# Build include directories
	HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
	include_directories(${HEADER_INCLUDES})
	
endif (EXISTS "${PROJECT_MODULE}")

#
# Build DT3HWVideoPlayer
#

#set (PROJECT_MODULE "${ENGINE_ROOT}/src/DT3HWVideoPlayer/")
#if (EXISTS "${PROJECT_MODULE}")
#
#	#Add libraries
#	set(MODULE_LIBS ${MODULE_LIBS} "libswscale.a" "libavformat.a" "libavcodec.a" "libavfilter.a" "libavutil.a" "libswresample.a"  )
#	set(MODULE_LIBS_DIRS ${MODULE_LIBS_DIRS} "${ENGINE_ROOT}/src/DT3HWVideoPlayer/3rdParty/ffmpeg-2.0.2/ffmpeg-android/lib/${ANDROID_NDK_ABI_NAME}" )
#	
#	#Library includes
#	include_directories("${ENGINE_ROOT}/src/DT3HWVideoPlayer/3rdParty/ffmpeg-2.0.2/ffmpeg-android/include")
#
#	# Compile files
#	file(GLOB_RECURSE PROJECT_SRCS 
#		"${ENGINE_ROOT}/src/DT3HWVideoPlayer/FFmpeg/*.cpp"
#		"${ENGINE_ROOT}/src/DT3HWVideoPlayer/ScriptingVideoTexture.cpp"
#		"${ENGINE_ROOT}/src/DT3HWVideoPlayer/ComponentGUIVideoPlayer.cpp"
#	)
#	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})
#
#	# Build include directories
#	HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
#	include_directories(${HEADER_INCLUDES})
#	
#endif (EXISTS "${PROJECT_MODULE}")


#
# Build DT3OpenGL
#

set (PROJECT_MODULE "${ENGINE_ROOT}/src/DT3OpenGL/")
if (EXISTS "${PROJECT_MODULE}")

	file(GLOB_RECURSE PROJECT_SRCS 
		"${ENGINE_ROOT}/src/DT3OpenGL/*.hpp"
		"${ENGINE_ROOT}/src/DT3OpenGL/*.cpp"
	)
	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})

	# Build include directories
	HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
	include_directories(${HEADER_INCLUDES})
	
endif (EXISTS "${PROJECT_MODULE}")


#
# Build DT3Sockets
#

set (PROJECT_MODULE "${ENGINE_ROOT}/src/DT3Sockets/")
if (EXISTS "${PROJECT_MODULE}")

	file(GLOB_RECURSE PROJECT_SRCS 
		"${ENGINE_ROOT}/src/DT3Sockets/*.hpp"
		"${ENGINE_ROOT}/src/DT3Sockets/*.cpp"
	)
	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})

	# Build include directories
	HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
	include_directories(${HEADER_INCLUDES})
	
endif (EXISTS "${PROJECT_MODULE}")

#
# Build DT3Sprites
#

#set (PROJECT_MODULE "${ENGINE_ROOT}/src/DT3Sprites/")
#if (EXISTS "${PROJECT_MODULE}")
#
#	file(GLOB_RECURSE PROJECT_SRCS 
#		"${ENGINE_ROOT}/src/DT3Sprites/*.hpp"
#		"${ENGINE_ROOT}/src/DT3Sprites/*.cpp"
#	)
#	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})
#
#	# Build include directories
#	HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
#	include_directories(${HEADER_INCLUDES})
#	
#endif (EXISTS "${PROJECT_MODULE}")

#
# Build DT3InAppPurchases
#

set (PROJECT_MODULE "${ENGINE_ROOT}/src/DT3InAppPurchases/")
if (EXISTS "${PROJECT_MODULE}")

	file(GLOB_RECURSE PROJECT_SRCS 
		"${ENGINE_ROOT}/src/DT3InAppPurchases/InAppPurchases.cpp"
		"${ENGINE_ROOT}/src/DT3InAppPurchases/InAppPurchasesProduct.cpp"
		"${ENGINE_ROOT}/src/DT3InAppPurchases/Android/InAppPurchasesAndroid.cpp"
	)
	SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})

	# Build include directories
	HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
	include_directories(${HEADER_INCLUDES})
	
endif (EXISTS "${PROJECT_MODULE}")

#
# Native Activity
#

file(GLOB_RECURSE PROJECT_SRCS 
	"${NDK_APP_GLUE}/android_native_app_glue.h"
	"${NDK_APP_GLUE}/android_native_app_glue.c"
	"${PROJECT_SOURCE_DIR}/AppNative.hpp"
	"${PROJECT_SOURCE_DIR}/AppNative.cpp"
)
SET(PROJECT_SOURCE ${PROJECT_SOURCE} ${PROJECT_SRCS})

# Build include directories
HEADER_DIRECTORIES("${PROJECT_SRCS}" HEADER_INCLUDES)
include_directories(${HEADER_INCLUDES})

#
# Build!
#

link_directories(${NDK_PLATFORM_LIBS} ${MODULE_LIBS_DIRS})

add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCE})

set_target_properties(${PROJECT_NAME} PROPERTIES 
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/project/libs/${ANDROID_ABI}"
)

target_link_libraries (${PROJECT_NAME} 
	${NDK_PLATFORM_LIBS}/libm.so 
	${NDK_PLATFORM_LIBS}/libdl.so 
	${NDK_PLATFORM_LIBS}/libc.so 
	${NDK_PLATFORM_LIBS}/liblog.so 
	${NDK_PLATFORM_LIBS}/libGLESv2.so 
	${NDK_PLATFORM_LIBS}/libEGL.so 
	${NDK_PLATFORM_LIBS}/libandroid.so 
	${NDK_PLATFORM_LIBS}/libstdc++.a 
	${NDK_PLATFORM_LIBS}/libOpenSLES.so
	${MODULE_LIBS}
) 

