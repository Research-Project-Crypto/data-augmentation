#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <stack>
#include <thread>

namespace program
{
	class thread_pool
	{
        // atomic variable == thread safe
		std::atomic<bool> m_accept_jobs;
        // 
		std::condition_variable m_data_condition;

        // stack (list) with functions with return type void
		std::stack<std::function<void()>> m_job_stack;
        // mutex => thread lock prevent access violation errors
		std::mutex m_lock;
        // vector (fancy list) with all the threads
		std::vector<std::thread> m_thread_pool;

        // main thread to launch thread pool
		std::thread m_managing_thread;
	public:
        // constructor of class
		thread_pool();
        // destructor
		~thread_pool();

        // destroy thread pool
		void destroy();
		bool has_jobs();
        // push function / lambda on stack
		void push(std::function<void()> func);
	private:
        // create thread pool
		void create();
        // tell the thread pool we're done using it and kill of all threads
		void done();
        // infinite running function waiting for new jobs to be pushed on stack
		void run();
	};

    // inline == global
    // thread_pool == instance
    // * == pointer
    // g_thread_pool == variable name
    // {} == initiate class object
	inline thread_pool* g_thread_pool{};
}