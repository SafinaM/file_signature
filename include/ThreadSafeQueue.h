template <typename T>
class ThreadSafeQueue {
public:
	ThreadSafeQueue() = default;
	ThreadSafeQueue(const ThreadSafeQueue& other) {
		std::lock_guard<std::mutex> lk(other.mut);
		m_queue = other.data_queue;
	}

	void push(const T& value) {
		std::lock_guard<std::mutex>lockGuard(m_mutex);
		m_queue.push(value);
		m_cond.notify_one();
	}

	bool try_pop(T& value) {
		std::lock_guard<std::mutex> lk(m_mutex);
		if (m_queue.empty())
			return false;
		value = m_queue.front();
		m_queue.pop();
		return true;
	}

	std::shared_ptr<T> try_pop() {
		std::lock_guard<std::mutex> lk(m_mutex);
		if (m_queue.empty())
			return nullptr;
		std::shared_ptr<T> ptr = std::make_shared<T>(m_queue.front());
		m_queue.pop();
		return ptr;
	}

	bool empty() {
		std::lock_guard<std::mutex> lockGuard(m_mutex);
		return m_queue.empty();
	}

private:
	mutable std::mutex m_mutex;
	std::condition_variable m_cond;
	std::queue<T> m_queue;
};
