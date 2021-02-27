// Copyright CesiumGS, Inc. and Contributors

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/ITaskProcessor.h"
#include <future>

namespace CesiumAsync {

    AsyncSystem::AsyncSystem(
        std::shared_ptr<ITaskProcessor> pTaskProcessor
    ) noexcept :
        _pSchedulers(std::make_shared<Impl::AsyncSystemSchedulers>(pTaskProcessor))
    {
    }

    void AsyncSystem::dispatchMainThreadTasks() {
        this->_pSchedulers->mainThreadScheduler.run_all_tasks();
    }

    namespace Impl {
        AsyncSystemSchedulers::AsyncSystemSchedulers(
            std::shared_ptr<ITaskProcessor> pTaskProcessor_
        ) :
            pTaskProcessor(pTaskProcessor_),
            mainThreadScheduler()
        {
        }

        void AsyncSystemSchedulers::schedule(async::task_run_handle t) {
            struct Receiver {
                async::task_run_handle taskHandle;
            };

            std::shared_ptr<Receiver> pReceiver = std::make_shared<Receiver>();
            pReceiver->taskHandle = std::move(t);
            
            this->pTaskProcessor->startTask([pReceiver]() mutable {
                pReceiver->taskHandle.run();
            });
        }
    }

}
