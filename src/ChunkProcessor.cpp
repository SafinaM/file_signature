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

void ChunkProcessor::extractReadyHashes() {
	auto it = std::find_if(m_futureHashList.begin(), m_futureHashList.end(), [](const auto& it) {
		return it.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready; }
	);
	if (it != m_futureHashList.end()) {
		if (it->valid()) {
			m_prioritizedHashes.push(it->get());
			m_futureHashList.erase(it);
		}
	}
}

std::shared_ptr<char[]> ChunkProcessor::readDataOfChunkSize() {
	return m_fileReader->read(getCurrentChunkSize());
}

// collecting hashes from threadpool here
void ChunkProcessor::checkResults() {
	try {
		while (m_currentWritten.load() < m_maxId.load()) {
			std::lock_guard<std::mutex> lk(m_mutex);
			extractReadyHashes();
			m_conditionalVariable.notify_all();
		}
	} catch (const std::exception& e) {
		std::cerr << "One of the threads in thread pool throws an exception, id: "
				  << std::this_thread::get_id() << e.what()
				  << std::endl;
		exit(-1);
	} catch(...) {
		std::cerr << "produceDataThread: Unknown failure occurred. Possible memory corruption!" << std::endl;
		exit(-1);
	}
}

void ChunkProcessor::produceData() {
	try {
		while(m_currentRead.load() < m_maxId.load()) {
			const auto data = readDataOfChunkSize();
			if (!data) {
				std::cerr << "Invalid data, please check FileReader" << std::endl;
				throw std::runtime_error(
					"\nChunkProcessor::produceData: nullptr was returned! "
					"Please check the size of a file and chunkSize!");
			}
			auto future = m_threadPool.submit(
				Hasher::jenkinsOneAtATimeHash,
					data,
					getCurrentChunkSize(),
					m_currentRead.load());

					m_currentRead.fetch_add(1);
			{
				std::lock_guard lockGuard(m_mutex); // I could not catch UB here without this mutex...
				m_futureHashList.emplace_back(std::move(future));
			}

			m_conditionalVariable.notify_all();
		}
	} catch(const std::exception& exception) {
		std::cerr << "ChunkProcessor::produceData: throws an exception, id: "
				  << std::this_thread::get_id()
				  << exception.what() << std::endl;
		exit(-1);
	} catch(...) {
		std::cerr << "ChunkProcessor::produceData: Unknown failure occurred. Possible memory corruption!" << std::endl;
		exit(-1);
	}
}

void ChunkProcessor::consumeData() {
	try {
		while (m_currentWritten.load() < m_maxId.load()) {
			std::unique_lock lk(m_mutex);
			m_conditionalVariable.wait(lk,[this] {return !m_prioritizedHashes.empty();});
			Data data = m_prioritizedHashes.top();
			// additional checking to keep an order, theoretically it is possible, especially in a case of big chunkSize ~ 100 MB
			// note that id of a data chunk should be equal to number of already written chunks
			// as a variant to use seekp, but now let's keep your hard disk alive
			if (data.first != m_currentWritten) {
				std::cout << "we are waiting element number " << m_currentWritten << std::endl;
				std::this_thread::yield;
				continue;
			}
			m_prioritizedHashes.pop();
			lk.unlock();
			writeHash(data);
			m_currentWritten.fetch_add(1);
		}
	}  catch(const std::exception& exception) {
		std::cerr << "consumeData throws an exception, id: "
				  << std::this_thread::get_id()
				  << exception.what() << std::endl;
		exit(-1);
	} catch(...) {
		std::cerr << "consumeData: Unknown failure occurred. Possible memory corruption!" << std::endl;
		exit(-1);
	}
}

void ChunkProcessor::writeHash(const Data& data) {
	m_fileWriter->write(data.second);
	std::cout << "id = " << data.first << ", hash = " << data.second << std::endl;
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
		{}

void ChunkProcessor::run() {
	m_producingDataFuture = std::async(std::launch::async, &ChunkProcessor::produceData, this);
	m_consumingDataFuture = std::async(std::launch::async, &ChunkProcessor::consumeData, this);
	m_checkingResultsFuture = std::async(std::launch::async, &ChunkProcessor::checkResults, this);
}

void ChunkProcessor::tryToStop() {

	m_producingDataFuture.get();
	m_consumingDataFuture.get();
	m_checkingResultsFuture.get();
}

ChunkProcessor::~ChunkProcessor() {
	tryToStop();
}
