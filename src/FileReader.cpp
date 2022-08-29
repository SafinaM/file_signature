#include <FileReader.h>

#include <iostream>
#include <memory>


FileReader::FileReader(std::string_view inputFilePath) :
	m_inputFilePath(inputFilePath) {

	m_ifstream.open(m_inputFilePath, std::ios::binary | std::ios::in);
	if (!m_ifstream.is_open()) {
		std::cerr << "file " << m_inputFilePath << " not found" << std::endl;
		throw std::runtime_error("Fstream was not opened!");
	}
}

std::shared_ptr<char[]> FileReader::read(uint64_t chunkSize) {

	std::shared_ptr<char[]> chunkBuffer(new char[chunkSize]);

	m_ifstream.read(chunkBuffer.get(), chunkSize);
	uint64_t count = m_ifstream.gcount();
	if (!count) {
		std::cerr << "FileReader: number of read bytes is zero! Too large chunksize!" << std::endl;
		return nullptr;
	}
	if (!m_ifstream.good()) {
		std::cerr << "FileReader: Something was wrong!" << std::endl;
		return nullptr;
	}
	return chunkBuffer;
}

