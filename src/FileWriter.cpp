#include <FileWriter.h>

#include <iostream>


FileWriter::FileWriter(const char* outputFilePath) :
	m_outputFilePath(outputFilePath) {

	m_ofstream.open(m_outputFilePath, std::ios::out | std::ios::trunc);
	if (!m_ofstream) {
		std::cerr << "file: " << m_outputFilePath << " not found" << std::endl;
		m_validated = false;
		return;
	}
	m_validated = true;
}

bool FileWriter::isValid() {
	return m_validated;
}

void FileWriter::write(uint64_t hash) {
	if (!m_ofstream) {
		return;
	}
	m_ofstream << hash << std::endl; // human friendly view
}

FileWriter::~FileWriter() {
	m_ofstream.close();
}

