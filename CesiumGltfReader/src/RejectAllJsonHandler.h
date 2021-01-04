#pragma once

namespace CesiumGltf {
    class RejectAllJsonHandler : public JsonHandler {
    public:
        RejectAllJsonHandler() {
            reset(this);
        }
    };
}
