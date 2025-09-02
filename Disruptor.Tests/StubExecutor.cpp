#include "stdafx.h"
#include "StubExecutor.h"

#include <future>
#include <functional>
#include <mutex>

namespace Disruptor
{
namespace Tests
{

    // std::future< void > StubExecutor::execute(const std::function< void() >& command)
    // {
    //     ++m_executionCount;

    //     std::future< void > result;

    //     if (!m_ignoreExecutions)
    //     {
    //         std::packaged_task< void() > task(command);
    //         result = task.get_future();

    //         std::lock_guard< decltype(m_mutex) > lock(m_mutex);
    //         m_threads.push_back(boost::thread(std::move(task)));
    //     }

    //     return result;
    // }

    std::future<void> StubExecutor::execute(const std::function<void()>& command)
    {
        m_executionCount.fetch_add(1, std::memory_order_relaxed);

        if (m_ignoreExecutions.load(std::memory_order_relaxed))
        {
            return std::async(std::launch::deferred, [] {});  // dummy future
        }

        std::packaged_task<void()> task(command);
        std::future<void> result = task.get_future();

        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_threads.emplace_back(std::thread(std::move(task)));

        return result;
    }

    // void StubExecutor::joinAllThreads()
    // {
    //     std::lock_guard< decltype(m_mutex) > lock(m_mutex);

    //     while (!m_threads.empty())
    //     {
    //         boost::thread thread(std::move(m_threads.front()));
    //         m_threads.pop_front();

    //         if (thread.joinable())
    //         {
    //             try
    //             {
    //                 thread.interrupt();
    //                 thread.timed_join(boost::posix_time::milliseconds(5000));
    //             }
    //             catch (std::exception& ex)
    //             {
    //                 std::cout << ex.what() << std::endl;
    //             }
    //         }

    //         EXPECT_FALSE(thread.joinable()) << "Failed to stop thread: " << thread.get_id();
    //     }
    // }
    void StubExecutor::joinAllThreads()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        while (!m_threads.empty())
        {
            std::thread thread(std::move(m_threads.front()));
            m_threads.pop_front();

            if (thread.joinable())
            {
                try
                {
                    // No `interrupt()` in std::thread. You must design cooperative cancel if needed.
                    thread.join();  // Blocking join
                }
                catch (const std::exception& ex)
                {
                    std::cout << "Exception while joining thread: " << ex.what() << std::endl;
                }
            }

            // If you're using Google Test's EXPECT_* macros, they provide better error reporting
            assert(!thread.joinable() && "Failed to stop thread");
        }
    }

    void StubExecutor::ignoreExecutions()
    {
        //m_ignoreExecutions = true;
        m_ignoreExecutions.store(true, std::memory_order_relaxed);
    }

    std::int32_t StubExecutor::getExecutionCount() const
    {
        //return m_executionCount;
        return m_executionCount.load(std::memory_order_relaxed);
    }

} // namespace Tests
} // namespace Disruptor
