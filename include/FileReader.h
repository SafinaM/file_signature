#include <fstream>
#include <memory>

#pragma once

// not thread safe
struct FileReader {

	FileReader(std::string_view inputFilePath, uint64_t fileSize);

	~FileReader() = default;

	std::shared_ptr<char[]> read(uint64_t chunkSize);

	bool isValid();

private:
	uint64_t m_fileSize;
	std::ifstream m_ifstream;
	std::string m_inputFilePath;

};
