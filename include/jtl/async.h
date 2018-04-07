#ifndef JTL_ASYNC_H
#define JTL_ASYNC_H

#include <future> // std::future

namespace jtl
{
#if 0
	template< class Function, class... Args>
	std::future<typename std::result_of<Function(Args...)>::type> async(Function&& f, Args&&... args)
	{
		return std::async(std::launch::async, std::forward<Function>(f), std::forward<Args>(args)...);
	}
#else
	template< class Function, class... Args>
	std::future<typename std::result_of<Function(Args...)>::type> async(Function&& f, Args&&... args)
	{
		typedef typename std::result_of<Function(Args...)>::type R;
		auto bound_task = std::bind(std::forward<Function>(f), std::forward<Args>(args)...);
		std::packaged_task<R()> task(std::move(bound_task));
		auto ret = task.get_future();
		std::thread t(std::move(task));
		t.detach();
		return ret;
	}
#endif

}

#endif
