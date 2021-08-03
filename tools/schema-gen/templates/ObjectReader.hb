class CODEGEN_API {{reader}} : public {{readerBase.type}}
  {{~#if extension}}, public IExtensionJsonHandler {{/if}} {
public:
  using ValueType = {{scopedModel}};

  {{#if extension}}
  static inline constexpr const char* ExtensionName = "{{extension}}";
  {{/if}}

  {{#each subtypes}}
  {{> object}}
  {{/each}}

  {{#each enums}}
  {{> enum}}
  {{/each}}

  {{reader}}({{readerBase.params}});

  void reset(IJsonHandler* pParentHandler, {{scopedModel}}* pObject);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

  {{#if extension}}
  virtual void reset(IJsonHandler* pParentHandler, CesiumJsonReader::ExtensibleObject& o, const std::string_view& extensionName) override;

  virtual IJsonHandler* readNull() override {
    return {{readerBase.type}}::readNull();
  };
  virtual IJsonHandler* readBool(bool b) override {
    return {{readerBase.type}}::readBool(b);
  }
  virtual IJsonHandler* readInt32(int32_t i) override {
    return {{readerBase.type}}::readInt32(i);
  }
  virtual IJsonHandler* readUint32(uint32_t i) override {
    return {{readerBase.type}}::readUint32(i);
  }
  virtual IJsonHandler* readInt64(int64_t i) override {
    return {{readerBase.type}}::readInt64(i);
  }
  virtual IJsonHandler* readUint64(uint64_t i) override {
    return {{readerBase.type}}::readUint64(i);
  }
  virtual IJsonHandler* readDouble(double d) override {
    return {{readerBase.type}}::readDouble(d);
  }
  virtual IJsonHandler* readString(const std::string_view& str) override {
    return {{readerBase.type}}::readString(str);
  }
  virtual IJsonHandler* readObjectStart() override {
    return {{readerBase.type}}::readObjectStart();
  }
  virtual IJsonHandler* readObjectEnd() override {
    return {{readerBase.type}}::readObjectEnd();
  }
  virtual IJsonHandler* readArrayStart() override {
    return {{readerBase.type}}::readArrayStart();
  }
  virtual IJsonHandler* readArrayEnd() override {
    return {{readerBase.type}}::readArrayEnd();
  }
  virtual void reportWarning(const std::string& warning, std::vector<std::string>&& context = std::vector<std::string>()) override {
    {{readerBase.type}}::reportWarning(warning, std::move(context));
  }
  {{/if}}

protected:
  IJsonHandler* readObjectKey{{baseName}}(const std::string& objectType, const std::string_view& str, {{scopedModel}}& o);

private:
  {{scopedModel}}* _pObject = nullptr;

  {{#each properties}}
  {{readerMember}}
  {{/each}}
};

