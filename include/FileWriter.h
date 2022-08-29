#include <fstream>
#include <mutex>


// not thread safe
struct FileWriter{

	FileWriter(const char* m_outputFilePath);
	~FileWriter();

	void write(uint64_t hash);

	bool isValid();

private:

	std::ofstream m_ofstream;
	const char* m_outputFilePath;
	bool m_validated;

};