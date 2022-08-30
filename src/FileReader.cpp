#include <FileReader.h>

#include <iostream>
#include <memory>


FileReader::FileReader(std::string_view inputFilePath, uint64_t fileSize) :
	m_inputFilePath(inputFilePath), m_fileSize(fileSize) {

	m_ifstream.open(m_inputFilePath, std::ios::binary | std::ios::in);
	if (!m_ifstream.is_open()) {
		std::cerr << "FileReader::FileReader: file " << m_inputFilePath << " not found" << std::endl;
		throw std::runtime_error("FileReader::FileReader: Fstream was not opened!");
	}

	// I do not use it right now, but logically FileReader should know about file size
	if (!m_fileSize) {
		std::cerr << "FileReader::FileReader: File size is zero!" << std::endl;
		throw std::runtime_error("FileReader::FileReader: FileReder was not created.");
	}
}

std::shared_ptr<char[]> FileReader::read(uint64_t chunkSize) {

	std::shared_ptr<char[]> chunkBuffer(new char[chunkSize]);

	m_ifstream.read(chunkBuffer.get(), chunkSize);

	if (!m_ifstream.good()) {
		std::cerr << "FileReader::read: Something was wrong!" << std::endl;
		return nullptr;
	}
	return chunkBuffer;
}

