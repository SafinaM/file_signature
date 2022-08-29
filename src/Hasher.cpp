#include <Hasher.h>
#include <memory>

std::pair<uint64_t, uint64_t> Hasher::jenkinsOneAtATimeHash(std::shared_ptr<char[]> key, std::size_t length, uint64_t id) {

	uint64_t hash = 0;
	for (uint64_t i = 0; i < length; ++i) {
		hash += key[i];
		hash += hash << 10;
		hash ^= hash >> 6;
	}
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;

	return std::make_pair(id, hash);
}