# AsyncSystem {#async-system}

`AsyncSystem` is Cesium Native's primary mechanism for managing asynchronous, multi-threaded work. Understanding it is essential for using, modifying, and extending Cesium Native.

An example will help to illustrate why we need `AsyncSystem` at all.

<!-- Add simple diagram from here, probably turned horizontal: https://docs.google.com/presentation/d/12gxJrXKe4dolyZHhu-LSkHhBb_FsNP1LkVB_cRALWO8/edit#slide=id.g2e680c17110_0_0 -->

Let's say we're trying to load a tile in a [3D Tiles](https://github.com/CesiumGS/3d-tiles) tileset. First, we need to start a network request to download the tile's content. We'll do an HTTP GET for a particular URL, and some time tens to hundreds to even thousands of milliseconds later, the tile content at that URL will be downloaded and ready for us to use.

Once it's downloaded, we need to process that tile content that we just downloaded. That means:

* Parse the JSON it contains,
* Decompress the Draco-encoded meshes or the JPEG-encoded images,
* Process the triangle meshes for use with a physics engine,
* And lots more!

We want to do as much of this kind of work as we possibly can in a background thread, because CPUs have multiple cores these days and the main thread is usually busy doing other important work.

Unfortunately, there is inevitably at least a little bit of tile loading work that can't be done in a background thread, though. Game engines, for example, usually have strict rules against creating game objects anywhere other than the main thread. We can't get anything onto the screen without creating game objects, so after the background work is complete, we then need to continue processing this tile in the main thread in order to do the final preparation to render it.

`AsyncSystem` gives us an elegant way to express this kind of sequential process involving a series of asynchronous steps.

## Creating an AsyncSystem

Most applications have a single `AsyncSystem` that is used throughout. It is constructed from an `ITaskProcessor` instance, which is a simple interface used to perform some work in a background thread. The simplest possible implementation looks like this:

\snippet ExamplesAsyncSystem.cpp simplest-task-processor

This implementation will work well, but it isn't very efficient because a brand new thread is created for each task. Most applications will implement this interface using a thread pool, task graph, or similar functionality that their application already contains.

The `AsyncSystem` can then be created as follows:

\snippet ExamplesAsyncSystem.cpp create-async-system

`AsyncSystem` instances can be safely and efficiently stored and copied by value. This makes it easy to make them available wherever they're needed, including in lambda captures. You can think of an `AsyncSystem` instance as acting a bit like a smart pointer.

\snippet ExamplesAsyncSystem.cpp capture-by-value

However, it is essential that the last `AsyncSystem` instance be destroyed only after everything is done using it. So a common pattern is to create and store it as a static local in an accessor function:

\snippet ExamplesAsyncSystem.cpp async-system-singleton

## Future<T>

`Future<T>` represents an asynchronous operation that will eventually complete and produce a value of type `T`. If you're familiar with a JavaScript [Promise](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise), the concept is very much the same. We commonly see `Future<T>` as the return value of a method that starts an asynchronous operation. For example, we can use `IAssetAccessor` to asynchronously download the content of a web URL:

\snippet ExamplesAsyncSystem.cpp create-request-future

This `get` function will start the HTTP request, but it will return quickly, without waiting for the request to complete. Initially, the `Future` is in the _Pending_ state, meaning that the asynchronous operation has not yet completed. Once it does complete, the `Future` will be in one of these two states:

* _Resolved_: The operation has completed successfully and the result value is available.
* _Rejected_: The operation failed, and an exception describing the error is available.

If we want to block the current thread and wait for the request to complete, we can call `waitInMainThread`:

\snippet ExamplesAsyncSystem.cpp wait-in-main-thread

`waitInMainThread` blocks until the `Future` completes. If it is resolved, `waitInMainThread` returns its value. If it is rejected, `waitInMainThread` throws an exception representing the error.

> [!note]
> The `std::move` is needed because `Future<T>` is meant to be consumed just once. This allows for best efficiency because the value resulting from the asynchronous operation can be provided to the caller without ever making a copy of it.

In practice, though, we almost never call `waitInMainThread`. After all, what good is it to start an asynchronous operation if we're just going to block the calling thread waiting for it? Instead, we register a _continuation function_ with the `Future`.

## Continuations

A continuation function is a callback - usually a lambda - that is invoked when the `Future` resolves. The name comes from the idea that the execution "continues" with that function after the async operation. Continuations are registered with the `then...` methods on the `Future`. Different `then` functions are used to control _in what thread_ the callback is invoked:

* `thenInMainThread`: The continuation function is invoked in the "main" thread. Many applications have a clear idea of which thread they consider the main one, but in Cesium Native, "main thread" very specifically means the thread that called `waitInMainThread` or `dispatchMainThreadTasks` on the `AsyncSystem`. This need not be the same as the thread your application considers to be the main one.
* `thenInWorkerThread`: The continuation function is invoked in a background worker thread by calling `startTask` on the `ITaskProcessor` instance with which the `AsyncSystem` was constructed.
* `thenInThreadPool`: The continuation function is invoked in a specified \ref CesiumAsync::ThreadPool.
* `thenImmediately`: The continuation function is invoked immediately in whatever thread happened to resolve the `Future`. This is only appropriate for work that completes very quickly and is safe to run in any thread.

Continuing with our web request example above, we can register a continuation that receives the completed request and does something with the downloaded data in the main thread:

\snippet ExamplesAsyncSystem.cpp continuation

So far, this all looks like a glorified callback system. The real power of `AsyncSystem` becomes apparent when we chain continuations together. Here's what that might look like:

\snippet ExamplesAsyncSystem.cpp chaining

In this example, we call `get` to start a network request, as before. We immediately call `thenInWorkerThread` on the returned `Future` in order to register a continuation for when the request completes. This continuation runs in a background thread and calls `processDownloadedContent`. We can imagine that it's doing some intensive processing, and we wouldn't want to block the main thread while we're doing it.

The continuation lambda returns an instance of type `ProcessedContent`. Because the lambda function we pass to it returns `ProcessedContent`, the `thenInWorkerThread` call returns `Future<ProcessedContent>`. This new `Future` will resolve when the request completes _and then_ the `processDownloadedContent` also completes.

We can attach a further continuation to this new `Future`, which we do in the example by calling `thenInMainThread`. This second continuation will be invoked in the main thread once the `ProcessedContent` is asynchronously created. It calls `updateApplicationWithProcessedContent`, which we can imagine updates the user interface based on the downloaded and processed content.

