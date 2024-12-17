# AsyncSystem {#async-system}

[AsyncSystem](@ref CesiumAsync::AsyncSystem) is Cesium Native's primary mechanism for managing asynchronous, multi-threaded work. Understanding it is essential for using, modifying, and extending Cesium Native.

An example will help to illustrate why we need `AsyncSystem` at all.

@mermaid{async-system-load-example}

Let's say we're trying to load a tile in a [3D Tiles](https://github.com/CesiumGS/3d-tiles) tileset. First, we need to start a network request to download the tile's content. We'll do an HTTP GET for a particular URL, and some time tens to hundreds to even thousands of milliseconds later, the tile content at that URL will be downloaded and ready for us to use.

Once it's downloaded, we need to process that tile content that we just downloaded. That means:

* Parse the JSON it contains,
* Decompress the Draco-encoded meshes or the JPEG-encoded images,
* Process the triangle meshes for use with a physics engine,
* And lots more!

We want to do as much of this kind of work as we possibly can in a background thread, because CPUs have multiple cores these days and the main thread is usually busy doing other important work.

Unfortunately, there is inevitably at least a little bit of tile loading work that can't be done in a background thread. Game engines, for example, usually have strict rules against creating game objects anywhere other than the main thread. We can't get anything onto the screen without creating game objects, so after the background work is complete, we then need to continue processing this tile in the main thread in order to do the final preparation to render it.

`AsyncSystem` gives us an elegant way to express this kind of sequential process involving a series of asynchronous steps.
<!--! [TOC] -->

## The AsyncSystem Class {#asyncsystem-class}

An `AsyncSystem` object manages the other objects we use to schedule work and wait for its completion. During initialization, an application [constructs an AsyncSystem object](#creating-an-asyncsystem) and passes it to Cesium Native, which in turn uses it in all operations that might complete asynchronously. `AsyncSystem` instances can be safely and efficiently stored and copied by value; this makes it easy to make them available wherever they're needed, including in lambda captures. On the other hand, holding a reference to an `AsyncSystem` object is very bug-prone in asynchronous code; the lifetime of the reference holder can be quite different from the code that uses it and hard to reason about. It is idiomatic to pass a `const` reference to an `AsyncSystem` object as a parameter to a function, but that reference must be copied to a value if it will be used outside of the lifetime of the function. For example, in the following code, the `asyncSystem` function parameter is copied to a constant value member in the inner lambda:

\snippet{trimleft} Cesium3DTilesReader/src/SubtreeFileReader.cpp async-system-store-in-lambda

## Future<T> {#future}

[Future<T>](@ref CesiumAsync::Future) represents an asynchronous operation that will eventually complete and produce a value of type `T`. If you're familiar with a JavaScript [Promise](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise), the concept is very much the same. We commonly see `Future<T>` as the return value of a method that starts an asynchronous operation. For example, we can use [IAssetAccessor](@ref CesiumAsync::IAssetAccessor) to asynchronously download the content of a web URL:

\snippet{trimleft} ExamplesAsyncSystem.cpp create-request-future

This [get](@ref CesiumAsync::IAssetAccessor::get) function will start the HTTP request, but it will return quickly, without waiting for the request to complete. Initially, the `Future` is in the _Pending_ state, meaning that the asynchronous operation has not yet completed. Once it does complete, the `Future` will be in one of these two states:

* _Resolved_: The operation has completed successfully and the result value is available.
* _Rejected_: The operation failed, and an exception describing the error is available.

If we want to block the current thread and wait for the request to complete, we can call [wait](@ref CesiumAsync::Future::wait):

\snippet{trimleft} ExamplesAsyncSystem.cpp wait

`wait` blocks until the `Future` completes. If it is resolved, `wait` returns its value. If it is rejected, `wait` throws an exception representing the error.

> [!note]
> The `std::move` is needed because `Future<T>` is meant to be consumed just once. This allows for best efficiency because the value resulting from the asynchronous operation can be provided to the caller without ever making a copy of it.

`wait` should never be called in the "main thread" (defined in the next section), because doing so can cause a deadlock. To block the main thread waiting for a `Future`, call [waitInMainThread](@ref CesiumAsync::Future::waitInMainThread) instead.

In practice, though, we almost never call `wait` or `waitInMainThread`. After all, what good is it to start an asynchronous operation if we're just going to block the calling thread waiting for it? Instead, we register a _continuation function_ with the `Future`.

## Continuation Functions {#continuation-functions}

A continuation function is a callback - usually a lambda - that is invoked when the `Future` resolves. The name comes from the idea that the execution "continues" with that function after the async operation. Continuations are registered with the `then...` methods on the `Future`. Different `then` functions are used to control _in what thread_ the callback is invoked:

* [thenInMainThread](@ref CesiumAsync::Future::thenInMainThread): The continuation function is invoked in the "main" thread. Many applications have a clear idea of which thread they consider the main one, but in Cesium Native, "main thread" very specifically means the thread that called `waitInMainThread` or [dispatchMainThreadTasks](@ref CesiumAsync::AsyncSystem::dispatchMainThreadTasks) on the `AsyncSystem`. This need not be the same as the thread your application considers to be the main one.
* [thenInWorkerThread](@ref CesiumAsync::Future::thenInWorkerThread): The continuation function is invoked in a background worker thread by calling [startTask](@ref CesiumAsync::ITaskProcessor::startTask) on the `ITaskProcessor` instance with which the `AsyncSystem` was constructed.
* [thenInThreadPool](@ref CesiumAsync::Future::thenInThreadPool): The continuation function is invoked in a specified [ThreadPool](@ref CesiumAsync::ThreadPool). This is most commonly used with a `ThreadPool` containing just a single thread, in order to delegate certain tasks (such as database access) to a dedicated thread. In other cases, `thenInWorkerThread` is probably a better choice.
* [thenImmediately](@ref CesiumAsync::Future::thenImmediately): The continuation function is invoked immediately in whatever thread happened to resolve the `Future`. This is only appropriate for work that completes very quickly and is safe to run in any thread.

Continuing with our web request example above, we can register a continuation that receives the completed request and does something with the downloaded data in the main thread:

\snippet{trimleft} ExamplesAsyncSystem.cpp continuation

So far, this all looks like a glorified callback system. The real power of `AsyncSystem` becomes apparent when we chain continuations together. Here's what that might look like:

\snippet{trimleft} ExamplesAsyncSystem.cpp chaining

In this example, we call `get` to start a network request, as before. We immediately call `thenInWorkerThread` on the returned `Future` in order to register a continuation for when the request completes. This continuation runs in a background thread and calls `processDownloadedContent`. We can imagine that it's doing some intensive processing, and we wouldn't want to block the main thread while it's doing it.

The continuation lambda returns an instance of example type `ProcessedContent`. Because the lambda function we pass to it returns `ProcessedContent`, the `thenInWorkerThread` call returns `Future<ProcessedContent>`. This new `Future` will resolve when the request completes _and then_ the `processDownloadedContent` also completes.

We can attach a further continuation to this new `Future`, which we do in the example by calling `thenInMainThread`. This second continuation will be invoked in the main thread once the `ProcessedContent` is asynchronously created. It calls `updateApplicationWithProcessedContent`, which we can imagine updates the user interface based on the downloaded and processed content.

With just a few lines of code, we've created a process that:

1. Asynchronously downloads a resource from the internet.
2. Does some CPU-intensive processing on that resource in a background thread.
3. Updates the user interface, which can usually only be done in the main thread.

We've done this:

1. Without ever blocking any threads.
2. Without writing any complicated thread synchronization code, such as with `std::mutex`.

## Catch {#catch}

The `then...` family of functions register continuations that are invoked when a `Future` resolves (completes successfully). What if the `Future` rejects (completes with an exception) instead?

We can register continuations for the rejection case by calling the [catchInMainThread](@ref CesiumAsync::Future::catchInMainThread) and [catchImmediately](@ref CesiumAsync::Future::catchImmediately) methods instead.

> [!note]
> Cesium Native does not currently have a `catchInWorkerThread` or `catchInThreadPool` method.

Unlike the `then...` family continuations, which receive the value of the successful asynchronous operation, the `catch...` family continuations receive a `std::exception`. A common pattern is to turn a catch exception into a valid value that indicates something went wrong:

\snippet{trimleft} ExamplesAsyncSystem.cpp catch

In this example, we call `startOperationThatMightFail`, and it returns a `Future<ProcessedContent>`. We then call `catchImmediately` on it in order to register a continuation that will be invoked if the operation fails with an exception. Notice that this continuation returns a `ProcessedContent` too, in this case created from the failure error message. This is a common pattern - a `catch` continuation turning the error into a result value that encapsulates the error.

The `catchImmediately` returns a new `Future`, just like a `then...` method does. The value type `T` of the `Future<T>` will be the same as the original `Future` that we called `catchImmediately` on, and it is for this reason that the continuation function given to `catch...` _must_ return a value of the same type that `startOperationThatMightFail` would have produced on success. The only other option is to throw an exception, at which point execution will continue at the continuation provided to the _next_ `catch...` in the chain.

Note that a `catch...` continuation can handle an exception or rejected `Future` that reaches it from _any_ prior point in the `Future` chain. When a `catch...` continuation is invoked, it is not necessarily the `Future` to which it is directly attached that experienced the error. When a `Future` rejects, execution resumes at the next `catch...` continuation in the chain, skipping all `then...` continuations along the way.

As the name implies, `catch...` continuations are very similar to exception handlers in normal synchronous code. They're generally only used to handle truly exceptional situations, and are best avoided for normal control flow.

## Future Unwrapping {#future-unwrapping}

In our examples above, both `then...` and `catch...` continuations always returned a value of some type `T`. A continuation may instead return a `Future<T>`! When this happens, the `AsyncSystem` "unwraps" the returned `Future`. That means that the `Future` returned by the `then...` or `catch...` will not itself resolve or reject until the `Future` returned by the continuation resolves or rejects. An example will make this clearer.

Imagine that we're downloading a web page from the internet, as we were in previous examples. In this case, though, the web page references some other internet resource, such as an image. We want to download that too, once we know what it is, and aren't done until both the original web page and the image are downloaded. We can do this by taking advantage of `Future` unwrapping, like so:

\snippet{trimleft} ExamplesAsyncSystem.cpp unwrapping

In this example, our first continuation calls `findReferenceImageUrl` to determine the URL of an image referenced by the page. We then call `get` to download that image. Notice that we've captured `asyncSystem` and `pAssetAccessor` in the lambda in order to make this possible. The `get` call returns a `Future`, and we return that `Future` from our continuation. The returned `Future` is unwrapped by the `AsyncSystem`, and as a result, the second continuation, the one passed to `thenInMainThread`, will not be invoked until this second request `Future` resolves.

You may have noticed that our second continuation, the one passed to `thenInMainThread`, only has access to the image's `IAssetRequest`. What if we also need to use the page `IAssetRequest`? Or some other data derived from it? While it's possible to do this manually with extra continuations and lambda captures, the `thenPassThrough` method makes the process simpler.

## thenPassThrough {#then-pass-through}

The [thenPassThrough](@ref CesiumAsync::Future::thenPassThrough) method makes it easy to pass through some extra data to a continuation. This is especially useful in combination with `Future` unwrapping. It looks like this:

\snippet{trimleft} ExamplesAsyncSystem.cpp then-pass-through

Like the example in the previous section, we're finding a referenced image URL and then initiating a second HTTP request to go download it. This time, though, we call `thenPassThrough` on the `Future` returned by the `get`, and pass it the `ProcessedContent`. As a result, the next continuation in the chain receives _both_ the `ProcessedContent` and the image `IAssetRequest` in the form of a `std::tuple`.

## Combining Futures {#combining-futures}

In the previous examples, we downloaded a web page, found the URL of an image within it, and then downloaded that image, too. But what if the page contains multiple images? That's where the [all](@ref CesiumAsync::AsyncSystem::all) method on `AsyncSystem` comes in. Given a `std::vector<Future<T>>`, the `all` method of `AsyncSystem` returns a `Future<std::vector<T>>`. In other words, it turns a list of `Futures` into a single `Future` that resolves to a list of values. We can use it like this:

\snippet{trimleft} ExamplesAsyncSystem.cpp all

The `Future<std::vector<T>>` returned by `all` resolves when all of the Futures it is given resolve. It rejects when _any_ of the Futures it is given reject. To prevent a single rejection from making all of the resolved Futures inaccessible, add a `catch...` continuation to each Future and pass the return value to `all`.

## Creating Futures {#creating-futures}

So far, the initial `Future<T>` instances we have used have been created by others. For example, we've used `IAssetAccessor::get` to create a `Future<T>` representing a web resource once the asynchronous download is complete. How do those `Future<T>` instances actually get created?

The simplest way to create a `Future<T>` is by calling [createResolvedFuture](@ref CesiumAsync::AsyncSystem::createResolvedFuture) on the `AsyncSystem`. Given a value, it creates an already-resolved `Future<T>` to hold that value. This is most useful when combined with [Future Unwrapping](#future-unwrapping). Because all return statements in a given continuation must return either a value or a `Future<T>` (we can't do both within a single continuation), `createResolvedFuture` is useful when some branches within our continuation are ready to return a value directly, while others need to start a new asynchronous operation. For example:

\snippet{trimleft} ExamplesAsyncSystem.cpp create-resolved-future

In this example, if the initial web page doesn't contain a referenced image, we immediately return a `nullptr` image request using `createResolvedFuture`. If it does, we call `get` to download it.

Another way to create a `Future<T>` is by calling [runInMainThread](@ref CesiumAsync::AsyncSystem::runInMainThread), [runInWorkerThread](@ref CesiumAsync::AsyncSystem::runInWorkerThread), or [runInThreadPool](@ref CesiumAsync::AsyncSystem::runInThreadPool) on `AsyncSystem`. These are similar to the equivalent `then...` methods in that they take a function - usually a lambda - that is invoked in the appropriate threading context. Unlike the `then...` methods, which _continue_ after some other asynchronous operation, these methods start a brand new asynchronous operation. Just like the `then...` methods, though, the `runIn...` methods return a `Future<T>`, allowing additional asynchronous work to continue - perhaps in a different thread - after the initial function returns. This makes `runInWorkerThread` a very convenient way to do some work in the background and retrieve its value when it completes.

The most powerful way to create a `Future<T>`, though, is to use a `Promise<T>`.

## Promise<T> {#promise}

As previously described, a `Future<T>` represents a value that will become available sometime in the future. A [Promise<T>](@ref CesiumAsync::Promise) is how the asynchronous operation actually _provides_ that value when it becomes available. It is most useful when interfacing with libraries that use other patterns for asynchronous work, and allow us to bring their results into the `AsyncSystem`.

Imagine that we're using a third-party library that does asynchronous work with a simple callback-based API. The library's function called `computeSomethingSlowly` does some work in the background and then calls the callback that we pass to it:

\snippet{trimleft} ExamplesAsyncSystem.cpp compute-something-slowly

We'd like to be able to use the `AsyncSystem` to do further work after this completes. Here's how that can be done:

\snippet{trimleft,strip} ExamplesAsyncSystem.cpp compute-something-slowly-async-system

The trick is to call [createPromise](@ref CesiumAsync::AsyncSystem::createPromise) to create a `Promise<T>`, and then capture it in the callback lambda given to the library's `computeSomethingSlowly`. When the callback is invoked, we call [resolve](@ref CesiumAsync::Promise::resolve) on the `Promise<T>`, which resolves the associated `Future<T>` that was previously obtained with [getFuture](@ref CesiumAsync::Promise::getFuture). We can then chain continuations off that `Future<T>` in the normal way. Instead of calling `resolve` on the `Promise<T>`, we can also call [reject](@ref CesiumAsync::Promise::reject).

One problem with this `createPromise` approach is that it does not behave as well as it should in the face of exceptions. Imagine that we're wrapping all of this functionality up in our own function, like this:

\snippet{trimleft} ExamplesAsyncSystem.cpp compute-something-slowly-wrapper

Now what happens if that third-party library function, `computeSomethingSlowly`, throws an exception? Our `myComputeSomethingSlowlyWrapper` doesn't catch it, so the exception will be propagated out of our function and the caller will have to deal with it. This is poor form in a function that returns a `Future<T>`, because it forces callers to handle two different error mechanisms. The function might throw an exception, or the function might reject the `Future<SlowValue>` that it returns. Handling both would be a hassle, and it's unfair to ask our users to do so.

We could change `myComputeSomethingSlowlyWrapper` to catch the possible exception and turn it into a call to `reject` on the `Promise<T>`, and that would work fine. But there's an easier way. We can use the [createFuture](@ref CesiumAsync::AsyncSystem::createFuture) method on `AsyncSystem`:

\snippet{trimleft} ExamplesAsyncSystem.cpp compute-something-slowly-wrapper-handle-exception

The `createFuture` method takes a function as its only parameter, and it _immediately_ invokes that function in the current thread (that's why it's safe to capture everything by reference `&` in this case). The function receives a parameter of type `Promise<T>`, which is identical to the one that would be created by `createPromise`. Just like before, we call the library function that we're wrapping and resolve the `Promise<T>` when its callback is invoked.

The important difference, however, is that if `computeSomethingSlowly` throws an exception, `createFuture` will automatically catch that exception and turn it into a rejection of the `Future<T>`.

## SharedFuture<T> {#shared-future}

A `Future<T>` may have at most one continuation. It's not possible to arrange for two continuation functions to be called when a given `Future<T>` resolves. This policy makes `Future<T>` more efficient by minimizing bookkeeping. In the majority of cases, this is sufficient. But not always.

In those cases where we do want to be able to attach multiple continuations, we can convert a `Future<T>` into a [SharedFuture<T>](@ref CesiumAsync::SharedFuture) by calling the [share](@ref CesiumAsync::Future::share) method on it. The returned `SharedFuture<T>` instance works a bit like a smart pointer. It's safe to copy, and all copies are logically identical to the original. Just like `Future<T>`, we can call `then...` and `catch...` methods to attach continuations. The difference is that we can do this multiple times, even on a single `SharedFuture<T>` instance, and all attached continuations will be invoked when the future completes.

It may initially be surprising to learn that calling `then...` or `catch...` on a `SharedFuture<T>` returns a `Future<T>`, not another `SharedFuture<T>`. This is again for efficiency. Just because multiple continuations are needed for one point in a continuation chain does not necessarily mean later parts of the chain will also need multiple continuations. If they do, however, it's just a matter of calling `share` again.

## Lambda Captures and Thread Safety {#lambda-captures-and-thread-safety}

`AsyncSystem` is a powerful abstraction for writing safe and easy-to-understand multithreaded code. Even if so, `AsyncSystem` does not completely prevent you from creating data races. In particular, it's essential to use care and good judgment when choosing what to capture in continuation lambdas. Here are some tips:

* DO NOT capture by reference or pointer, unless you're certain that the object referenced is thread-safe and will still be around when the continuation is invoked. This can be difficult to achieve in practice!
* DO think in terms of transferring ownership of an object to the promise chain, and transferring ownership back out at the end. This requires use of `std::move` and a `mutable` lambda (so that the captured value is not `const`). It looks like this:

\snippet{trimleft} ExamplesAsyncSystem.cpp lambda-move

* DO be aware of _when_ and _in what thread_ lambda captures are destroyed. Continuation lambda captures are destroyed immediately after the continuation runs, in whatever thread ran the continuation. If a `catch...` continuation is skipped because the `Future` to which it's attached resolved instead of rejecting, the continuation's captures are destroyed in whatever thread did the resolving. Similarly, if a `then...` continuation is skipped because the `Future` to which it's attached rejected, the continuation's captures are destroyed in whatever thread did the rejecting.
* DO NOT capture an [IntrusivePointer](@ref CesiumUtility::IntrusivePointer) to a non-thread-safe object (such as one derived from `ReferenceCountedNonThreadSafe`) except in a continuation that runs in the thread that owns it. For example, in the usual case that an object is owned by the "main thread", a pointer to that object should only be captured in a lambda given to `runInMainThread`, `thenInMainThread`, or `catchInMainThread`. Furthermore, as a corollary to the item above, ensure that these `IntrusivePointer`-capturing continuations cannot be skipped when the Future is either resolved or rejected, because this could result in the `IntrusivePointer` being destroyed in the wrong thread. Failure to follow this rule can lead to corruption of the object's reference count, leading to some difficult to debug problems. In Debug builds, assertions will help to detect this sort of problem.

## AsyncSystem and ITaskProcessor Implementation {#async-system-implementation}

### Creating an AsyncSystem {#creating-an-asyncsystem}

[AsyncSystem](@ref CesiumAsync::AsyncSystem) is implemented using a subclass of the [ITaskProcessor](@ref CesiumAsync::ITaskProcessor) interface class via [dependency injection](https://en.wikipedia.org/wiki/Dependency_injection).
`ITaskProcessor` specifies a simple interface used to perform some work in a background thread. The simplest possible implementation looks like this:

\snippet{trimleft} ExamplesAsyncSystem.cpp simplest-task-processor

This implementation will work, but it isn't very efficient because a brand new thread is created for each task. Most applications will implement this interface using a thread pool, task graph, or similar functionality that their application already contains. 

The `AsyncSystem` could be created as follows:

\snippet{trimleft} ExamplesAsyncSystem.cpp create-async-system

However, Cesium Native does not contain any calls like this, other than in test code. Except under unusual circumstances, applications should construct a single `AsyncSystem` object that is copied and is used throughout. Two `AsyncSystem` objects that are created with this constructor, even if they share the same `ITaskProcessor`, will have different work queues and different notions of the main thread and, if they are used together, disaster can result. 

On the other hand, `AsyncSystem` instances have copy semantics, so it is easy to make them available wherever they're needed, including in lambda captures. In this example, `asyncSystem` is captured by value for use in the lambda.

\snippet{trimleft} ExamplesAsyncSystem.cpp capture-by-value

You can think of an instance of `AsyncSystem` as a reference (perhaps a "smart reference") to an underlying implementation which includes an `ITaskProcessor` and all the supporting data structures for implementing futures, promises, and other `AsyncSystem` abstractions. When we create an `AsyncSystem` using its constructor, we're creating a brand new underlying implementation. If we then copy that `AsyncSystem` (using its copy constructor or assignment operator), we're not really copying that underlying implementation, we're just creating another reference to the same one with its `ITaskProcessor` object. Only when the last `AsyncSystem` instance referencing a particular underlying implementation is destroyed is that _underlying implementation_ destroyed.

You can copy and destroy `AsyncSystem` instances at will, but you must take care that the _last_ instance referencing a given underlying implementation is destroyed only after all of that underlying implementation's `Futures` are complete. So a common pattern is to create and store an `AsyncSystem` as a static local in an accessor function:

\snippet{trimleft} ExamplesAsyncSystem.cpp async-system-singleton

