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
				std::cerr << "Invalid data, please check FileReader" << std::endl;
				throw std::runtime_error(
					"\nChunkProcessor::produceData: nullptr was returned! "
					"Please check the size of a file and chunkSize!");
			}

			auto future = m_threadPool.submit(Hasher::jenkinsOneAtATimeHash, data, getCurrentChunkSize(), m_currentRead.load());
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
		done = true;
		m_conditionalVariable.notify_all();
		throw;

	} catch(...) {
		std::cerr << "ChunkProcessor::produceData: Unknown failure occurred. Possible memory corruption!" << std::endl;
		done = true;
		m_conditionalVariable.notify_all();
		throw;
	}
}

void ChunkProcessor::consumeData() {
	try {
		while (m_currentWritten.load() < m_maxId.load() || done) {

			std::unique_lock lk(m_mutex);
			m_conditionalVariable.wait(lk, [this] {return !m_futureHashList.empty() || !m_prioritizedHashes.empty() || done;});

			if (done)
				return;

//			auto it = std::partition(m_futureHashList.begin(), m_futureHashList.end(), [](const auto &it) {
//				return it.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready;
//			});

			auto it = std::find_if(m_futureHashList.begin(), m_futureHashList.end(), [](const auto &it) {
				return it.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready;
			});

//			for (; it != m_futureHashList.end();) {
//				m_prioritizedHashes.push(it->get());
//				it = m_futureHashList.erase(it);
//			}
//			lk.unlock();


			if (it != m_futureHashList.end()){
				m_prioritizedHashes.push(it->get());
				m_futureHashList.erase(it);
			}

			// additional checking to keep an order, theoretically it is possible, especially in a case of big chunkSize ~ 100 MB
			// note that id of a data chunk should be equal to number of already written chunks
			// as a variant to use seekp, but now let's keep your hard disk alive
			if (m_prioritizedHashes.empty() || m_prioritizedHashes.top().first != m_currentWritten) {
				std::this_thread::yield;
				continue;
			}
			Data data = m_prioritizedHashes.top();
			m_prioritizedHashes.pop();
			writeHash(data);

			m_currentWritten.fetch_add(1);
		}
	}  catch(const std::exception& exception) {
		std::cerr << "consumeData throws an exception, id: "
				  << std::this_thread::get_id()
				  << exception.what() << std::endl;
		done = true;
		m_conditionalVariable.notify_all();
		throw;
	} catch(...) {
		std::cerr << "consumeData: Unknown failure occurred. Possible memory corruption!" << std::endl;
		done = true;
		m_conditionalVariable.notify_all();
		throw;
	}
}

void ChunkProcessor::writeHash(const Data& data) {
	m_fileWriter->write(data.second);
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
		{
//			m_futureHashList.reserve(m_maxId);
		}

void ChunkProcessor::run() {
	m_producingDataFuture = std::async(std::launch::async, &ChunkProcessor::produceData, this);
	m_consumingDataFuture = std::async(std::launch::async, &ChunkProcessor::consumeData, this);
	tryToStop();
}

void ChunkProcessor::tryToStop() {

	m_producingDataFuture.get();
	m_consumingDataFuture.get();
}

ChunkProcessor::~ChunkProcessor() {
	done = true;
}
