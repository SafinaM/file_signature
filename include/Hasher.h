#include <Data.h>

#include <utility>
#include <memory>
#include <cstdint>

#pragma once

struct Hasher {
	// first - id, second - hash
	static Data jenkinsOneAtATimeHash(std::shared_ptr<char[]> key, uint64_t length, uint64_t id);

};


