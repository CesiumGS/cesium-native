#pragma once

#include "Cesium3DTiles/IAssetAccessor.h"
#include <string>
#include <vector>
#include <memory>

#pragma warning(push)
#pragma warning(disable:4458 4324)
#include <async++.h>
#pragma warning(pop)

namespace Cesium3DTiles {
    class IAssetAccessor;
    class ITaskProcessor;

    template <class T>
    class Future;

    namespace Impl {
        struct AsyncSystemSchedulers {
            AsyncSystemSchedulers(
                std::shared_ptr<IAssetAccessor> pAssetAccessor,
                std::shared_ptr<ITaskProcessor> pTaskProcessor
            );

            std::shared_ptr<IAssetAccessor> pAssetAccessor;
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
    }

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

    template <class T>
    class Future {
    public:
        Future() :
            _pSchedulers(),
            _task()
        {
        }

        Future(Future<T>&& rhs) :
            _pSchedulers(std::move(rhs._pSchedulers)),
            _task(std::move(rhs._task))
        {
        }

        Future(std::shared_ptr<Impl::AsyncSystemSchedulers> pSchedulers, async::task<T>&& task) :
            _pSchedulers(pSchedulers),
            _task(std::move(task))
        {
        }

        template <class Func>
        Future<typename Impl::RemoveFuture<typename std::invoke_result<Func, T>::type>::type> thenInWorkerThread(Func&& f) {
            return Future<typename Impl::RemoveFuture<typename std::invoke_result<Func, T>::type>::type>(
                this->_pSchedulers,
                _task.then(*this->_pSchedulers, unwrapFuture<Func, T>(std::forward<Func>(f)))
            );
        }

        template <class Func>
        Future<typename Impl::RemoveFuture<typename std::invoke_result<Func, T>::type>::type> thenInMainThread(Func&& f) {
            return Future<typename Impl::RemoveFuture<typename std::invoke_result<Func, T>::type>::type>(
                this->_pSchedulers,
                _task.then(this->_pSchedulers->mainThreadScheduler, unwrapFuture<Func, T>(std::forward<Func>(f)))
            );
        }

    private:
        std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;
        async::task<T> _task;

        friend class AsyncSystem;
        template <class Func, class T>
        friend struct ParameterizedTaskUnwrapper;
        template <class Func>
        friend struct TaskUnwrapper;
    };

    class CESIUM3DTILES_API AsyncSystem {
    public:
        AsyncSystem(
            std::shared_ptr<IAssetAccessor> pAssetAccessor,
            std::shared_ptr<ITaskProcessor> pTaskProcessor
        );

        Future<std::unique_ptr<IAssetRequest>> requestAsset(
            const std::string& url,
            const std::vector<IAssetAccessor::THeader>& headers = std::vector<IAssetAccessor::THeader>()
        ) const;

        template <class Func>
        Future<typename Impl::RemoveFuture<typename std::invoke_result<Func>::type>::type> runInWorkerThread(Func&& f) const {
            return Future<typename Impl::RemoveFuture<typename std::invoke_result<Func>::type>::type>(
                this->_pSchedulers,
                async::spawn(this->_pSchedulers->workerThreadScheduler, unwrapFuture<Func>(std::forward<Func>(f)))
            );
        }

        template <class Func>
        Future<typename Impl::RemoveFuture<typename std::invoke_result<Func>::type>::type> runInMainThread(Func&& f) const {
            return Future<typename Impl::RemoveFuture<typename std::invoke_result<Func>::type>::type>(
                this->_pSchedulers,
                async::spawn(this->_pSchedulers->mainThreadScheduler, unwrapFuture<Func>(std::forward<Func>(f)))
            );
        }

        void runMainThreadTasks() {
            this->_pSchedulers->mainThreadScheduler.run_all_tasks();
        }

    private:
        std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;

        template <class T>
        friend class Future;
    };
}
