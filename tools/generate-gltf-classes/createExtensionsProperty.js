const unindent = require("./unindent");

function createExtensionsProperty(extensions, name, schema) {
    if (!extensions) {
        return undefined;
    }

    return {
        name: "extensions",
        headers: [],
        readerHeaders: extensions.map(extension => `"${extension.className}JsonHandler.h"`),
        readerHeadersImpl: extensions.map(extension => `"CesiumGltf/${extension.className}.h"`),
        type: undefined,
        readerType: "ExtensionsJsonHandler",
        schemas: [],
        localTypes: [],
        readerLocalTypes: [createExtensionType(extensions)],
        readerLocalTypesImpl: [createExtensionTypeImpl(name, extensions)],
        briefDoc: undefined,
        fullDoc: undefined
    };
}

function createExtensionType(extensions) {
    return unindent(`
        class ExtensionsJsonHandler : public ObjectJsonHandler {
        public:
          void reset(IJsonHandler* pParent, std::vector<std::any>* pExtensions);
          virtual IJsonHandler* Key(const char* str, size_t length, bool copy) override;
        
        private:
          std::vector<std::any>* _pExtensions = nullptr;
          ${extensions.map(extension => `${extension.className}JsonHandler _${extension.name};`).join("\n")}
        };
    `);
    return "Extensions!!";
}

function createExtensionTypeImpl(name, extensions) {
    return unindent(`
        void ${name}JsonHandler::ExtensionsJsonHandler::reset(IJsonHandler* pParent, std::vector<std::any>* pExtensions) {
            ObjectJsonHandler::reset(pParent);
            this->_pExtensions = pExtensions;
        }
        
        IJsonHandler* ${name}JsonHandler::ExtensionsJsonHandler::Key(const char* str, size_t /* length */, bool /* copy */) {
            using namespace std::string_literals;
        
            ${extensions.map(extension => `if ("${extension.name}"s == str) return property("${extension.name}", this->_${extension.name}, std::any_cast<${extension.className}&>(this->_pExtensions->emplace_back(${extension.className}())));`).join("\n")}
        
            return this->ignoreAndContinue();
        }
    `);
}

module.exports = createExtensionsProperty;
