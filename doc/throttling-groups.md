# Throttling Groups

## Motivation

`AsyncSystem::runInWorkerThread` (as the name implies) runs a task in a worker thread and returns a Future that resolves when it completes. If this work is CPU bound - as it usually is - it's not desirable to run too many such tasks simultaneously, because the overhead of task switching will become high and all of the tasks will complete slowly.

In practice, though, `runInWorkerThread` is dispatched via some sort of thread pool. In the case of Cesium for Unreal, these tasks are dispatched to Unreal Engine's task graph, which is similar. The thread pool limits the number of tasks that run simultaneously; any extra tasks are added to a queue. As each task completes, the next task in the queue is dispatched.

This scheme is strictly first in, first out (FIFO). Once `runInWorkerThread` (or similarly, `thenInWorkerThread`) is called, the task _will_ run eventually, and multiple tasks dispatched this way will start in the order in which these methods were called. There is no possibility of canceling or reprioritizing a task that hasn't started yet.

On top of this, not all tasks are CPU bound. Cesium Native also needs to do HTTP requests. Network bandwidth, like CPU time, is a limited resource; attempting to do a very large number of network requests simultaneously is inefficient. While thread pools allow `AsyncSystem` to use CPU time more or less efficiently, there is no similar mechanism for HTTP requests, GPU time, or any other type of limited resource.

Throttling groups are a feature of `AsyncSystem` that allows arbitrary tasks to be throttled and prioritized within a group. For example:

1. The 3D Tiles selection algorithm determines it needs a tile with priority P.
2. The first step in loading that tile is to do a network request for the tile payload. So a network request task is added to the "network request" throttling group.

  The "network request" throttling group is configured to allow up to 20 tasks (in this case, network requests) to run simultaneously. Tasks in a throttling group are executed in priority order. The priority can be changed after the task is created. For example, if a later run of the 3D Tiles selection algorithm decides that this tile is now a higher priority because the view changed. Tasks can also be removed (canceled) later. A task that is already running when it is canceled will be allowed to finish, but a task that is unstarted when it is canceled will never start.

3. Eventually, based on its priority, the network request task starts.
4. When the network request completes, a new task is added to the "CPU work" throttling group to parse the tile content from the downloaded request.

  The "CPU work" throttling group is just another instance of the `ThrottlingGroup` class, this time configured to allow something like `max(2, (number of CPUs - 2))` simultaneous tasks. Just as before, the tasks are prioritized and cancelable.

5. Parsing the tile content payload reveals a glTF with an external buffer, so that buffer must also be downloaded from the network. A new task is started in the "network request" throttling group to download that external buffer.

  When the CPU task completes, excluding any work that has been dispatched to another throttling group (how do we tell?), the throttling group is free to move on to the next highest priority task.

6. The new task in the "network request" throttling group eventually completes. Continuations attached to that Future are scheduled again in the "CPU work" throttling group. The task will begin executing when the group has the capacity and it is the highest priority work to be done.



A task in a throttling group starts when the Future returned by `beginThrottle` resolves. It ends (allowing the next task to run), when the future chain reaches the corresponding call to `endThrottle`. An intervening `beginThrottle` in the future chain will temporarily switch to a new throttling group, and resume the old one when `endThrottle` is reached.
