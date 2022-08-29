#include <cstdint>

#pragma once

struct ArgParser {

	ArgParser() = default;

	const char* getInputFilePath();

	const char* getOutputPath();

	uint64_t getChunkSize();

	uint64_t getInputFileSize();

	bool parse(int arc, char** argv);

private:
	const char* m_inputFilePath;
	const char* m_outputFilePath;
	uint64_t m_chunkSize;
	uint64_t m_fileSize;

};
