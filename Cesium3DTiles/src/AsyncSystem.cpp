#include "Cesium3DTiles/AsyncSystem.h"
#include "Cesium3DTiles/ITaskProcessor.h"
#include <future>

namespace Cesium3DTiles {

    AsyncSystem::AsyncSystem(
        std::shared_ptr<IAssetAccessor> pAssetAccessor,
        std::shared_ptr<ITaskProcessor> pTaskProcessor
    ) :
        _pSchedulers(std::make_shared<Impl::AsyncSystemSchedulers>(pAssetAccessor, pTaskProcessor))
    {
    }

    Future<std::unique_ptr<IAssetRequest>> AsyncSystem::requestAsset(
        const std::string& url,
        const std::vector<IAssetAccessor::THeader>& headers
    ) const {
        struct Receiver {
            std::unique_ptr<IAssetRequest> pRequest;
            async::event_task<std::unique_ptr<IAssetRequest>> event;
        };

        std::shared_ptr<Receiver> pReceiver = std::make_unique<Receiver>();
        pReceiver->pRequest = this->_pSchedulers->pAssetAccessor->requestAsset(url, headers);

        Future<std::unique_ptr<IAssetRequest>> result(this->_pSchedulers, pReceiver->event.get_task());

        pReceiver->pRequest->bind([pReceiver](IAssetRequest*) {
            // With the unique_ptr in the Receiver, the request and the receiver
            // circularly reference each other, so neither can ever be destroyed.
            // So here we move the unique_ptr to the request into a local so that
            // when it goes out of scope, both can be destroyed.
            std::unique_ptr<IAssetRequest> pRequest = std::move(pReceiver->pRequest);
            pReceiver->event.set(std::move(pRequest));
        });

        return result;
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
