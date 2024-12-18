#include "ean13_reader.h"

#define CHECK(A) if(!(A)) return false;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr int CHAR_LEN = 4;
constexpr auto END_PATTERN = FixedPattern<3, 3>{ 1, 1, 1 };
constexpr auto MID_PATTERN = FixedPattern<5, 5>{ 1, 1, 1, 1, 1 };
static const int FIRST_DIGIT_ENCODINGS[] = { 0x00, 0x0B, 0x0D, 0x0E, 0x13, 0x19, 0x1C, 0x15, 0x16, 0x1A };

// The GS1 specification has the following to say about quiet zones
// Type: EAN-13 | EAN-8 | UPC-A | UPC-E | EAN Add-on | UPC Add-on
// QZ L:   11   |   7   |   9   |   9   |     7-12   |     9-12
// QZ R:    7   |   7   |   9   |   7   |        5   |        5

constexpr float QUIET_ZONE_LEFT = 6;
constexpr float QUIET_ZONE_RIGHT_EAN = 3; // used to be 6, see #526 and #558
constexpr float QUIET_ZONE_RIGHT_UPC = 6;
constexpr float QUIET_ZONE_ADDON = 3;

//* Attempts to decode a sequence of black/white lines into single
//* digit.
//*
//* @param counters the counts of runs of observed black/white/black/... values
//* @param patterns the list of patterns to compare the contents of counters to
//* @param requireUnambiguousMatch the 'best match' must be better than all other matches
//* @return The decoded digit index, -1 if no pattern matched

template <typename Counters, typename Patterns>
static int DecodeDigit(const Counters& counters, const Patterns& patterns, float maxAvgVariance,
	float maxIndividualVariance, bool requireUnambiguousMatch = true)
{
	float bestVariance = maxAvgVariance; // worst variance we'll accept
	constexpr int INVALID_MATCH = -1;
	int bestMatch = INVALID_MATCH;
	for (int i = 0; i < Size(patterns); i++) {
		float variance = PatternMatchVariance(counters, patterns[i], maxIndividualVariance);
		if (variance < bestVariance) {
			bestVariance = variance;
			bestMatch = i;
		}
		else if (requireUnambiguousMatch && variance == bestVariance) {
			// if we find a second 'best match' with the same variance, we can not reliably report to have a suitable match
			bestMatch = INVALID_MATCH;
		}
	}
	return bestMatch;
}


static bool DecodeDigit(const PatternView& view, std::string& txt, int* lgPattern = nullptr)
{
	// These two values are critical for determining how permissive the decoding will be.
	// We've arrived at these values through a lot of trial and error. Setting them any higher
	// lets false positives creep in quickly.
	static constexpr float MAX_AVG_VARIANCE = 0.48f;
	static constexpr float MAX_INDIVIDUAL_VARIANCE = 0.7f;

	int bestMatch =
		lgPattern ? DecodeDigit(view, L_AND_G_PATTERNS, MAX_AVG_VARIANCE, MAX_INDIVIDUAL_VARIANCE, false)
		: DecodeDigit(view, L_PATTERNS, MAX_AVG_VARIANCE, MAX_INDIVIDUAL_VARIANCE, false);
	if (bestMatch == -1)
		return false;

	txt += ToDigit(bestMatch % 10);

	if (lgPattern) {
		int bestMatchbool = int(bestMatch >= 10);
		(*lgPattern <<= 1) |= bestMatchbool;
	}

	return true;

}

static bool DecodeDigits(int digitCount, PatternView& next, std::string& txt, int* lgPattern = nullptr)
{
	for (int j = 0; j < digitCount; ++j, next.skipSymbol())
		if (!DecodeDigit(next, txt, lgPattern))
			return false;
	return true;
}


static bool EAN13(PartialResult& res, PatternView begin)
{
	auto mid = begin.subView(27, MID_PATTERN.size());
	auto end = begin.subView(56, END_PATTERN.size());

	CHECK(end.isValid() && IsRightGuard(end, END_PATTERN, QUIET_ZONE_RIGHT_EAN) && IsPattern(mid, MID_PATTERN));

	auto next = begin.subView(END_PATTERN.size(), CHAR_LEN);
	res.txt = " "; // make space for lgPattern character
	int lgPattern = 0;

	CHECK(DecodeDigits(6, next, res.txt, &lgPattern));

	next = next.subView(MID_PATTERN.size(), CHAR_LEN);

	CHECK(DecodeDigits(6, next, res.txt));

	int i = IndexOf(FIRST_DIGIT_ENCODINGS, lgPattern);
	CHECK(i != -1);
	res.txt[0] = ToDigit(i);

	res.end = end;
	return true;
}


static bool decodePattern(PatternView& next, PartialResult& res)
{
	const int minSize = 3 + 6 * 4 + 6; // UPC-E

	next = FindLeftGuard(next, minSize, END_PATTERN, QUIET_ZONE_LEFT);
	if (!next.isValid())
		return false;

	auto begin = next;

	if (!EAN13(res, begin))
		return false;

	return true;
}

static bool getPatternRow(const std::vector<uint8_t>& image, int height, int width, int rowNumber, std::vector<uint16_t>& res)
{
	const int stride = 1;
	auto* begin = &image[rowNumber * width];
	auto* end = begin + width * stride;

	auto* lastPos = begin;
	bool lastVal = false;

	res.clear();

	for (const auto* p = begin; p != end; p += 1) {
		bool val = *p == 0;
		if (val != lastVal) {
			res.push_back(narrow_cast<std::vector<uint16_t>::value_type>((p - lastPos) / stride));
			lastVal = val;
			lastPos = p;
		}
	}

	res.push_back(narrow_cast<std::vector<uint16_t>::value_type>((end - lastPos) / stride));

	if (*(end - stride) == 0)
		res.push_back(0); // last value is number of white pixels, here 0

	return true;
}

// Function to rotate the image by a specified angle
std::vector<uint8_t> RotateImage(const std::vector<uint8_t>& image, int width, int height, float angle) {
	int newHeight = static_cast<int>(std::abs(height * std::cos(angle)) + std::abs(width * std::sin(angle)));
	int newWidth = static_cast<int>(std::abs(width * std::cos(angle)) + std::abs(height * std::sin(angle)));
	std::vector<uint8_t> rotatedImage(newHeight * newWidth, 255); // Initialize with white pixels

	float centerX = width / 2.0f;
	float centerY = height / 2.0f;
	float newCenterX = newWidth / 2.0f;
	float newCenterY = newHeight / 2.0f;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int newX = static_cast<int>((x - centerX) * std::cos(angle) - (y - centerY) * std::sin(angle) + newCenterX);
			int newY = static_cast<int>((x - centerX) * std::sin(angle) + (y - centerY) * std::cos(angle) + newCenterY);
			if (newX >= 0 && newX < newWidth && newY >= 0 && newY < newHeight) {
				rotatedImage[newY * newWidth + newX] = image[y * width + x];
			}
		}
	}

	return rotatedImage;
}
std::string GetMostFrequentResult(const std::set<std::string>& results) {
	std::unordered_map<std::string, int> frequencyMap;
	for (const auto& result : results) {
		frequencyMap[result]++;
	}

	std::string mostFrequentResult;
	int maxFrequency = 0;
	for (const auto& [result, frequency] : frequencyMap) {
		if (frequency > maxFrequency) {
			maxFrequency = frequency;
			mostFrequentResult = result;
		}
	}

	return mostFrequentResult;
}

void SaveImageAsJPG(const std::vector<uint8_t>& image, int height, int width, const std::string& filename) {
    cv::Mat matImage(height, width, CV_8UC1);

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            matImage.at<uchar>(i, j) = image[i * width + j];
        }
    }

    cv::imwrite(filename, matImage);
}

/**
* We're going to examine rows from the middle outward, searching alternately above and below the
* middle, and farther out each time. rowStep is the number of rows between each successive
* attempt above and below the middle. So we'd scan row middle, then middle - rowStep, then
* middle + rowStep, then middle - (2 * rowStep), etc.
* rowStep is bigger as the image is taller, but is always at least 1. We've somewhat arbitrarily
* decided that moving up and down by about 1/16 of the image is pretty good; we try more of the
* image if "trying harder".
*/
std::set<std::string> DoDecode(const std::vector<uint8_t>& BinarizedImage, int width, int height)
{

	//SaveImageAsJPG(BinarizedImage, height, width, "debug_image.jpg");

	std::set<std::string> Result;

	PartialResult res;
	//int fixed_max_top_lines = 100;

	int middle = height / 2;
	int rowStep = 1;
	int maxLines = height;

	std::vector<int> checkRows;

	std::vector<uint16_t> bars;
	bars.reserve(128); // e.g. EAN-13 has 59 bars/spaces

	for (int i = 0; i < maxLines; i++) {

		// Scanning from the middle out. Determine which row we're looking at next:
		int rowStepsAboveOrBelow = (i + 1) / 2;
		bool isAbove = (i & 0x01) == 0; // i.e. is x even?
		int rowNumber = middle + rowStep * (isAbove ? rowStepsAboveOrBelow : -rowStepsAboveOrBelow);
		bool isCheckRow = false;
		if (rowNumber < 0 || rowNumber >= height) {
			// Oops, if we run off the top or bottom, stop
			break;
		}

		// See if we have additional check rows (see below) to process
		if (checkRows.size()) {
			--i;
			rowNumber = checkRows.back();
			checkRows.pop_back();
			isCheckRow = true;
			if (rowNumber < 0 || rowNumber >= height)
				continue;
		}

		if (!getPatternRow(BinarizedImage, height, width, rowNumber, bars))
			continue;

		//false, true
		for (bool upsideDown : {false}) {
			// trying again?
			if (upsideDown) {
				// reverse the row and continue
				std::reverse(bars.begin(), bars.end());
			}
			// Look for a barcode
			PatternView next(bars);
			do {
				bool result = decodePattern(next, res);

				size_t last_delim = 0;
				size_t next_delim = 0;
				std::string delimiter = " ";

				while ((next_delim = res.txt.find(delimiter, last_delim)) != std::string::npos) {
					if (res.txt.substr(last_delim, next_delim - last_delim).size() == 13) {
						Result.insert(res.txt);
					}
					last_delim = next_delim + 1;
				}

				if (res.txt.substr(last_delim).size() == 13) {
					Result.insert(res.txt);
				}

				// make sure we make progress and we start the next try on a bar
				next.shift(2 - (next.index() % 2));
				next.extend();

			} while (next.size());
		}
	}

	return Result;

}

// Function to decode the barcode with rotation invariance
std::string DecodeWithRotation(const cv::Mat buffer_image) {

	std::vector<uint8_t> image;
	int height = buffer_image.rows;
	int width = buffer_image.cols;

	// Convert the image to grayscale
	cv::Mat gray_image;
	cv::cvtColor(buffer_image, gray_image, cv::COLOR_BGR2GRAY);

	//Convert grayscale image to binary image using Otus thresholding
	cv::Mat binary_image;
	cv::threshold(gray_image, binary_image, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

	//save the binary image for debugging
	//cv::imwrite("binary_image.jpg", binary_image);
	// Flatten the binary_image image into a vector
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			image.push_back(binary_image.at<uchar>(i, j));
		}
	}

	std::set<std::string> results = DoDecode(image, width, height);

	//Save the image as jpg for debugging

	if (results.empty()) {
		std::vector<uint8_t> rotatedImage = image;
		int newHeight = height;
		int newWidth = width;
		for (int angle = 90; angle < 360; angle += 90) {
			float radians = angle * M_PI / 180.0f;
			rotatedImage = RotateImage(image, width, height, radians);
			newHeight = static_cast<int>(std::abs(height * std::cos(radians)) + std::abs(width * std::sin(radians)));
			newWidth = static_cast<int>(std::abs(width * std::cos(radians)) + std::abs(height * std::sin(radians)));
			results = DoDecode(rotatedImage, newWidth, newHeight);
			if (!results.empty()) {
				std::cout << "Barcode found at " << angle << " degrees rotation." << std::endl;
				break;
			}
		}
	}
	else {
		std::cout << "Barcode found at 0 degrees rotation." << std::endl;
	}

	return GetMostFrequentResult(results);
}
