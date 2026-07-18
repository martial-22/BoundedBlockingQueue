#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <stdexcept>

namespace std
{

template<typename T>
class BoundedBlockingQueue
{
public:
    explicit BoundedBlockingQueue(size_t capacity)
					: capacity_(capacity)
    {
	    if (capacity_ == 0)
	    {
		    throw invalid_argument("BoundedBlockingQueue: 0 capacity");
	    }
    }

	template <typename V>
    bool emplace(V&& value)
    {
	    unique_lock<mutex> lock(mtx_);
	    
	    prod_cv_.wait(lock, [&](){
		    return stopped_ || data_.size() < capacity_;
	    });
	    
	    bool status = false;
	    if (!stopped_)
	    {
            data_.emplace(forward<V>(value));
            status = true;
		}
		
		lock.unlock();
		cons_cv_.notify_one();
		
		return status;
    }

    optional<T> pop()
    {
	    unique_lock<mutex> lock(mtx_);
	    
	    cons_cv_.wait(lock, [&](){
		    return stopped_ || !data_.empty();
		});
    
	    if (data_.empty())
	    {
		    return nullopt;
	    }
	    T front = move(data_.front());
	    data_.pop();
	    
	    lock.unlock();
	    prod_cv_.notify_one();
	    
	    return front;
    }

    void shutdown()
    {
	    unique_lock<mutex> lock(mtx_);
	    stopped_ = true;
	    lock.unlock();
	    
	    prod_cv_.notify_all();
	    cons_cv_.notify_all();
    }
    
private:
	queue<T> data_;
	mutex mtx_;
	
	condition_variable prod_cv_;
	condition_variable cons_cv_;
	
	const size_t capacity_;
	bool stopped_ = false;
};

}