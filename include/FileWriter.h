#include <fstream>

#pragma once

// not thread safe
struct FileWriter{

	FileWriter(std::string_view m_outputFilePath);
	~FileWriter() = default;

	void write(uint64_t hash);

private:

	std::ofstream m_ofstream;
	std::string m_outputFilePath;

};