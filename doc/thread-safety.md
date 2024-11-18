# Thread Safety {#thread-safety}

We use multiple threads in order to attain the performance advantages that come with parallelism. When our software can do multiple things at once, it takes less time to do its work, and users are less likely to notice pauses: cases where something they've asked the system to do needs to first wait on some other work to complete. Imagine a interactive 3D rendering system that blocked rendering and interactivity while it waited for content to load from the network. It would be unusable.

Developing multi-threaded software is difficult. We need to avoid data races and deadlocks, while also making sure we haven't inadvertently eliminated the parallelism that we were aiming for in the first place. These problems can sometimes be extremely subtle, occurring only when the system is under load or when certain unusual circumstances occur.

No amount of testing can prove that these problems don't exist. Convincing ourselves that they don't exist essentially amounts to constructing a "proof" based on the possible states of our software system. This is precarious, because the state space of even moderately complicated software tends to be enormous, and our proofs - out of pragmatic necessity - are usually informal. Worse, the assumptions that go into our informal proofs of correctness can be easily invalidated as our software evolves. A seemingly harmless change to a small part of the system can unwittingly invalidate our carefully-constructed thread safety, introducing a new data race that will manifest as incorrect results or a crash only later, on an end-user's system.

In the worst case, achieving correctness in a multithreaded program requires a perfect understanding of the entire system for every little change to it. In complex software, developed collaboratively by an open-source community, this is obviously impossible.

There is no magic wand solution to this problem, but we can make it more tractable through good design. By developing our software around certain rules and patterns, we allow developers to confidently use and modify our software without understanding every little detail of it. They only need to understand the patterns and how those patterns affect their current situation.

## Thread Safety Rules

The following are the thread safety rules and patterns employed by Cesium Native. We can think of this as a contract. Anyone implementing functionality in Cesium Native must ensure that they are fulfilling this contract. Anyone using functionality in Cesium Native can use it with confidence that the contract has been fulfilled.

In rare cases, this contract cannot be fulfilled. When that happens, the points of divergence must be documented very clearly. If the divergence is likely to be surprising, consider capturing the divergence in naming as well. For example, `CesiumUtility::ReferenceCountedNonThreadSafe` does not follow the usual rules.

These rules probably won't be surprising, and they also should not impose a large burden on implementers in most cases.

### Separate instances are safe to use concurrently.

Threads can simultaneously create and use their own private instances of a given `class` or `struct`, without any concerns about thread safety.

However, you must also consider the thread safety of any arguments passed to the constructor or any methods that you call. If any of them are shared between threads, then that usage must also be safe according to these rules.

##### Implementation Notes

This rule is usually satisfied without any particular effort from the implementer. However, if the instance accesses shared, mutable state (static fields, global variables, singletons), then access to this shared state must be serialized using a `std::mutex`.

### Static methods are safe to call concurrently.

It is safe to call a `static` method from multiple threads simultaneously.

You must also consider the thread safety of any arguments passed to the static method. If any of them are shared between threads, then that usage must also be safe according to these rules.

##### Implementation Notes

A static method often operates only on the arguments passed to it. As long as those arguments are not shared between threads (or are safe to use in this way), the static method itself will be safe to use from multiple threads simultaneously without any particular effort from the implementer.

When the static method accesses static fields, global variables, or singletons, more care must be taken by the implementer in order to satisfy this rule. If there is any possibility of the shared data changing (i.e., it's not completely immutable), a synchronization primitive such as a `std::mutex` must be used to serialize access to the shared data.

### A single instance may be used concurrently as long as _all_ accesses are `const`.

It is safe to share a single instance of a `class` or `struct` between threads as long as all threads are restricted to `const` access of the instance.

If even a single thread is or may be modifying the instance, then it is NOT safe for any other thread to access the instance in any way.

Note that if this instance references other instances, then those other instances must also be safe to use according to these rules.

##### Implementation Notes

This rule is usually satisfied without any particular effort from the implementer. However, there are a few cases to be aware of:

1. _Mutable fields_: If the type has a field declared `mutable`, it means that field can be modified even via const access. To conform to this rule, access to that field must be serialized with a `std::mutex`. Often, the presence of a `mutable` field indicates a type that cannot realistically conform to these rules, and it should be documented as such.

2. _Const casts_: Casting away const with a `const_cast` is always a potentially dangerous thing to do. However, according to these rules, it can also introduce thread safety concerns.

3. _Pointers and references_: Maybe people are surprised to learn that `const` is not transitive through pointers and references. For example, in a type like this:

```cpp
struct Target {
  int value;
};

struct Foo {
  Target* p;
};
```

It is perfectly allowed to modify the _target_ of `p` via a const reference to `Foo`, like this:

```cpp
const Foo foo = ...;
foo.p->value = 10;
```

This is also true if `p` were a reference, a `std::unique_ptr`, a `std::span`, etc. However, other types like `std::vector` would not permit this.

Implementers should take care not to abuse this behavior. An implementation that modifies a referenced object from a const method is very likely to run afoul of these rules.
