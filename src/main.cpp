#include <ArgParser.h>
#include <FileReader.h>
#include <FileWriter.h>
#include <ChunkProcessor.h>

#include <iostream>

int main(int argc, char** argv) {

	try {
		ArgParser argParser;
		// here we can validate input parameters
		if (!argParser.parse(argc, argv))
			return -1;

		uint32_t maxNumberOfElements = argParser.getInputFileSize() / argParser.getChunkSize();
		uint64_t lastChunkSize = argParser.getInputFileSize() % argParser.getChunkSize();

		if (lastChunkSize)
			++maxNumberOfElements;

		std::unique_ptr<FileReader> fileReader = std::unique_ptr<FileReader>(
			new FileReader(argParser.getInputFilePath()));

		if (!fileReader) {
			return -1;
		}

		std::unique_ptr<FileWriter> fileWriter = std::unique_ptr<FileWriter>(new FileWriter(argParser.getOutputPath()));

		if (!fileWriter) {
			return -1;
		}

		ChunkProcessor produceConsumer = ChunkProcessor(
			maxNumberOfElements,
			std::move(fileReader),
			std::move(fileWriter),
			argParser.getInputFileSize(),
			argParser.getChunkSize());

		produceConsumer.run();

	} catch (const std::exception& e) {
		std::cerr << "Exception caught: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Unknown failure occurred. Possible memory corruption!" << std::endl;
	}

	return 0;
}