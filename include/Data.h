#include <cstdint>

#pragma once

struct Data {
	uint64_t id;
	uint64_t hash;
	Data() = default;

};

struct Greater {
	bool operator() (const Data& x, const Data& y) const { return x.id > y.id; }
};
