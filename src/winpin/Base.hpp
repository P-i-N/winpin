#pragma once

#include <cstdint>

#define NOMINMAX
#include <Windows.h>
#include <WinUser.h>

#ifdef DEBUG
#include <cstdio>
#include <format>
#define LOG( ... ) printf( "%s\n", std::format( __VA_ARGS__ ).c_str() )
#else
#define LOG( ... ) \
	do \
	{ \
	} while ( false )
#endif
