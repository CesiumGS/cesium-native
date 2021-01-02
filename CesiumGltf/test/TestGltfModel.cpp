#include "catch2/catch.hpp"
#include "CesiumGltf/GltfModel.h"
#include <rapidjson/reader.h>
#include <gsl/span>
#include <string>

using namespace CesiumGltf;

namespace CesiumGltf {
    class JsonHandler {
    public:
        virtual JsonHandler* Null() { return nullptr; }
        virtual JsonHandler* Bool(bool /*b*/) { return nullptr; }
        virtual JsonHandler* Int(int /*i*/) { return nullptr; }
        virtual JsonHandler* Uint(unsigned /*i*/) { return nullptr; }
        virtual JsonHandler* Int64(int64_t /*i*/) { return nullptr; }
        virtual JsonHandler* Uint64(uint64_t /*i*/) { return nullptr; }
        virtual JsonHandler* Double(double /*d*/) { return nullptr; }
        virtual JsonHandler* RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/) { return nullptr; }
        virtual JsonHandler* String(const char* /*str*/, size_t /*length*/, bool /*copy*/) { return nullptr; }
        virtual JsonHandler* StartObject() { return nullptr; }
        virtual JsonHandler* Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) { return nullptr; }
        virtual JsonHandler* EndObject(size_t /*memberCount*/) { return nullptr; }
        virtual JsonHandler* StartArray() { return nullptr; }
        virtual JsonHandler* EndArray(size_t /*elementCount*/) { return nullptr; }

        template <typename TAccessor, typename TProperty>
        JsonHandler* property(TAccessor& accessor, TProperty& value) {
            accessor.reset(this, &value);
            return &accessor;
        }

        JsonHandler* parent() {
            return this->_pParent;
        }

    protected:
        void reset(JsonHandler* pParent) {
            this->_pParent = pParent;
        }

    private:
        JsonHandler* _pParent = nullptr;
    };

    class IgnoreValueJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent) {
            JsonHandler::reset(pParent);
            this->_depth = 0;
        }

        virtual JsonHandler* Null() { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Bool(bool /*b*/) { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Int(int /*i*/) { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Uint(unsigned /*i*/) { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Int64(int64_t /*i*/) { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Uint64(uint64_t /*i*/) { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Double(double /*d*/) { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/) { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* String(const char* /*str*/, size_t /*length*/, bool /*copy*/) { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* StartObject() { ++this->_depth; return this; }
        virtual JsonHandler* Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) { return this; }
        virtual JsonHandler* EndObject(size_t /*memberCount*/) { --this->_depth; return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* StartArray() { ++this->_depth; return this; }
        virtual JsonHandler* EndArray(size_t /*elementCount*/) { --this->_depth; return this->_depth == 0 ? this->parent() : this; }

    private:
        size_t _depth = 0;
    };

    class RejectAllJsonHandler : public JsonHandler {
    public:
        RejectAllJsonHandler() {
            reset(this);
        }
    };

    struct Accessor {
        size_t count;
    };

    struct Model {
        std::vector<Accessor> accessors;
    };

    template <typename T>
    class IntegerJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, T* pInteger) {
            JsonHandler::reset(pParent);
            this->_pInteger = pInteger;
        }

        virtual JsonHandler* Int(int i) override {
            assert(this->_pInteger);
            *this->_pInteger = static_cast<T>(i);
            return this->parent();
        }
        virtual JsonHandler* Uint(unsigned i) override {
            assert(this->_pInteger);
            *this->_pInteger = static_cast<T>(i);
            return this->parent();
        }
        virtual JsonHandler* Int64(int64_t i) override {
            assert(this->_pInteger);
            *this->_pInteger = static_cast<T>(i);
            return this->parent();
        }
        virtual JsonHandler* Uint64(uint64_t i) override {
            assert(this->_pInteger);
            *this->_pInteger = static_cast<T>(i);
            return this->parent();
        }

    private:
        T* _pInteger = nullptr;
    };

    class ObjectJsonHandler : public JsonHandler {
    public:
        virtual JsonHandler* StartObject() override {
            return this;
        }

        virtual JsonHandler* EndObject(size_t /*memberCount*/) override {
            return this->parent();
        }

        JsonHandler* ignore() {
            this->_ignoreHandler.reset(this);
            return &this->_ignoreHandler;
        }

    private:
        IgnoreValueJsonHandler _ignoreHandler;
    };

    class AccessorJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Accessor* pAccessor) {
            JsonHandler::reset(pParent);
            this->_pAccessor = pAccessor;
        }

        virtual JsonHandler* Key(const char* str, size_t /*length*/, bool /*copy*/) override {
            using namespace std::string_literals;

            assert(this->_pAccessor);
            if ("count"s == str) return property(this->_count, this->_pAccessor->count);

            return this->ignore();
        }

    private:
        Accessor* _pAccessor = nullptr;
        IntegerJsonHandler<size_t> _count;
    };

    template <typename T, typename THandler>
    class ObjectArrayJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, std::vector<T>* pArray) {
            this->_pParent = pParent;
            this->_pArray = pArray;
            this->_arrayIsOpen = false;
        }

        virtual JsonHandler* StartArray() override {
            if (this->_arrayIsOpen) {
                return nullptr;
            }

            this->_arrayIsOpen = true;
            return this;
        }

        virtual JsonHandler* EndArray(size_t) override {
            return this->_pParent;
        }

        virtual JsonHandler* StartObject() override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            T& o = this->_pArray->emplace_back();
            this->_objectHandler.reset(this, &o);
            return this->_objectHandler.StartObject();
        }

    private:
        JsonHandler* _pParent = nullptr;
        std::vector<T>* _pArray = nullptr;
        bool _arrayIsOpen = false;
        THandler _objectHandler;
    };

    class ModelJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Model* pModel) {
            ObjectJsonHandler::reset(pParent);
            this->_pModel = pModel;
        }

        virtual JsonHandler* Key(const char* str, size_t /*length*/, bool /*copy*/) override {
            using namespace std::string_literals;

            assert(this->_pModel);
            if ("accessors"s == str) return property(this->_accessors, this->_pModel->accessors);

            return this->ignore();
        }

    private:
        Model* _pModel;
        ObjectArrayJsonHandler<Accessor, AccessorJsonHandler> _accessors;
    };

    class GltfReader final {
    public:
        Model parse(const gsl::span<const uint8_t>& data) {
            rapidjson::Reader reader;
            rapidjson::MemoryStream inputStream(reinterpret_cast<const char*>(data.data()), data.size());

            Model result;
            ModelJsonHandler modelHandler;
            RejectAllJsonHandler rejectHandler;
            Dispatcher dispatcher { &modelHandler };

            modelHandler.reset(&rejectHandler, &result);

            reader.IterativeParseInit();

            while (!reader.IterativeParseComplete()) {
                reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(inputStream, dispatcher);
            }

            return result;
        }

    private:
        struct Dispatcher {
            JsonHandler* pCurrent;

            bool update(JsonHandler* pNext) {
                if (pNext == nullptr) {
                    return false;
                }
                
                this->pCurrent = pNext;
                return true;
            }

            bool Null() { return update(pCurrent->Null()); }
            bool Bool(bool b) { return update(pCurrent->Bool(b)); }
            bool Int(int i) { return update(pCurrent->Int(i)); }
            bool Uint(unsigned i) { return update(pCurrent->Uint(i)); }
            bool Int64(int64_t i) { return update(pCurrent->Int64(i)); }
            bool Uint64(uint64_t i) { return update(pCurrent->Uint64(i)); }
            bool Double(double d) { return update(pCurrent->Double(d)); }
            bool RawNumber(const char* str, size_t length, bool copy) { return update(pCurrent->RawNumber(str, length, copy)); }
            bool String(const char* str, size_t length, bool copy) { return update(pCurrent->String(str, length, copy)); }
            bool StartObject() { return update(pCurrent->StartObject()); }
            bool Key(const char* str, size_t length, bool copy) { return update(pCurrent->Key(str, length, copy)); }
            bool EndObject(size_t memberCount) { return update(pCurrent->EndObject(memberCount)); }
            bool StartArray() { return update(pCurrent->StartArray()); }
            bool EndArray(size_t elementCount) { return update(pCurrent->EndArray(elementCount)); }
        };
    };

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
    GltfReader reader;
    
    Model model = reader.parse(gsl::span(reinterpret_cast<const uint8_t*>(s.c_str()), s.size()));
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