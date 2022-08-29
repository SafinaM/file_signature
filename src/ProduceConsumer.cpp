#include <ProduceConsumer.h>
#include <Hasher.h>

#include <iostream>

inline uint64_t ProduceConsumer::getCurrentChunkSize() {
	if ((m_currentRead + 1) * m_chunkSize < m_fileSize)
		return m_chunkSize;
	else
		return m_fileSize % m_chunkSize;
}

void ProduceConsumer::extractReadyHashes() {
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

std::shared_ptr<char[]> ProduceConsumer::readDataOfChunkSize() {
	return m_fileReader->read(getCurrentChunkSize());
}

// collecting hashes from threadpool here
void ProduceConsumer::checkResults() {
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

void ProduceConsumer::produceData() {
	try {
		while(m_currentRead.load() < m_maxId.load()) {
			const auto data = readDataOfChunkSize();
			if (!data) {
				std::cerr << "Invalid data, please check FileReader" << std::endl;
				throw std::runtime_error("\nProduceConsumer::produceData: nullptr was returned! Please check the size of a file and chunkSize!");
			}
			{
				std::lock_guard<std::mutex> lk(m_mutex);
				auto future = m_threadPool.submit(
					Hasher::jenkinsOneAtATimeHash,
					data,
					getCurrentChunkSize(),
					m_currentRead.load());
				m_currentRead.fetch_add(1);
				m_futureHashList.emplace_back(std::move(future));
			}

			m_conditionalVariable.notify_all();
		}
	} catch(const std::exception& exception) {
		std::cerr << "ProduceConsumer::produceData: throws an exception, id: "
				  << std::this_thread::get_id()
				  << exception.what() << std::endl;
		exit(-1);
	} catch(...) {
		std::cerr << "ProduceConsumer::produceData: Unknown failure occurred. Possible memory corruption!" << std::endl;
		exit(-1);
	}

}

void ProduceConsumer::consumeData() {
	try {
		while (m_currentWritten.load() < m_maxId.load()) {
			std::unique_lock lk(m_mutex);
			m_conditionalVariable.wait(lk,[this] {return !m_prioritizedHashes.empty();});
			Data data = m_prioritizedHashes.top();
			// additional checking to keep an order, theoratically it is possible,
			// note that id of a data chunk should be equal to number of already written chunks
			if (data.first != m_currentWritten) {
				std::cout << "was skipped, wait " << data.first << std::endl;
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

void ProduceConsumer::writeHash(const Data& data) {
	m_fileWriter->write(data.second);
	std::cout << "hash = " << data.first << "  " << data.second << std::endl;
	std::cout << "m_currentRead = " << m_currentRead << std::endl;
	std::cout << "m_currentWritten = " << m_currentWritten << std::endl;
}

ProduceConsumer::ProduceConsumer(
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

void ProduceConsumer::run() {
	m_producingDataFuture = std::async(std::launch::async, &ProduceConsumer::produceData, this);
	m_consumingDataFuture = std::async(std::launch::async, &ProduceConsumer::consumeData, this);
	m_checkingResultsFuture = std::async(std::launch::async, &ProduceConsumer::checkResults, this);
}

// yes a little bit ugly,
void ProduceConsumer::tryToStop() {

	if (m_producingDataFuture.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready) {
		m_producingDataFuture.get();
	}
	if (m_consumingDataFuture.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready) {
		m_consumingDataFuture.get();
	}
	if (m_checkingResultsFuture.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready) {
		m_checkingResultsFuture.get();
	}
}

ProduceConsumer::~ProduceConsumer() {
	tryToStop();
}
