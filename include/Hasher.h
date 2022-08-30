#include <cstdint>
#include <utility>
#include <memory>

#pragma once

struct Hasher {
	// first - id, second - hash
	static std::pair<uint64_t, uint64_t> jenkinsOneAtATimeHash(std::shared_ptr<char[]> key, uint64_t length, uint64_t id);

};


