## C++20 coroutines

The example uses C++20 stackless coroutines for doing asynchronous IO on top of epoll and non-blocking sockets. That is a relatively realistic potential usecase which at the same time looks simple enough to understand how those C++ builtin coroutines are working.

The program starts 2 threads: one is a worker for a bunch of clients, another is a worker for server and its peers. The threads serve IO of their sockets.

The test's goal is for the clients to send and receive N 1-byte messages, and then close the socket. At the same time the test's code shouldn't use any callbacks. All must be done using coroutines with `co_await` command.

### Summary

The test works, and the code looks simpler than it would be with the callbacks indeed. With smart approach to implementing those async operations they won't require any heap allocations, and the coroutine switching seems fast (although the test doesn't measure that), which means the performance is not a problem.

#### Can't yield from anywhere
However there is a significant downside, that the coroutines only allow to yield from the root function. In other words, the coroutine can't call a plain function which would `co_await` inside. That makes those coroutines hardly usable for any complex code having deep callstacks, doing multiple blocking operations during the processing. Such complex pipelines would have to be flattened into a sequence of steps to bring all the blocking operations up to the root of the coroutine. Stackfull coroutines don't have such issue, they allow to yield from any place.

#### About memory usage
Another point to mention is that those stackless coroutines are claimed to be very lightweight in terms of memory compared to the stackfull ones, because the latter need to allocate a big tens of KBs stack. That isn't really a problem, at least in Linux. Memory mapping from virtual to physical pages in Linux is lazy. It means, that if for a stackfull coroutine a stack 100MB is created as `mmap(100MB)`, then those 100MB won't instantly occupy 100MB physical memory. This call will only reserve a range of virtual memory of size 100MB for future use. The actual physical memory allocation will happen on demand, in 4KB blocks. That is, while this stackfull coroutine would be using only <= 4KB stack, only this size is mapped. As it will use more and more stack, it would physically grow in 4KB steps. That already isn't too much.
