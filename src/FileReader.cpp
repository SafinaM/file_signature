#include <FileReader.h>

#include <iostream>
#include <memory>


FileReader::FileReader(const char* inputFilePath, uint64_t size) :
	m_inputFilePath(inputFilePath) {

	m_ifstream.open(m_inputFilePath, std::ios::binary | std::ios::in);
	if (!m_ifstream.is_open()) {
		std::cerr << "file" << m_inputFilePath << " not found" << std::endl;
		m_validated = false;
		return;
	}
	m_validated = true;
}

bool FileReader::isValid() {

	return m_validated;
}

std::shared_ptr<char[]> FileReader::read(uint64_t chunkSize) {

	std::shared_ptr<char[]> chunkBuffer(new char[chunkSize]);

	m_ifstream.read(chunkBuffer.get(), chunkSize);
	if (!m_ifstream.good()) {
		std::cerr << "FileReader: Something was wrong!" << std::endl;
		return nullptr;
	}
	return chunkBuffer;
}

