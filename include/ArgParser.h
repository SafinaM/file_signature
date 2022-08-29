#include <cstdint>
#include <string_view>

#pragma once

struct ArgParser {

	ArgParser() = default;

	std::string_view getInputFilePath();

	std::string_view getOutputPath();

	uint64_t getChunkSize();

	uint64_t getInputFileSize();

	bool parse(int arc, char** argv);

private:
	std::string_view m_inputFilePath;
	std::string_view m_outputFilePath;
	uint64_t m_chunkSize;
	uint64_t m_fileSize;

};
