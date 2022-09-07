#include <ArgParser.h>
#include <FileReader.h>
#include <FileWriter.h>
#include <ThreadPool.h>
#include <ThreadSafeQueue.h>

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
	priority_queue m_prioritizedHashes;

	threadsafe_queue<Data> m_hashesInThreadSafeQueue;

	ThreadPool m_threadPool;

	std::unique_ptr<FileReader> m_fileReader;
	std::unique_ptr<FileWriter> m_fileWriter;

	std::future<void> m_producingDataFuture;
	std::thread m_consumingThread;

	std::atomic<uint64_t> m_currentRead{0};
	std::atomic<uint64_t> m_currentWritten{0};
	std::atomic<uint64_t> m_maxId;
	std::atomic<bool> done{false};
	uint64_t m_chunkSize;
	uint64_t m_fileSize;

	void extractReadyHashes();

	std::shared_ptr<char[]> readDataOfChunkSize();

	void produceData();

	void consumeData();

	void writeHash(const Data& data);

};