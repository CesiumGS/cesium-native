#pragma once

#include "EnumJsonHandler.h"

#include <CesiumI3S/TextureDefinition.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/BoolJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumI3SReader {

class ImageJsonHandler : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::Image;
  ImageJsonHandler() noexcept;
  void
  reset(CesiumJsonReader::IJsonHandler* pParent, CesiumI3S::Image* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::Image* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _id;
  CesiumJsonReader::IntegerJsonHandler<uint32_t> _size;
  CesiumJsonReader::DoubleJsonHandler _pixelInWorldUnits;
  CesiumJsonReader::
      ArrayJsonHandler<std::string, CesiumJsonReader::StringJsonHandler>
          _href;
  CesiumJsonReader::
      ArrayJsonHandler<uint64_t, CesiumJsonReader::IntegerJsonHandler<uint64_t>>
          _byteOffset;
  CesiumJsonReader::
      ArrayJsonHandler<uint64_t, CesiumJsonReader::IntegerJsonHandler<uint64_t>>
          _length;
};

class TextureDefinitionInfoJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::TextureDefinitionInfo;
  TextureDefinitionInfoJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::TextureDefinitionInfo* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::TextureDefinitionInfo* _pObject = nullptr;
  CesiumJsonReader::
      ArrayJsonHandler<std::string, CesiumJsonReader::StringJsonHandler>
          _encoding;
  EnumArrayJsonHandler<CesiumI3S::TextureWrap> _wrap;
  CesiumJsonReader::BoolJsonHandler _atlas;
  CesiumJsonReader::StringJsonHandler _uvSet;
  EnumJsonHandler<CesiumI3S::TextureChannels> _channels;
  CesiumJsonReader::ArrayJsonHandler<CesiumI3S::Image, ImageJsonHandler>
      _images;
};

class TextureSetDefinitionFormatJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::TextureSetDefinitionFormat;
  TextureSetDefinitionFormatJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::TextureSetDefinitionFormat* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::TextureSetDefinitionFormat* _pObject = nullptr;
  CesiumJsonReader::StringJsonHandler _name;
  EnumJsonHandler<CesiumI3S::TextureFormat> _format;
};

class TextureSetDefinitionJsonHandler
    : public CesiumJsonReader::ObjectJsonHandler {
public:
  using ValueType = CesiumI3S::TextureSetDefinition;
  TextureSetDefinitionJsonHandler() noexcept;
  void reset(
      CesiumJsonReader::IJsonHandler* pParent,
      CesiumI3S::TextureSetDefinition* pObject);
  CesiumJsonReader::IJsonHandler*
  readObjectKey(const std::string_view& str) override;

private:
  CesiumI3S::TextureSetDefinition* _pObject = nullptr;
  CesiumJsonReader::ArrayJsonHandler<
      CesiumI3S::TextureSetDefinitionFormat,
      TextureSetDefinitionFormatJsonHandler>
      _formats;
  CesiumJsonReader::BoolJsonHandler _atlas;
};

} // namespace CesiumI3SReader
