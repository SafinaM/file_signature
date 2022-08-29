#include <FileWriter.h>

#include <iostream>


FileWriter::FileWriter(std::string_view outputFilePath) :
	m_outputFilePath(outputFilePath) {

	m_ofstream.open(m_outputFilePath, std::ios::out | std::ios::trunc);
	if (!m_ofstream) {
		std::cerr << "file: " << m_outputFilePath << " not found" << std::endl;
		throw std::runtime_error("Fstream was not opened!");

	}
}

void FileWriter::write(uint64_t hash) {
	if (!m_ofstream) {
		std::cerr << "FileWriter: Something was wrong!" << std::endl;
		return;
	}
	m_ofstream << hash << std::endl; // human friendly view
}
