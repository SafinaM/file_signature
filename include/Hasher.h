#include <cstdint>
#include <utility>
#include <memory>

#pragma once

struct Hasher {

//	static std::pair<uint64_t, uint32_t> jenkinsOneAtATimeHash(const char* key, std::size_t length, uint64_t id);
	static std::pair<uint64_t, uint64_t> jenkinsOneAtATimeHash(std::shared_ptr<char[]> key, std::size_t length, uint64_t id);

};


