#include <ChunkProcessor.h>
#include <Hasher.h>
#include <iostream>

// yes it could be in FileReader, but I also need file size for Hash calculating, that it why it here
inline uint64_t ChunkProcessor::getCurrentChunkSize() {
	if ((m_currentRead + 1) * m_chunkSize <= m_fileSize)
		return m_chunkSize;
	else
		return m_fileSize % m_chunkSize;
}

std::shared_ptr<char[]> ChunkProcessor::readDataOfChunkSize() {
	return m_fileReader->read(getCurrentChunkSize());
}

void ChunkProcessor::produceData() {

	try {
		while(m_currentRead.load() < m_maxId.load() || done) {
			std::shared_ptr<char[]> data = readDataOfChunkSize();
			if (!data) {
				throw std::runtime_error(
					"\nChunkProcessor::produceData: nullptr was returned! "
					"Please check the size of a file and chunkSize!");
			}
			uint64_t chunkSize = getCurrentChunkSize();
			auto func = [this](std::shared_ptr<char[]> data, uint64_t chunkSize, uint64_t currentRead) {
				auto hash = Hasher::jenkinsOneAtATimeHash(data, chunkSize, currentRead);
				m_hashesInThreadSafeQueue.push(std::move(hash));
			};

			auto future = m_threadPool.submit(func, data, chunkSize, m_currentRead.load());
			m_currentRead.fetch_add(1);
		}
	} catch(const std::exception& exception) {
		done = true;
		throw;

	} catch(...) {
		done = true;
		throw;
	}
}

void ChunkProcessor::consumeData() {

	try {
		while (m_currentWritten.load() < m_maxId.load() && !done) {

			// if nothing to process let's reschedule our threads
			if (m_hashesInThreadSafeQueue.empty() && m_prioritizedHashes.empty()) {
				std::this_thread::yield;
				continue;
			}

			// take all processed hashes from thread safe queue
			while (!m_hashesInThreadSafeQueue.empty()) {

#ifndef WITH_BOOST
				auto dataPtr = m_hashesInThreadSafeQueue.try_pop();
					if (dataPtr != nullptr)
						m_prioritizedHashes.push(*dataPtr);
#else
				Data data;
				if (m_hashesInThreadSafeQueue.pop(data))
					m_prioritizedHashes.push(data);
#endif
			}

			// additional checking to keep an order, theoretically it is possible, especially in a case of big chunkSize ~ 100 MB
			// note that id of a data chunk should be equal to number of already written chunks
			// as a variant to use seekp, but now let's keep your hard disk alive
			if (m_prioritizedHashes.empty() || m_prioritizedHashes.top().id != m_currentWritten) {
				std::this_thread::yield;
				continue;
			}

			Data data = m_prioritizedHashes.top();
			m_prioritizedHashes.pop();
			writeHash(data);

			m_currentWritten.fetch_add(1);
		}
	}  catch(const std::exception& exception) {
		done = true;
		throw;
	} catch(...) {
		done = true;
		throw;
	}
}

void ChunkProcessor::writeHash(const Data& data) {

	m_fileWriter->write(data.hash);
//	 std::cout << "id = " << data.first << ", hash = " << data.second << std::endl;
}

ChunkProcessor::ChunkProcessor(
	uint64_t numberOfChunks,
	std::unique_ptr<FileReader> fileReader,
	std::unique_ptr<FileWriter> fileWriter,
	uint64_t size,
	uint64_t chunkSize) :
		m_maxId(numberOfChunks),
		m_fileReader(std::move(fileReader)),
		m_fileWriter(std::move(fileWriter)),
		m_fileSize(size),
		m_chunkSize(chunkSize)

#ifdef WITH_BOOST
, m_hashesInThreadSafeQueue(32) // better to do some experiments, but for example  - ok
#endif
		{}

void ChunkProcessor::run() {

	m_producingDataFuture = std::async(std::launch::async, &ChunkProcessor::produceData, this);
	m_consumingThread = std::thread(&ChunkProcessor::consumeData, this);
	tryToStop();
}

void ChunkProcessor::tryToStop() {

	m_producingDataFuture.get();
	m_consumingThread.join();
}

ChunkProcessor::~ChunkProcessor() {
	done = true;
}
