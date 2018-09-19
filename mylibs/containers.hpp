#pragma once

#include <vector>
#include <algorithm>

template <typename T> bool contains (std::vector<T> const& c, T const& val) {
	return std::find(c.begin(), c.end(), val) != c.end();
}
template <typename T> bool any_contains (std::vector<T> const& as, std::vector<T> const& bs) {
	for (auto& a : as) {
		if (contains(bs, a))
			return true;
	}
	return false;
}
