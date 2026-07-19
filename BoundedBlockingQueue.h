#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <stdexcept>

template<typename T>
class BoundedBlockingQueue
{
public:
	explicit BoundedBlockingQueue(size_t capacity)
		: capacity_(capacity)
	{
		if (capacity_ == 0)
		{
			throw std::invalid_argument("BoundedBlockingQueue: 0 capacity");
		}
	}

	template <typename V>
	requires std::constructible_from<T, V>
	bool push(V&& value)
	{
		std::unique_lock lock(mtx_);
		prod_cv_.wait(lock, [this](){
			return stopped_ || data_.size() < capacity_;
		});
		
		bool status = false;
		if (!stopped_)
		{
			data_.emplace(std::forward<V>(value));
			status = true;
		}
		
		lock.unlock();
		cons_cv_.notify_one();
		
		return status;
	}
	
	std::optional<T> pop()
	{
		std::unique_lock lock(mtx_);
		cons_cv_.wait(lock, [this](){
			return stopped_ || !data_.empty();
		});
		
		if (data_.empty())
		{
			return std::nullopt;
		}
		
		T front = std::move(data_.front());
		data_.pop();
		
		lock.unlock();
		prod_cv_.notify_one();
		
		return { std::move(front) };
	}
	
	void shutdown() noexcept
	{
		{
			std::lock_guard lock(mtx_);
			stopped_ = true;
		}
		prod_cv_.notify_all();
		cons_cv_.notify_all();
	}
    
private:
	std::queue<T> data_;
	std::mutex mtx_;
	
	std::condition_variable prod_cv_;
	std::condition_variable cons_cv_;
	
	const size_t capacity_;
	bool stopped_ = false;
};
