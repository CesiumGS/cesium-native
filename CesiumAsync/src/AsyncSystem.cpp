#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/ITaskProcessor.h"
#include <future>

namespace CesiumAsync {

    AsyncSystem::AsyncSystem(
        std::shared_ptr<IAssetAccessor> pAssetAccessor,
        std::shared_ptr<ITaskProcessor> pTaskProcessor
    ) noexcept :
        _pSchedulers(std::make_shared<Impl::AsyncSystemSchedulers>(pAssetAccessor, pTaskProcessor))
    {
    }

    Future<std::unique_ptr<IAssetRequest>> AsyncSystem::requestAsset(
        const std::string& url,
        const std::vector<IAssetAccessor::THeader>& headers
    ) const {
        std::shared_ptr<async::event_task<std::unique_ptr<IAssetRequest>>> pEvent = std::make_shared<async::event_task<std::unique_ptr<IAssetRequest>>>();
        this->_pSchedulers->pAssetAccessor->requestAsset(this, 
            url, 
            headers, 
            [pEvent](std::unique_ptr<IAssetRequest> pRequest) {
				pEvent->set(std::move(pRequest));
            });

        Future<std::unique_ptr<IAssetRequest>> result(this->_pSchedulers, pEvent->get_task());
        return result;
    }

    void AsyncSystem::dispatchMainThreadTasks() {
        this->_pSchedulers->mainThreadScheduler.run_all_tasks();
    }

    namespace Impl {
        AsyncSystemSchedulers::AsyncSystemSchedulers(
            std::shared_ptr<IAssetAccessor> pAssetAccessor_,
            std::shared_ptr<ITaskProcessor> pTaskProcessor_
        ) :
            pAssetAccessor(pAssetAccessor_),
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
