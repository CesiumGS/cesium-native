#pragma once

namespace tinygltf {
    struct Accessor;
}

namespace CesiumGltf {
    template <typename T>
    class GltfCollection;

    class GltfAccessor;

    #define CESIUM_GLTF_CONST_WRAPPER_DECL(Type, WrappedType) \
        public: \
            Type##Const(); \
            Type##Const(const Type##Const& rhs); \
            Type##Const(Type##Const&& rhs) = delete; \
            ~Type##Const(); \
            Type##Const& operator=(const Type##Const& rhs) = delete; \
            Type##Const& operator=(Type##Const&& rhs) = delete; \
        private: \
            using Tiny = WrappedType; \
            Type##Const(const WrappedType* p); \
            const WrappedType* _p; \
            bool _owns; \
            template <typename TElement> \
            friend class GltfCollection; \
            friend class GltfAccessor;

    #define CESIUM_GLTF_MUTABLE_WRAPPER_DECL(Type, WrappedType) \
        public: \
            Type(); \
            Type(const Type& rhs); \
            Type(Type&& rhs); \
            Type(const Type##Const& rhs); \
            Type(Type##Const&& rhs); \
            Type& operator=(const Type& rhs); \
            Type& operator=(Type&& rhs); \
            Type& operator=(const Type##Const& rhs); \
        private: \
            using Const = Type##Const; \
            Type(WrappedType* p); \
            Tiny*& tiny() { return const_cast<WrappedType*&>(this->_p); }

    class GltfAccessorConst {
    public:
        size_t getCount() const;

        CESIUM_GLTF_CONST_WRAPPER_DECL(GltfAccessor, tinygltf::Accessor);
    };

    class GltfAccessor final : public GltfAccessorConst {
    public:
        void setCount(size_t value);

        CESIUM_GLTF_MUTABLE_WRAPPER_DECL(GltfAccessor, tinygltf::Accessor);        
    };
}
