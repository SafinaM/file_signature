#include <ArgParser.h>
#include <ChunkProcessor.h>
#include <FileReader.h>
#include <FileWriter.h>
#include <Hasher.h>
#include <ThreadPool.h>
#include <iostream>

#include <memory>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <future>
#include <thread>
#include <utility>
#include <filesystem>

using Data = std::pair<uint64_t, uint64_t>;

uint32_t testFunc(unsigned int delay) {
	std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	return 2;
}
int main(int argc, char** argv) {

	std::cout << "FileReader test..." << std::endl;
	try {
		FileReader fileReader("test.bin", 100);

	} catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		assert(strcmp(e.what(), "FileReader::FileReader: Fstream was not opened!") == 0);
	}

	std::cout << "FileWriter test..." << std::endl;

	try {
		FileWriter fileWriter("test.bin");
	} catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		assert(strcmp(e.what(), "FileWriter::FileWriter: Fstream was not opened!") == 0);
	}

	std::cout << "ThreadPool test..." << std::endl;

	uint64_t bufferSize = 20;
	std::shared_ptr<char[]> ptr1(new char[bufferSize]);
	std::memset(ptr1.get(), 0, bufferSize);
	std::shared_ptr<char[]> ptr2(new char[bufferSize]);
	std::memset(ptr2.get(), 1, bufferSize);
	ThreadPool threadPool;


	auto future1 = threadPool.submit(testFunc, 20);
	std::future<Data> future2 = threadPool.submit(Hasher::jenkinsOneAtATimeHash, ptr1, bufferSize, 1);
	std::future<Data> future3 = threadPool.submit(Hasher::jenkinsOneAtATimeHash, ptr2, bufferSize, 2);
	auto result1 = future1.get();
	auto result2 = future2.get();
	auto result3 = future3.get();
	assert(result1 == 2);
	assert(result2.first == 1);
	assert(result3.first == 2);
	assert(result2.second == 0);
	assert(result3.second == 3455262669121713132); // calculate hash before

	std::cout << "ChunkProcessor test..." << std::endl;

try {
	const uint64_t oneMegabyte = 1024 * 1024; // 1MB

	std::array<uint64_t, 3> chunkSize {oneMegabyte, 2 * oneMegabyte, 3 * oneMegabyte};
	for (const auto chunkSize: chunkSize) {
		std::cout << "\nCnunkSize = "<< chunkSize;
		std::string_view filePath = "randomFile"; // src/Test/randomFile - 3MB
		const uint64_t size = std::filesystem::file_size(filePath);
		uint64_t numberOfChunks = size / chunkSize;

		uint64_t lastChunkSize = size % chunkSize;

		if (lastChunkSize)
			++numberOfChunks;

		std::unique_ptr<FileReader> fileReader1 = std::unique_ptr<FileReader>(new FileReader(filePath, size));
		std::unique_ptr<FileWriter> fileWriter1 = std::unique_ptr<FileWriter>(new FileWriter("output.txt"));
		ChunkProcessor produceConsumer = ChunkProcessor(
		numberOfChunks,
		std::move(fileReader1),
		std::move(fileWriter1),
		size,
		chunkSize);
		produceConsumer.run();
		std::cout << std::endl;
	}
} catch(...) {
	std::cerr << "Test file: randomFile - was not found. Please check src/Test/randomFile!" << std::endl;
}
	std::cout << "\nSuccess!" << std::endl;

	return 0;
}

