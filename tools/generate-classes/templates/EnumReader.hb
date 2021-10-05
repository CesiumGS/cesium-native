class {{reader}} : public JsonHandler {
public:
  void reset(IJsonHandler* pParent, {{scopedModel}}* pEnum);
  virtual IJsonHandler* readString(const std::string_view& str) override;

private:
  {{scopedModel}}* _pEnum = nullptr;
};

