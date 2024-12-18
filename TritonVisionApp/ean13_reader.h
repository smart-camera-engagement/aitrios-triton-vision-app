#pragma once

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include "pattern.h"
#include <cassert>
#include <set>
#include <unordered_map>
#include <fstream>
#include <opencv2/opencv.hpp>


#ifdef max
#undef max
#endif


#ifdef min
#undef min
#endif

const std::array<std::array<int, 4>, 10> L_PATTERNS = {
	3, 2, 1, 1, // 0
	2, 2, 2, 1, // 1
	2, 1, 2, 2, // 2
	1, 4, 1, 1, // 3
	1, 1, 3, 2, // 4
	1, 2, 3, 1, // 5
	1, 1, 1, 4, // 6
	1, 3, 1, 2, // 7
	1, 2, 1, 3, // 8
	3, 1, 1, 2, // 9
};

const std::array<std::array<int, 4>, 20> L_AND_G_PATTERNS = {
	3, 2, 1, 1, // 0
	2, 2, 2, 1, // 1
	2, 1, 2, 2, // 2
	1, 4, 1, 1, // 3
	1, 1, 3, 2, // 4
	1, 2, 3, 1, // 5
	1, 1, 1, 4, // 6
	1, 3, 1, 2, // 7
	1, 2, 1, 3, // 8
	3, 1, 1, 2, // 9
	// reversed
	1, 1, 2, 3, // 10
	1, 2, 2, 2, // 11
	2, 2, 1, 2, // 12
	1, 1, 4, 1, // 13
	2, 3, 1, 1, // 14
	1, 3, 2, 1, // 15
	4, 1, 1, 1, // 16
	2, 1, 3, 1, // 17
	3, 1, 2, 1, // 18
	2, 1, 1, 3, // 19
};


template <typename Container, typename Value>
auto Find(Container& c, const Value& v) -> decltype(std::begin(c)) {
	return std::find(std::begin(c), std::end(c), v);
}

template <typename T = char>
T ToDigit(int i)
{
	if (i < 0 || i > 9)
		return false;
	return static_cast<T>('0' + i);
}

template <typename Container, typename Value>
int IndexOf(const Container& c, const Value& v) {
	auto i = Find(c, v);
	return i == std::end(c) ? -1 : narrow_cast<int>(std::distance(std::begin(c), i));
}

template <typename CP, typename PP>
static float PatternMatchVariance(const CP* counters, const PP* pattern, size_t length, float maxIndividualVariance)
{
	int total = std::accumulate(counters, counters + length, 0);
	int patternLength = std::accumulate(pattern, pattern + length, 0);
	if (total < patternLength) {
		// If we don't even have one pixel per unit of bar width, assume this is too small
		// to reliably match, so fail:
		return std::numeric_limits<float>::max();
	}

	float unitBarWidth = (float)total / patternLength;
	maxIndividualVariance *= unitBarWidth;

	float totalVariance = 0.0f;
	for (size_t x = 0; x < length; ++x) {
		float variance = std::abs(counters[x] - pattern[x] * unitBarWidth);
		if (variance > maxIndividualVariance) {
			return std::numeric_limits<float>::max();
		}
		totalVariance += variance;
	}
	return totalVariance / total;
}

template <typename Counters, typename Pattern>
static float PatternMatchVariance(const Counters& counters, const Pattern& pattern, float maxIndividualVariance) {
	assert(Size(counters) == Size(pattern));
	return PatternMatchVariance(counters.data(), pattern.data(), counters.size(), maxIndividualVariance);
}

struct PartialResult
{
	std::string txt;
	PatternView end;
	PartialResult() { txt.reserve(14); }
};

std::set<std::string> DoDecode(const std::vector<uint8_t>& image, int width, int height);

std::string DecodeWithRotation(const cv::Mat buffer_image);
