# Multithreading {#multithreading}

We use multiple threads in order to attain the performance advantages that come with parallelism. When our software can do multiple things at once, it takes less time to do its work, and users are less likely to notice pauses: cases where something they've asked the system to do needs to first wait on some other work to complete. Imagine a interactive 3D rendering system that blocked rendering and interactivity while it waited for content to load from the network. It would be unusable.

Developing multi-threaded software is difficult. We need to avoid data races and deadlocks, while also making sure we haven't inadvertently eliminated the parallelism that we were aiming for in the first place. These problems can sometimes be extremely subtle, occurring only when the system is under load or when certain unusual circumstances occur.

No amount of testing can prove that these problems don't exist. Convincing ourselves that they don't exist essentially amounts to constructing a "proof" based on the possible states of our software system. This is precarious, because the state space of even moderately complicated software tends to be enormous, and our proofs - out of pragmatic necessity - are usually informal. Worse, the assumptions that go into our informal proofs of correctness can be easily invalidated as our software evolves. A seemingly harmless change to a small part of the system can unwittingly invalidate our carefully-constructed thread safety, introducing a new data race that will manifest as incorrect results or a crash only later, on an end-user's system.

In the worst case, achieving correctness in a multithreaded program requires a perfect understanding of the entire system for every little change to it. In complex software, developed collaboratively by an open-source community, this is obviously impossible.

There is no magic wand solution to this problem, but we can make it more tractable through good design. By developing our software around certain rules and patterns, we allow developers to confidently use and modify our software without understanding every little detail of it. They only need to understand the patterns and how those patterns affect their current situation.

* \subpage thread-safety-rules - Rules and patterns for safe, understandable multi-threaded code.
* \subpage async-system - The foundational abstraction for asynchronous, multi-threaded work. Allows us to express a sequential process involving a series of asynchronous steps.


