// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/Library.h"
#include <string>
#include <vector>
#include <memory>

#pragma warning(push)

#pragma warning(disable:4458 4324)

#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <async++.h>

#pragma warning(pop)

namespace CesiumAsync {
    class ITaskProcessor;

    template <class T>
    class Future;

    namespace Impl {
        // Begin omitting doxgen warnings for Impl namespace
        //! @cond Doxygen_Suppress

        struct AsyncSystemSchedulers {
            AsyncSystemSchedulers(
                std::shared_ptr<ITaskProcessor> pTaskProcessor
            );

            std::shared_ptr<ITaskProcessor> pTaskProcessor;
            async::fifo_scheduler mainThreadScheduler;

            void schedule(async::task_run_handle t);
        };

        template<typename T>
        struct RemoveFuture {
            typedef T type;
        };
        template<typename T>
        struct RemoveFuture<Future<T>> {
            typedef T type;
        };
        template<typename T>
        struct RemoveFuture<const Future<T>> {
            typedef T type;
        };
        template<typename T>
        struct RemoveFuture<async::task<T>> {
            typedef T type;
        };
        template<typename T>
        struct RemoveFuture<const async::task<T>> {
            typedef T type;
        };

        template <class Func>
        struct IdentityUnwrapper {
            static Func unwrap(Func&& f) {
                return std::forward<Func>(f);
            }
        };

        template <class Func, class T>
        struct ParameterizedTaskUnwrapper {
            static auto unwrap(Func&& f) {
                return [f = std::move(f)](T&& t) {
                    return f(std::forward<T>(t))._task;
                };
            }
        };

        template <class Func>
        struct TaskUnwrapper {
            static auto unwrap(Func&& f) {
                return [f = std::move(f)]() {
                    return f()._task;
                };
            }
        };

        template <class Func, class T>
        auto unwrapFuture(Func&& f) {
            return std::conditional<
                std::is_same<
                    typename std::invoke_result<Func, T>::type,
                    typename Impl::RemoveFuture<typename std::invoke_result<Func, T>::type>::type
                >::value,
                IdentityUnwrapper<Func>,
                ParameterizedTaskUnwrapper<Func, T>
            >::type::unwrap(std::forward<Func>(f));
        }

        template <class Func>
        auto unwrapFuture(Func&& f) {
            return std::conditional<
                std::is_same<
                    typename std::invoke_result<Func>::type,
                    typename Impl::RemoveFuture<typename std::invoke_result<Func>::type>::type
                >::value,
                IdentityUnwrapper<Func>,
                TaskUnwrapper<Func>
            >::type::unwrap(std::forward<Func>(f));
        }

        //! @endcond
        // End omitting doxgen warnings for Impl namespace
    }

    /**
     * @brief A value that will be available in the future, as produced by {@link AsyncSystem}.
     * 
     * @tparam T The type of the value.
     */
    template <class T>
    class Future final {
    public:

        /**
         * @brief Move constructor
         */
        Future(Future<T>&& rhs) noexcept :
            _pSchedulers(std::move(rhs._pSchedulers)),
            _task(std::move(rhs._task))
        {
        }

        Future(const Future<T>& rhs) = delete;
        Future<T>& operator=(const Future<T>& rhs) = delete;

        /**
         * @brief Registers a continuation function to be invoked in a worker thread when this Future resolves, and invalidates this Future.
         * 
         * If the function itself returns a `Future`, the function will not be considered complete
         * until that returned `Future` also resolves.
         * 
         * @tparam Func The type of the function.
         * @param f The function.
         * @return A future that resolves after the supplied function completes.
         */
        template <class Func>
        Future<typename Impl::RemoveFuture<typename std::invoke_result<Func, T>::type>::type> thenInWorkerThread(Func&& f) && {
            return Future<typename Impl::RemoveFuture<typename std::invoke_result<Func, T>::type>::type>(
                this->_pSchedulers,
                _task.then(*this->_pSchedulers, Impl::unwrapFuture<Func, T>(std::forward<Func>(f)))
            );
        }

        /**
         * @brief Registers a continuation function to be invoked in the main thread when this Future resolves, and invalidates this Future.
         * 
         * The supplied function will not be invoked immediately, even if this method is called from the main thread.
         * Instead, it will be queued and invoked the next time {@link AsyncSystem::dispatchMainThreadTasks} is called.
         * 
         * If the function itself returns a `Future`, the function will not be considered complete
         * until that returned `Future` also resolves.
         * 
         * @tparam Func The type of the function.
         * @param f The function.
         * @return A future that resolves after the supplied function completes.
         */
        template <class Func>
        Future<typename Impl::RemoveFuture<typename std::invoke_result<Func, T>::type>::type> thenInMainThread(Func&& f) && {
            return Future<typename Impl::RemoveFuture<typename std::invoke_result<Func, T>::type>::type>(
                this->_pSchedulers,
                _task.then(this->_pSchedulers->mainThreadScheduler, Impl::unwrapFuture<Func, T>(std::forward<Func>(f)))
            );
        }

        /**
         * @brief Registers a continuation function to be invoked in the main thread when this Future rejects, and invalidates this Future.
         * 
         * The supplied function will not be invoked immediately, even if this method is called from the main thread.
         * Instead, it will be queued and invoked the next time {@link AsyncSystem::dispatchMainThreadTasks} is called.
         * 
         * If the function itself returns a `Future`, the function will not be considered complete
         * until that returned `Future` also resolves.
         * 
         * Any `then` continuations chained after this one will be invoked with the return value of the catch callback.
         * 
         * @tparam Func The type of the function.
         * @param f The function.
         * @return A future that resolves after the supplied function completes.
         */
        template <class Func>
        Future<T> catchInMainThread(Func&& f) && {
            auto catcher = [f = std::move(f)](async::task<T>&& t) {
                try {
                    return t.get();
                } catch (std::exception& e) {
                    return f(e);
                } catch (...) {
                    return f(std::runtime_error("Unknown exception"));
                }
            };

            return Future<T>(
                this->_pSchedulers,
                _task.then(this->_pSchedulers->mainThreadScheduler, catcher)
            );
        }

    private:
        Future(std::shared_ptr<Impl::AsyncSystemSchedulers> pSchedulers, async::task<T>&& task) noexcept :
            _pSchedulers(pSchedulers),
            _task(std::move(task))
        {
        }

        std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;
        async::task<T> _task;

        friend class AsyncSystem;

        template <class Func, class R>
        friend struct Impl::ParameterizedTaskUnwrapper;

        template <class Func>
        friend struct Impl::TaskUnwrapper;

        template <class R>
        friend class Future;
    };

    /**
     * @brief A system for managing asynchronous requests and tasks.
     * 
     * Instances of this class may be safely and efficiently stored and passed around by value.
     */
    class CESIUMASYNC_API AsyncSystem final {
    public:
        /**
         * @brief Constructs a new instance.
         * 
         * @param pTaskProcessor The interface used to run tasks in background threads.
         */
        AsyncSystem(
            std::shared_ptr<ITaskProcessor> pTaskProcessor
        ) noexcept;

        template <class T, class Func>
        Future<T> createFuture(Func&& f) const {
            std::shared_ptr<async::event_task<T>> pEvent = std::make_shared<async::event_task<T>>();

            f([pEvent](T&& result) {
                pEvent->set(std::move(result));
            }, [pEvent](std::exception&& error) {
                pEvent->set_exception(std::make_exception_ptr(error));
            });

            return Future<T>(this->_pSchedulers, pEvent->get_task());
        }

        /**
         * @brief Runs a function in a worker thread, returning a promise that resolves when the function completes.
         * 
         * If the function itself returns a `Future`, the function will not be considered complete
         * until that returned `Future` also resolves.
         * 
         * @tparam Func The type of the function.
         * @param f The function.
         * @return A future that resolves after the supplied function completes.
         */
        template <class Func>
        Future<typename Impl::RemoveFuture<typename std::invoke_result<Func>::type>::type> runInWorkerThread(Func&& f) const {
            return Future<typename Impl::RemoveFuture<typename std::invoke_result<Func>::type>::type>(
                this->_pSchedulers,
                async::spawn(*this->_pSchedulers, Impl::unwrapFuture<Func>(std::forward<Func>(f)))
            );
        }

        /**
         * @brief Runs a function in the main thread, returning a promise that resolves when the function completes.
         * 
         * The supplied function will not be called immediately, even if this method is invoked from the main thread.
         * Instead, it will be queued and called the next time {@link dispatchMainThreadTasks} is called.
         * 
         * If the function itself returns a `Future`, the function will not be considered complete
         * until that returned `Future` also resolves.
         * 
         * @tparam Func The type of the function.
         * @param f The function.
         * @return A future that resolves after the supplied function completes.
         */
        template <class Func>
        Future<typename Impl::RemoveFuture<typename std::invoke_result<Func>::type>::type> runInMainThread(Func&& f) const {
            return Future<typename Impl::RemoveFuture<typename std::invoke_result<Func>::type>::type>(
                this->_pSchedulers,
                async::spawn(this->_pSchedulers->mainThreadScheduler, Impl::unwrapFuture<Func>(std::forward<Func>(f)))
            );
        }

        /**
         * @brief Creates a future that is already resolved.
         * 
         * @tparam T The type of the future
         * @param value The value for the future
         * @return The future
         */
        template <class T>
        Future<T> createResolvedFuture(T&& value) const {
            return Future<T>(this->_pSchedulers, async::make_task<T>(std::forward<T>(value)));
        }

        /**
         * @brief Runs all tasks that are currently queued for the main thread.
         * 
         * The tasks are run in the calling thread.
         */
        void dispatchMainThreadTasks();

    private:
        std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;

        template <class T>
        friend class Future;
    };
}
