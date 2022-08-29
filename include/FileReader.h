#include <fstream>
#include <memory>
#include <mutex>

#pragma once

// not thread safe
struct FileReader {

	FileReader(const char*, uint64_t size);

	std::shared_ptr<char[]> read(uint64_t chunkSize);

	bool isValid();

private:
	std::ifstream m_ifstream;
	const char* m_inputFilePath;
	bool m_validated;

};
