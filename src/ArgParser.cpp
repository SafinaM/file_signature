#include "../include/ArgParser.h"

#include <iostream>
#include <filesystem>

const char* ArgParser::getInputFilePath() {
	return m_inputFilePath;
}

const char* ArgParser::getOutputPath() {
	return m_outputFilePath;
}

uint64_t ArgParser::getChunkSize() {
	return m_chunkSize;
}

uint64_t ArgParser::getInputFileSize() {
	return m_fileSize;
}

bool ArgParser::parse(int argc, char** argv)  {

	if (argc != 4)
		return false;

	m_inputFilePath = argv[1];
	m_outputFilePath = argv[2];

	// check size a file to set correct chunksize
	m_fileSize = std::filesystem::file_size(m_inputFilePath);
	if (!m_fileSize) {
		std::cout << "file " << m_inputFilePath << " is not found!" << std::endl;
		return false;
	}

	m_chunkSize = std::stoll(argv[3]);
	m_chunkSize *= 1024l * 1024l;

	if (!m_chunkSize) {
		m_chunkSize = 1024l * 1024l; // 1MB
		std::cerr << "chunkSize was taken as " << m_chunkSize << std::endl;
	}

	if (m_chunkSize > m_fileSize) {
		m_chunkSize = m_fileSize;
		std::cerr << "chunkSize was taken as " << m_chunkSize << std::endl;
	}

	return true;
}

