#include <ThreadPool.h>
#include <ArgParser.h>
#include <FileReader.h>
#include <FileWriter.h>

#include <list>
#include <memory>

#pragma once

struct ChunkProcessor {

	ChunkProcessor(
		uint64_t numberOfChunks,
		std::unique_ptr<FileReader> fileReader,
		std::unique_ptr<FileWriter> fileWriter,
		uint64_t size,
		uint64_t chunkSize);

	void run();

	void tryToStop();

	~ChunkProcessor();

// first - id, second - hash
using Data = std::pair<uint64_t, uint64_t>;
using priority_queue = std::priority_queue<
		Data,
		std::deque<Data>,
		std::greater<Data>>;

private:

	uint64_t getCurrentChunkSize();

	std::mutex m_mutex;
	std::condition_variable m_conditionalVariable;
	priority_queue m_prioritizedHashes;

	std::list<std::future<Data>> m_futureHashList;

	ThreadPool m_threadPool;

	std::unique_ptr<FileReader> m_fileReader;
	std::unique_ptr<FileWriter> m_fileWriter;

	std::future<void> m_producingDataFuture;
	std::future<void> m_consumingDataFuture;
	std::future<void> m_checkingResultsFuture;

	std::atomic<uint64_t> m_currentRead{0};
	std::atomic<uint64_t> m_currentWritten{0};
	std::atomic<uint64_t> m_maxId;
	uint64_t m_chunkSize;
	uint64_t m_fileSize;

	void extractReadyHashes();

	std::shared_ptr<char[]> readDataOfChunkSize();

	// collecting hashes from threadpool here
	void checkResults();

	void produceData();

	void consumeData();

	void writeHash(const Data& data);

};