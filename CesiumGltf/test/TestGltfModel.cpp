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

        virtual JsonHandler* Null() override { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Bool(bool /*b*/) override { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Int(int /*i*/) override { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Uint(unsigned /*i*/) override { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Int64(int64_t /*i*/) override { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Uint64(uint64_t /*i*/) override { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* Double(double /*d*/) override { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* RawNumber(const char* /*str*/, size_t /*length*/, bool /*copy*/) override { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* String(const char* /*str*/, size_t /*length*/, bool /*copy*/) override { return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* StartObject() override { ++this->_depth; return this; }
        virtual JsonHandler* Key(const char* /*str*/, size_t /*length*/, bool /*copy*/) override { return this; }
        virtual JsonHandler* EndObject(size_t /*memberCount*/) override { --this->_depth; return this->_depth == 0 ? this->parent() : this; }
        virtual JsonHandler* StartArray() override { ++this->_depth; return this; }
        virtual JsonHandler* EndArray(size_t /*elementCount*/) override { --this->_depth; return this->_depth == 0 ? this->parent() : this; }

    private:
        size_t _depth = 0;
    };

    class RejectAllJsonHandler : public JsonHandler {
    public:
        RejectAllJsonHandler() {
            reset(this);
        }
    };

    enum class ComponentType {
        BYTE = 5120,
        UNSIGNED_BYTE = 5121,
        SHORT = 5122,
        UNSIGNED_SHORT = 5123,
        UNSIGNED_INT = 5125,
        FLOAT = 5126
    };

    enum class AccessorType {
        SCALAR,
        VEC2,
        VEC3,
        VEC4,
        MAT2,
        MAT3,
        MAT4
    };

    struct Accessor {
        size_t bufferView;
        size_t byteOffset = 0;
        ComponentType componentType;
        bool normalized = false;
        size_t count;
        AccessorType type;
        std::vector<double> max;
        std::vector<double> min;
    };

    enum class PrimitiveMode {
        POINTS = 0,
        LINES = 1,
        LINE_LOOP = 2,
        LINE_STRIP = 3,
        TRIANGLES = 4,
        TRIANGLE_STRIP = 5,
        TRIANGLE_FAN = 6
    };

    struct Primitive {
        std::unordered_map<std::string, size_t> attributes;
        size_t indices;
        size_t material;
        PrimitiveMode mode = PrimitiveMode::TRIANGLES;
        std::vector<std::unordered_map<std::string, size_t>> targets;
    };

    struct Mesh {
        std::vector<Primitive> primitives;
        std::vector<double> weights;
    };

    struct Model {
        std::vector<Accessor> accessors;
        std::vector<Mesh> meshes;
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

    class BoolJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, bool* pBool) {
            JsonHandler::reset(pParent);
            this->_pBool = pBool;
        }

        virtual JsonHandler* Bool(bool b) override {
            assert(this->_pBool);
            *this->_pBool = b;
            return this->parent();
        }

    private:
        bool* _pBool = nullptr;
    };

    class AccessorTypeJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, AccessorType* pEnum) {
            JsonHandler::reset(pParent);
            this->_pEnum = pEnum;
        }

        virtual JsonHandler* String(const char* str, size_t /*length*/, bool /*copy*/) override {
            using namespace std::string_literals;

            if ("SCALAR"s == str) *this->_pEnum = AccessorType::SCALAR;
            else if ("VEC2"s == str) *this->_pEnum = AccessorType::VEC2;
            else if ("VEC3"s == str) *this->_pEnum = AccessorType::VEC3;
            else if ("VEC4"s == str) *this->_pEnum = AccessorType::VEC4;
            else if ("MAT2"s == str) *this->_pEnum = AccessorType::MAT2;
            else if ("MAT3"s == str) *this->_pEnum = AccessorType::MAT3;
            else if ("MAT4"s == str) *this->_pEnum = AccessorType::MAT4;
            else return nullptr;

            return this->parent();
        }

    private:
        AccessorType* _pEnum = nullptr;
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

    class DoubleArrayJsonHandler : public JsonHandler {
    public:
        void reset(JsonHandler* pParent, std::vector<double>* pArray) {
            JsonHandler::reset(pParent);
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
            return this->parent();
        }

        virtual JsonHandler* Double(double d) override {
            if (!this->_arrayIsOpen) {
                return nullptr;
            }

            assert(this->_pArray);
            this->_pArray->emplace_back(d);
            return this;
        }

    private:
        std::vector<double>* _pArray = nullptr;
        bool _arrayIsOpen = false;
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

    class AccessorJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Accessor* pAccessor) {
            JsonHandler::reset(pParent);
            this->_pAccessor = pAccessor;
        }

        virtual JsonHandler* Key(const char* str, size_t /*length*/, bool /*copy*/) override {
            using namespace std::string_literals;

            assert(this->_pAccessor);
            if ("bufferView"s == str) return property(this->_bufferView, this->_pAccessor->bufferView);
            if ("byteOffset"s == str) return property(this->_byteOffset, this->_pAccessor->byteOffset);
            if ("componentType"s == str) return property(this->_componentType, this->_pAccessor->componentType);
            if ("normalized"s == str) return property(this->_normalized, this->_pAccessor->normalized);
            if ("count"s == str) return property(this->_count, this->_pAccessor->count);
            if ("type"s == str) return property(this->_type, this->_pAccessor->type);
            if ("max"s == str) return property(this->_max, this->_pAccessor->max);
            if ("min"s == str) return property(this->_min, this->_pAccessor->min);

            return this->ignore();
        }

    private:
        Accessor* _pAccessor = nullptr;
        IntegerJsonHandler<size_t> _bufferView;
        IntegerJsonHandler<size_t> _byteOffset;
        IntegerJsonHandler<ComponentType> _componentType;
        BoolJsonHandler _normalized;
        IntegerJsonHandler<size_t> _count;
        AccessorTypeJsonHandler _type;
        DoubleArrayJsonHandler _max;
        DoubleArrayJsonHandler _min;
    };

    class PrimitiveJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Primitive* pPrimitive) {
            JsonHandler::reset(pParent);
            this->_pPrimitive = pPrimitive;
        }

        virtual JsonHandler* Key(const char* str, size_t /*length*/, bool /*copy*/) override {
            using namespace std::string_literals;

            assert(this->_pPrimitive);
            if ("indices"s == str) return property(this->_indices, this->_pPrimitive->indices);
            if ("material"s == str) return property(this->_material, this->_pPrimitive->material);
            if ("mode"s == str) return property(this->_mode, this->_pPrimitive->mode);

            return this->ignore();
        }

    private:
        Primitive* _pPrimitive = nullptr;
        // std::unordered_map<std::string, size_t> attributes;
        IntegerJsonHandler<size_t> _indices;
        IntegerJsonHandler<size_t> _material;
        IntegerJsonHandler<PrimitiveMode> _mode;
        // std::vector<std::unordered_map<std::string, size_t>> targets;
    };

    class MeshJsonHandler : public ObjectJsonHandler {
    public:
        void reset(JsonHandler* pParent, Mesh* pMesh) {
            JsonHandler::reset(pParent);
            this->_pMesh = pMesh;
        }

        virtual JsonHandler* Key(const char* str, size_t /*length*/, bool /*copy*/) override {
            using namespace std::string_literals;

            assert(this->_pMesh);
            if ("primitives"s == str) return property(this->_primitives, this->_pMesh->primitives);
            if ("weights"s == str) return property(this->_weights, this->_pMesh->weights);

            return this->ignore();
        }

    private:
        Mesh* _pMesh = nullptr;
        ObjectArrayJsonHandler<Primitive, PrimitiveJsonHandler> _primitives;
        DoubleArrayJsonHandler _weights;
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
            if ("meshes"s == str) return property(this->_meshes, this->_pModel->meshes);

            return this->ignore();
        }

    private:
        Model* _pModel;
        ObjectArrayJsonHandler<Accessor, AccessorJsonHandler> _accessors;
        ObjectArrayJsonHandler<Mesh, MeshJsonHandler> _meshes;
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

            bool success = true;
            while (success && !reader.IterativeParseComplete()) {
                success = reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(inputStream, dispatcher);
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
    std::string s = "{\"accessors\": [{\"count\": 4,\"componentType\":5121,\"type\":\"VEC2\",\"max\":[1.0, 2.2, 3.3],\"min\":[0.0, -1.2]}],\"surprise\":{\"foo\":true}}";
    GltfReader reader;
    
    Model model = reader.parse(gsl::span(reinterpret_cast<const uint8_t*>(s.c_str()), s.size()));
    REQUIRE(model.accessors.size() == 1);
    CHECK(model.accessors[0].count == 4);
    CHECK(model.accessors[0].componentType == ComponentType::UNSIGNED_BYTE);
    CHECK(model.accessors[0].type == AccessorType::VEC2);
    REQUIRE(model.accessors[0].min.size() == 2);
    CHECK(model.accessors[0].min[0] == 0.0);
    CHECK(model.accessors[0].min[1] == -1.2);
    REQUIRE(model.accessors[0].max.size() == 3);
    CHECK(model.accessors[0].max[0] == 1.0);
    CHECK(model.accessors[0].max[1] == 2.2);
    CHECK(model.accessors[0].max[2] == 3.3);
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