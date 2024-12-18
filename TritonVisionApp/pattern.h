#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

#ifdef max
#undef max
#endif


#ifdef min
#undef min
#endif

template <class T, class U>
constexpr T narrow_cast(U&& u) noexcept {
	return static_cast<T>(std::forward<U>(u));
}

template <typename Iterator>
struct Range
{
	Iterator _begin, _end;

	Range(Iterator b, Iterator e) : _begin(b), _end(e) {}

	template <typename C>
	Range(const C& c) : _begin(std::begin(c)), _end(std::end(c)) {}

	Iterator begin() const noexcept { return _begin; }
	Iterator end() const noexcept { return _end; }
	explicit operator bool() const { return begin() < end(); }
	int size() const { return narrow_cast<int>(end() - begin()); }
};

// see C++20 ssize
template <class Container>
constexpr auto Size(const Container& c) -> decltype(c.size(), int()) {
	return narrow_cast<int>(c.size());
}

template <class T, std::size_t N>
constexpr int Size(const T (&)[N]) noexcept {
	return narrow_cast<int>(N);
}


class PatternView
{
	using Iterator = std::vector<uint16_t>::const_pointer;
	Iterator _data = nullptr;
	int _size = 0;
	Iterator _base = nullptr;
	Iterator _end = nullptr;

public:
	using value_type = std::vector<uint16_t>::value_type;

	PatternView() = default;

	// A PatternRow always starts with the width of whitespace in front of the first black bar.
	// The first element of the PatternView is the first bar.
	PatternView(const std::vector<uint16_t>& bars)
		: _data(bars.data() + 1), _size(Size(bars) - 1), _base(bars.data()), _end(bars.data() + bars.size())
	{}

	PatternView(Iterator data, int size, Iterator base, Iterator end) : _data(data), _size(size), _base(base), _end(end) {}

	template <size_t N>
	PatternView(const std::array<uint16_t, N>& row) : _data(row.data()), _size(N)
	{}

	Iterator data() const { return _data; }
	Iterator begin() const { return _data; }
	Iterator end() const { return _data + _size; }

	value_type operator[](int i) const
	{
//		assert(i < _count);
		return _data[i];
	}

	int sum(int n = 0) const { return std::accumulate(_data, _data + (n == 0 ? _size : n), 0); }
	int size() const { return _size; }

	// index is the number of bars and spaces from the first bar to the current position
	int index() const { return narrow_cast<int>(_data - _base) - 1; }
	int pixelsInFront() const { return std::accumulate(_base, _data, 0); }
	int pixelsTillEnd() const { return std::accumulate(_base, _data + _size, 0) - 1; }
	bool isAtFirstBar() const { return _data == _base + 1; }
	bool isAtLastBar() const { return _data + _size == _end - 1; }
	bool isValid(int n) const { return _data && _data >= _base && _data + n <= _end; }
	bool isValid() const { return isValid(size()); }

	template<bool acceptIfAtFirstBar = false>
	bool hasQuietZoneBefore(float scale) const
	{
		return (acceptIfAtFirstBar && isAtFirstBar()) || _data[-1] >= sum() * scale;
	}

	template<bool acceptIfAtLastBar = true>
	bool hasQuietZoneAfter(float scale) const
	{
		return (acceptIfAtLastBar && isAtLastBar()) || _data[_size] >= sum() * scale;
	}

	PatternView subView(int offset, int size = 0) const
	{
//		if(std::abs(size) > count())
//			printf("%d > %d\n", std::abs(size), _count);
//		assert(std::abs(size) <= count());
		if (size == 0)
			size = _size - offset;
		else if (size < 0)
			size = _size - offset + size;
		return {begin() + offset, std::max(size, 0), _base, _end};
	}

	bool shift(int n)
	{
		return _data && ((_data += n) + _size <= _end);
	}

	bool skipPair()
	{
		return shift(2);
	}

	bool skipSymbol()
	{
		return shift(_size);
	}

	bool skipSingle(int maxWidth)
	{
		return shift(1) && _data[-1] <= maxWidth;
	}

	void extend()
	{
		_size = std::max(0, narrow_cast<int>(_end - _data));
	}
};

/**
 * @brief The BarAndSpace struct is a simple 2 element data structure to hold information about bar(s) and space(s).
 *
 * The operator[](int) can be used in combination with a PatternView
 */
template <typename T>
struct BarAndSpace
{
	using value_type = T;
	T bar = {}, space = {};
	// even index -> bar, odd index -> space
	constexpr T& operator[](int i) noexcept { return reinterpret_cast<T*>(this)[i & 1]; }
	constexpr T operator[](int i) const noexcept { return reinterpret_cast<const T*>(this)[i & 1]; }
	bool isValid() const { return bar != T{} && space != T{}; }
};

using BarAndSpaceI = BarAndSpace<uint16_t>;

template <int LEN, typename T, typename RT = T>
constexpr auto BarAndSpaceSum(const T* view) noexcept
{
	BarAndSpace<RT> res;
	for (int i = 0; i < LEN; ++i)
		res[i] += view[i];
	return res;
}

/**
 * @brief FixedPattern describes a compile-time constant (start/stop) pattern.
 *
 * @param N  number of bars/spaces
 * @param SUM  sum over all N elements (size of pattern in modules)
 * @param IS_SPARCE  whether or not the pattern contains '0's denoting 'wide' bars/spaces
 */
template <int N, int SUM, bool IS_SPARCE = false>
struct FixedPattern
{
	using value_type = std::vector<uint16_t>::value_type;
	value_type _data[N];
	constexpr value_type operator[](int i) const noexcept { return _data[i]; }
	constexpr const value_type* data() const noexcept { return _data; }
	constexpr int size() const noexcept { return N; }
	constexpr BarAndSpace<value_type> sums() const noexcept { return BarAndSpaceSum<N>(_data); }
};

template <int N, int SUM>
using FixedSparcePattern = FixedPattern<N, SUM, true>;

template <bool E2E = false, int LEN, int SUM>
float IsPattern(const PatternView& view, const FixedPattern<LEN, SUM, false>& pattern, int spaceInPixel = 0,
				float minQuietZone = 0, float moduleSizeRef = 0)
{
	if (E2E) {
		using float_t = double;

		auto widths = BarAndSpaceSum<LEN, PatternView::value_type, float_t>(view.data());
		auto sums = pattern.sums();
		BarAndSpace<float_t> modSize = {widths[0] / sums[0], widths[1] / sums[1]};

		auto m = std::min(modSize[0], modSize[1]);
		auto M = std::max(modSize[0], modSize[1]);

		if (M > 4 * m) // make sure module sizes of bars and spaces are not too far away from each other
			return 0;

		if (minQuietZone && spaceInPixel < minQuietZone * modSize.space)
			return 0;

		const BarAndSpace<float_t> thr = {modSize[0] * .75 + .5, modSize[1] / (2 + (LEN < 6)) + .5};

		for (int x = 0; x < LEN; ++x)
			if (std::abs(view[x] - pattern[x] * modSize[x]) > thr[x])
				return 0;

		const float_t moduleSize = (modSize[0] + modSize[1]) / 2;
		return moduleSize;
	}

	int width = view.sum(LEN);
	if (SUM > LEN && width < SUM)
		return 0;

	const float moduleSize = (float)width / SUM;

	if (minQuietZone && spaceInPixel < minQuietZone * moduleSize - 1)
		return 0;

	if (!moduleSizeRef)
		moduleSizeRef = moduleSize;

	// the offset of 0.5 is to make the code less sensitive to quantization errors for small (near 1) module sizes.
	// TODO: review once we have upsampling in the binarizer in place.
	const float threshold = moduleSizeRef * (0.5f + E2E * 0.25f) + 0.5f;

	for (int x = 0; x < LEN; ++x)
		if (std::abs(view[x] - pattern[x] * moduleSizeRef) > threshold)
			return 0;

	return moduleSize;
}

template <bool RELAXED_THRESHOLD = false, int N, int SUM>
float IsPattern(const PatternView& view, const FixedPattern<N, SUM, true>& pattern, int spaceInPixel = 0,
				float minQuietZone = 0, float moduleSizeRef = 0)
{
	// pattern contains the indices with the bars/spaces that need to be equally wide
	int width = 0;
	for (int x = 0; x < SUM; ++x)
		width += view[pattern[x]];

	const float moduleSize = (float)width / SUM;

	if (minQuietZone && spaceInPixel < minQuietZone * moduleSize - 1)
		return 0;

	if (!moduleSizeRef)
		moduleSizeRef = moduleSize;

	// the offset of 0.5 is to make the code less sensitive to quantization errors for small (near 1) module sizes.
	// TODO: review once we have upsampling in the binarizer in place.
	const float threshold = moduleSizeRef * (0.5f + RELAXED_THRESHOLD * 0.25f) + 0.5f;

	for (int x = 0; x < SUM; ++x)
		if (std::abs(view[pattern[x]] - moduleSizeRef) > threshold)
			return 0;

	return moduleSize;
}

template <int N, int SUM, bool IS_SPARCE>
bool IsRightGuard(const PatternView& view, const FixedPattern<N, SUM, IS_SPARCE>& pattern, float minQuietZone,
				  float moduleSizeRef = 0.f)
{
	int spaceInPixel = view.isAtLastBar() ? std::numeric_limits<int>::max() : *view.end();
	return IsPattern(view, pattern, spaceInPixel, minQuietZone, moduleSizeRef) != 0;
}

template<int LEN, typename Pred>
PatternView FindLeftGuard(const PatternView& view, int minSize, Pred isGuard)
{
	if (view.size() < minSize)
		return {};

	auto window = view.subView(0, LEN);
	if (window.isAtFirstBar() && isGuard(window, std::numeric_limits<int>::max()))
		return window;
	for (auto end = view.end() - minSize; window.data() < end; window.skipPair())
		if (isGuard(window, window[-1]))
			return window;

	return {};
}

template <int LEN, int SUM, bool IS_SPARCE>
PatternView FindLeftGuard(const PatternView& view, int minSize, const FixedPattern<LEN, SUM, IS_SPARCE>& pattern,
						  float minQuietZone)
{
	return FindLeftGuard<LEN>(view, std::max(minSize, LEN),
							  [&pattern, minQuietZone](const PatternView& window, int spaceInPixel) {
								  return IsPattern(window, pattern, spaceInPixel, minQuietZone);
							  });
}

