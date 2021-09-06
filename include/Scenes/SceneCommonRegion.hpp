#pragma once

#include <cstdint>

struct MyCommonRegion
{
	uint16_t width;
	uint16_t height;
	uint16_t frameBufferNum;

	uint32_t frame;
	double deltatime;
	double fps;
};