#ifndef I3D_OS_S3E
#   define I3D_OS_S3E 1
#endif
#ifndef I3D_PLATFORM_S3E
#   define I3D_PLATFORM_S3E 1
#endif
#ifndef IW_APP_NAME
#   define IW_APP_NAME "Blackguard"
#endif
#ifndef IW_APP_VERSION
#   define IW_APP_VERSION "1.0"
#endif
#ifndef IW_ASSERTION_CHANNEL_AMAZONADS
#   define IW_ASSERTION_CHANNEL_AMAZONADS 1
#endif
#ifndef IW_ASSERTION_CHANNEL_PVRTEXTOOL
#   define IW_ASSERTION_CHANNEL_PVRTEXTOOL 1
#endif
#ifndef IW_GXFONT_TTF
#   define IW_GXFONT_TTF 1
#endif
#ifndef IW_MKF_AMAZON_ADS_ANDROID_SDK
#   define IW_MKF_AMAZON_ADS_ANDROID_SDK 1
#endif
#ifndef IW_MKF_AMAZON_ADS_IOS_SDK
#   define IW_MKF_AMAZON_ADS_IOS_SDK 1
#endif
#ifndef IW_MKF_APPCORE
#   define IW_MKF_APPCORE 1
#endif
#ifndef IW_MKF_BLACKGUARD_DEPLOYMENT
#   define IW_MKF_BLACKGUARD_DEPLOYMENT 1
#endif
#ifndef IW_MKF_FREELICENSE
#   define IW_MKF_FREELICENSE 1
#endif
#ifndef IW_MKF_IWGEOM
#   define IW_MKF_IWGEOM 1
#endif
#ifndef IW_MKF_IWGL
#   define IW_MKF_IWGL 1
#endif
#ifndef IW_MKF_IWGX
#   define IW_MKF_IWGX 1
#endif
#ifndef IW_MKF_IWGXFONT
#   define IW_MKF_IWGXFONT 1
#endif
#ifndef IW_MKF_IWRESMANAGER
#   define IW_MKF_IWRESMANAGER 1
#endif
#ifndef IW_MKF_IWUTIL
#   define IW_MKF_IWUTIL 1
#endif
#ifndef IW_MKF_LIBJPEG
#   define IW_MKF_LIBJPEG 1
#endif
#ifndef IW_MKF_LIBPNG
#   define IW_MKF_LIBPNG 1
#endif
#ifndef IW_MKF_PVRTEXTOOL
#   define IW_MKF_PVRTEXTOOL 1
#endif
#ifndef IW_MKF_S3EAMAZONADS
#   define IW_MKF_S3EAMAZONADS 1
#endif
#ifndef IW_MKF_S3E_DEFAULT
#   define IW_MKF_S3E_DEFAULT 1
#endif
#ifndef IW_MKF_TINICONV
#   define IW_MKF_TINICONV 1
#endif
#ifndef IW_MKF_ZLIB
#   define IW_MKF_ZLIB 1
#endif
#ifndef IW_PLATFORM_S3E
#   define IW_PLATFORM_S3E 1
#endif
#ifndef IW_SDK
#   define IW_SDK 1
#endif
#ifndef IW_TRACE_CHANNEL_AMAZONADS
#   define IW_TRACE_CHANNEL_AMAZONADS 1
#endif
#ifndef IW_TRACE_CHANNEL_AMAZONADS_VERBOSE
#   define IW_TRACE_CHANNEL_AMAZONADS_VERBOSE 2
#endif
#ifndef IW_TRACE_CHANNEL_PVRTEXTOOL
#   define IW_TRACE_CHANNEL_PVRTEXTOOL 1
#endif
#ifndef IW_TRACE_CHANNEL_PVRTEXTOOL_VERBOSE
#   define IW_TRACE_CHANNEL_PVRTEXTOOL_VERBOSE 2
#endif
#ifndef S3E_INCLUDE_LEGACY
#   define S3E_INCLUDE_LEGACY 1
#endif
#ifndef _CRT_SECURE_NO_DEPRECATE
#   define _CRT_SECURE_NO_DEPRECATE 1
#endif
#ifndef __S3E__
#   define __S3E__ 1
#endif

/* including "extra_body" specified by the mkb plugin */

#undef win32

#ifndef unix
    #define unix
#endif

#ifndef __unix
    #define __unix
#endif

/*
#ifndef _REENTRANT
    #define _REENTRANT
#endif
*/

#if !defined I3D_ARCH_ARM && !defined I3D_ARCH_AARCH64  && !defined I3D_ARCH_X86 && !defined I3D_ARCH_AMD64 && !defined I3D_ARCH_MIPS && !defined I3D_ARCH_NACLX86_64
    #if defined __ARMCC_VERSION || defined __ARMC__ || defined __arm__
        #define I3D_ARCH_ARM
    #elif defined __mipsel || defined __MIPSEL
        #define I3D_ARCH_MIPS
    #elif defined __amd64
        #define I3D_ARCH_AMD64
    #elif defined __aarch64__
        #define I3D_ARCH_AARCH64
    #else
        #define I3D_ARCH_X86
    #endif
#endif

#if defined I3D_ARCH_ARM || defined I3D_ARCH_X86 || defined I3D_ARCH_MIPS || defined I3D_ARCH_NACLX86_64
    #if defined I3D_ARCH_64_BIT
        #error "Preprocessor macro mismatch.  Current architecture is not 64 Bit"
    #endif
#else
    #ifdef I3D_ARCH_32_BIT
        #error "Preprocessor macro mismatch.  Current architecture is not 32 Bit"
#endif
#endif

#if defined I3D_ARCH_X86 && !defined __i386__
    #define __i386__
#endif

#if defined I3D_ARCH_ARM && !defined __ARM__
    #define __ARM__
#endif

#ifdef I3D_ARCH_NACLX86_64
    #undef __x86_64
    #undef __x86_64__
    #undef __amd64
    #undef __amd64__
    #define __ILP32__
#endif

#if defined __cplusplus && defined _MSC_VER && _MSC_VER < 1400 && !defined IW_WCHAR_T_DEFINED && !defined __clang__
    // wchar_t is a builtin in C++ .. but not for MSVC <= 2003
    typedef unsigned short wchar_t;
    #define IW_WCHAR_T_DEFINED
#endif

#if defined _MSC_VER && !defined __clang__
    #pragma warning (disable: 4786)
#endif
#if !defined I3D_ARCH_32_BIT && !defined I3D_ARCH_64_BIT
    #if defined I3D_ARCH_AARCH64 || defined I3D_ARCH_AMD64
        #define I3D_ARCH_64_BIT
    #else
        #define I3D_ARCH_32_BIT
    #endif
#endif

#if !defined __S3E_CL_WRAPPER
    #if !defined __INTELLISENSE__
        #error "Wrong compiler used.  Please run the Hub to configure Visual Studio"
    #endif
#endif

#undef _WIN32
#undef WIN32
#undef __linux__
#undef __linux
#undef __gnu_linux__
#undef linux

#if defined __i386__ && defined __ANDROID__
#ifndef S3E_ANDROID_X86
#   define S3E_ANDROID_X86 1
#endif
#ifndef S3E_ANDROID_NATIVE
#   define S3E_ANDROID_NATIVE 1
#endif
#endif

#if defined __arm__ && defined __ANDROID__
#ifndef S3E_ANDROID
#   define S3E_ANDROID 1
#endif
#ifndef S3E_ANDROID_NATIVE
#   define S3E_ANDROID_NATIVE 1
#endif
#endif
