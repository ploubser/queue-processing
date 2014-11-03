#include <queue>
#include <iostream>
#include <mutex>
#include <thread>
#include <unistd.h>

template <typename ChannelType>
class Channel {
  public:
    explicit Channel(std::function<void(ChannelType)> callback) : callback_(callback) {
        for (int i = 0; i < 5; i++) {
            thread_pool_.push_back(std::thread(&Channel::processChannel, this));
        }
    };

    ~Channel() {
         {
            std::unique_lock<std::mutex> lock(m_);
            destroying_ = true;
        }

        condvar_.notify_all();
        for (auto& t : thread_pool_) {
            t.join();
        }
    }

    void push(ChannelType t) {
        {
            std::unique_lock<std::mutex> lock(m_);
            internal_queue_.push(t);
        }
        condvar_.notify_one();
    }

    void processChannel() {
        for (;;) {
             std::unique_lock<std::mutex> lock(m_);

            condvar_.wait(lock,  [this] () {
                return !internal_queue_.empty() || destroying_;
            });

            if (destroying_ && internal_queue_.empty()) {
                return;
            }

            ChannelType tmp = internal_queue_.front();
            internal_queue_.pop();
            lock.unlock();
            callback_(internal_queue_.front());
        }
    };

  private:
    std::queue<ChannelType> internal_queue_;
    std::vector<std::thread> thread_pool_;
    std::function<void(ChannelType)> callback_;
    std::mutex m_;
    std::condition_variable condvar_;
    bool destroying_ = false;

};


void test(std::string x) {
    sleep(1);
    std::cout << x << std::endl;
}

int main() {
    Channel<std::string> c { test };
    c.push("foobar");
    c.push("foobar");
    c.push("foobar");
    c.push("foobar");

    c.push("foobar");
    c.push("foobar");
    c.push("foobar");
    c.push("foobar");
}
