#include "catch2/catch.hpp"
#include "CesiumGltf/GltfModel.h"
#include <rapidjson/reader.h>
#include <gsl/span>
#include <string>

using namespace CesiumGltf;

namespace CesiumGltf {
    template <class T>
    void goParse(T&& handler) {
        while (!handler.reader.IterativeParseComplete() && !handler.done) {
            if (handler.next) {
                handler.next();
                handler.next = nullptr;
            } else {
                handler.reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(handler.inputStream, handler);
            }
        }
    }

    struct DefaultHandler {
        DefaultHandler(rapidjson::Reader& reader_, rapidjson::MemoryStream& inputStream_) :
            reader(reader_),
            inputStream(inputStream_),
            done(false)
        {}

        rapidjson::Reader& reader;
        rapidjson::MemoryStream& inputStream;
        std::function<void ()> next;
        bool done;

        bool Null() { return false; }
        bool Bool(bool /*b*/) { return false; }
        bool Int(int /*i*/) { return false; }
        bool Uint(unsigned /*i*/) { return false; }
        bool Int64(int64_t /*i*/) { return false; }
        bool Uint64(uint64_t /*i*/) { return false; }
        bool Double(double /*d*/) { return false; }
        bool RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/) { return false; }
        bool String(const char* /*str*/, size_t /*length*/, bool /*copy*/) { return false; }
        bool StartObject() { return false; }
        bool Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) { return false; }
        bool EndObject(size_t /*memberCount*/) { return false; }
        bool StartArray() { return false; }
        bool EndArray(size_t /*elementCount*/) { return false; }
    };

    struct ObjectHandler : public DefaultHandler {
        ObjectHandler(
            rapidjson::Reader& reader_,
            rapidjson::MemoryStream& inputStream_
        ) :
            DefaultHandler(reader_, inputStream_)
        {}

        bool StartObject() { return true; }
        bool EndObject(size_t) {
            this->done = true;
            return true;
        }
    };

    template <typename T, typename THandler>
    struct ObjectArrayHandler : public DefaultHandler {
        ObjectArrayHandler(
            rapidjson::Reader& reader_,
            rapidjson::MemoryStream& inputStream_,
            std::vector<T>& array_
        ) :
            DefaultHandler(reader_, inputStream_),
            array(array_),
            _arrayOpen(false)
        {}

        std::vector<T>& array;
        bool _arrayOpen;

        bool StartArray() {
            if (this->_arrayOpen) {
                return false;
            }

            this->_arrayOpen = true;
            return true;
        }

        bool StartObject() {
            if (!this->_arrayOpen) {
                return false;
            }

            T& o = this->array.emplace_back();
            THandler handler(this->reader, this->inputStream, o);
            bool result = handler.StartObject();

            if (result) {
                next = [handler = std::move(handler)]() mutable {
                    goParse(handler);
                };
            }

            return result;
        }

        bool EndArray(size_t /*elementCount*/) {
            if (!this->_arrayOpen) {
                return false;
            }

            this->_arrayOpen = false;
            this->done = true;
            return true;
        }
    };

    struct Accessor {
        size_t count;
    };

    struct Model {
        std::vector<Accessor> accessors;
    };

    template <typename T>
    struct IntegerHandler : public DefaultHandler {
        IntegerHandler(
            rapidjson::Reader& reader_,
            rapidjson::MemoryStream& inputStream_,
            T& value_
        ) :
            DefaultHandler(reader_, inputStream_),
            value(value_)
        {}

        T& value;

        bool Int(int i) {
            this->value = static_cast<T>(i);
            this->done = true;
            return true;
        }
        bool Uint(unsigned i) {
            this->value = static_cast<T>(i);
            this->done = true;
            return true;
        }
        bool Int64(int64_t i) {
            this->value = static_cast<T>(i);
            this->done = true;
            return true;
        }
        bool Uint64(uint64_t i) {
            this->value = static_cast<T>(i);
            this->done = true;
            return true;
        }
    };

    struct AccessorHandler : public ObjectHandler {
        AccessorHandler(
            rapidjson::Reader& reader_,
            rapidjson::MemoryStream& inputStream_,
            Accessor& accessor_
        ) :
            ObjectHandler(reader_, inputStream_),
            accessor(accessor_)
        {}

        Accessor& accessor;

        bool Key(const char* str, size_t /*length*/, bool /*copy*/) {
            using namespace std::string_literals;

            if ("count"s == str) {
                next = [this]() {
                    goParse(IntegerHandler<size_t>(this->reader, this->inputStream, this->accessor.count));
                };
            }

            return true;
        }
    };

    struct ModelHandler : public ObjectHandler {
        ModelHandler(
            rapidjson::Reader& reader_,
            rapidjson::MemoryStream& inputStream_,
            Model& model_
        ) :
            ObjectHandler(reader_, inputStream_),
            model(model_)
        {}

        Model& model;

        bool Key(const char* str, size_t /*length*/, bool /*copy*/) {
            using namespace std::string_literals;

            if ("accessors"s == str) {
                next = [this]() { goParse(ObjectArrayHandler<Accessor, AccessorHandler>(this->reader, this->inputStream, this->model.accessors)); };

            }
            return true;
        }
    };

    Model parseModel(const gsl::span<const uint8_t>& data) {
        rapidjson::Reader reader;
        rapidjson::MemoryStream inputStream(reinterpret_cast<const char*>(data.data()), data.size());

        reader.IterativeParseInit();

        Model model;
        ModelHandler handler(reader, inputStream, model);
        goParse(handler);

        return model;
    }
}

TEST_CASE("GltfModel") {
    std::string s = "{\"accessors\": [{\"count\": 4}]}";
    Model model = CesiumGltf::parseModel(gsl::span(reinterpret_cast<const uint8_t*>(s.c_str()), s.size()));
    CHECK(model.accessors.size() == 1);
    CHECK(model.accessors[0].count == 4);
    // std::vector<uint8_t> v;
    // GltfModel model = GltfModel::fromMemory(v);
    // GltfCollection<GltfMesh> meshes = model.meshes();
    // for (const GltfMesh& mesh : meshes) {
    //     mesh;
    // }

    // for (const std::string& s : model.extensionsUsed()) {
    //     s;
    // }
}