#include <ThreadPool.h>
#include <Hasher.h>
#include <ArgParser.h>
#include <FileReader.h>
#include <FileWriter.h>

#include <list>
#include <iostream>
#include <fstream>

using Data = std::pair<uint64_t, uint64_t>;

static std::thread::id  pause(size_t delay) {
	std::this_thread::sleep_for(std::chrono::milliseconds(delay + 1));
	return std::this_thread::get_id();
}

struct ProduceConsumer {

	inline uint64_t getCurrentChunkSize() {
		if ((m_currentRead + 1) * m_chunkSize < m_fileSize)
			return m_chunkSize;
		else
			return m_fileSize % m_chunkSize;
	}

	std::mutex m_mutex;
	std::condition_variable m_conditionalVariable;
	std::priority_queue<
		Data,
		std::deque<Data>,
		std::greater<Data>> m_prioritizedHashes;

	std::list<std::future<Data>> m_futureHashList;

	ThreadPool m_threadPool;

	std::unique_ptr<FileReader> m_fileReader;
	std::unique_ptr<FileWriter> m_fileWriter;

	std::future<void> m_producingDataFuture;
	std::future<void> m_consumingDataFuture;
	std::future<void> m_checkingResultsFuture;

	std::atomic<uint64_t> m_currentRead{0};
	std::atomic<bool> wasBroken;
	std::atomic<uint64_t> m_currentWritten{0};
	std::atomic<uint64_t> m_maxId;
	uint64_t m_chunkSize;
	uint64_t m_fileSize;

	void extractReadyHashes() {
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

	std::shared_ptr<char[]> readDataOfChunkSize() {
		return m_fileReader->read(getCurrentChunkSize());
	}

	// collecting hashes from threadpool here
	void checkResults() {
		try {
			while (m_currentWritten.load() < m_maxId.load()) {
				std::lock_guard<std::mutex> lk(m_mutex);
				extractReadyHashes();
				m_conditionalVariable.notify_all();
			}
		} catch (const std::exception& e) {

			std::cerr << "One of the threads in thread pool throws an exception, id: " << std::this_thread::get_id() << e.what() << std::endl;
			exit(-1);
		} catch(...) {
			std::cerr << "produceDataThread: Unknown failure occurred. Possible memory corruption!" << std::endl;
			exit(-1);
		}
	}

	void produceData() {
		try {
			while(m_currentRead.load() < m_maxId.load()) {
				const auto data = readDataOfChunkSize();
				if (!data) {
					std::cerr << "Invalid data, please check FileReader" << std::endl;
					throw std::runtime_error("\nProduceConsumer::produceData: nullptr was returned! Please check the size of a file and chunkSize!");
				}
				{
					std::lock_guard<std::mutex> lk(m_mutex);
					auto future = m_threadPool.submit(Hasher::jenkinsOneAtATimeHash, data, getCurrentChunkSize(), m_currentRead.load());
					m_currentRead.fetch_add(1);
					m_futureHashList.emplace_back(std::move(future));
				}

				m_conditionalVariable.notify_all();
			}
		} catch(const std::exception& exception) {
			std::cerr << "ProduceConsumer::produceData: throws an exception, id: " << std::this_thread::get_id() << exception.what() << std::endl;
			exit(-1);
		} catch(...) {
			std::cerr << "ProduceConsumer::produceData: Unknown failure occurred. Possible memory corruption!" << std::endl;
			exit(-1);
		}

	}

	void consumeData() {
		try {
			while (m_currentWritten.load() < m_maxId.load()) {
				std::unique_lock lk(m_mutex);
				m_conditionalVariable.wait(lk,[this] {return !m_prioritizedHashes.empty();});
				Data data = m_prioritizedHashes.top();
				// additional checking to keep an order
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
			std::cerr << "consumeData throws an exception, id: " << std::this_thread::get_id() << exception.what() << std::endl;
			exit(-1);
		} catch(...) {
			std::cerr << "consumeData: Unknown failure occurred. Possible memory corruption!" << std::endl;
			exit(-1);
		}
	}

	void writeHash(const Data& data) {
		m_fileWriter->write(data.second);
		std::cout << "hash = " << data.first << "  " << data.second << std::endl;
		std::cout << "m_currentRead = " << m_currentRead << std::endl;
		std::cout << "m_currentWritten = " << m_currentWritten << std::endl;
	}

	public:

	ProduceConsumer() = default;
	ProduceConsumer(
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

	void run() {
		m_producingDataFuture = std::async(std::launch::async, &ProduceConsumer::produceData, this);
		m_consumingDataFuture = std::async(std::launch::async, &ProduceConsumer::consumeData, this);
		m_checkingResultsFuture = std::async(std::launch::async, &ProduceConsumer::checkResults, this);
	}

	// yes a little bit ugly,
	// if we poll each thread without blocking we will get a chance to catch a exception from every thread
	void tryToStop() {

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

	~ProduceConsumer() {
		tryToStop();
	}
};

int main(int argc, char** argv) {
	if (argc != 4)
		return -1;

	try {
		ArgParser argParser;
		// here we can check validate input parameters
		if (!argParser.parse(argc, argv))
			return -1;

		// last piec of will be ignored
		uint32_t maxNumberOfElements = argParser.getInputFileSize() / argParser.getChunkSize();
		unsigned long lastChunkSize = argParser.getInputFileSize() % argParser.getChunkSize();

		if (lastChunkSize)
			++maxNumberOfElements;

		std::unique_ptr<FileReader> fileReader = std::unique_ptr<FileReader>(new FileReader(argParser.getInputFilePath(), argParser.getInputFileSize()));
		if (fileReader && !fileReader->isValid()) {
			return -1;
		}

		std::unique_ptr<FileWriter> fileWriter = std::unique_ptr<FileWriter>(new FileWriter(argParser.getOutputPath()));
		if (fileWriter && !fileWriter->isValid()) {
			return -1;
		}

		ProduceConsumer produceConsumer = ProduceConsumer(
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