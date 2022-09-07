#include <ArgParser.h>
#include <Data.h>
#include <FileReader.h>
#include <FileWriter.h>
#include <ThreadPool.h>
#include <ThreadSafeQueue.h>

#ifdef WITH_BOOST
	#include <boost/thread/thread.hpp>
	#include <boost/lockfree/queue.hpp>
#endif

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

using priority_queue = std::priority_queue<
		Data,
		std::deque<Data>, Greater>;

private:

	uint64_t getCurrentChunkSize();

	std::mutex m_mutex;
	priority_queue m_prioritizedHashes;
#ifndef WITH_BOOST
	ThreadSafeQueue<Data> m_hashesInThreadSafeQueue;
#else
	boost::lockfree::queue<Data, boost::lockfree::fixed_sized<false>> m_hashesInThreadSafeQueue;
#endif

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