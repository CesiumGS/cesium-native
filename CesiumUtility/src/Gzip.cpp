#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Gzip.h>

#include <zconf-ng.h>
#include <zlib-ng.h>

#include <cstddef>
#include <cstring>
#include <span>
#include <vector>

namespace CesiumUtility {

namespace {
constexpr unsigned int ChunkSize = 65536;
}

bool isGzip(const std::span<const std::byte>& data) {
  if (data.size() < 3) {
    return false;
  }
  return data[0] == std::byte{31} && data[1] == std::byte{139};
}

bool gzip(const std::span<const std::byte>& data, std::vector<std::byte>& out) {
  int ret;
  unsigned int index = 0;
  zng_stream strm;
  std::memset(&strm, 0, sizeof(strm));

  ret = zng_deflateInit2(
      &strm,
      Z_BEST_COMPRESSION,
      Z_DEFLATED,
      16 + MAX_WBITS,
      8,
      Z_DEFAULT_STRATEGY);
  if (ret != Z_OK) {
    return false;
  }

  strm.avail_in = static_cast<uInt>(data.size());
  strm.next_in = reinterpret_cast<const Bytef*>(data.data());

  do {
    out.resize(index + ChunkSize);
    strm.next_out = reinterpret_cast<Bytef*>(&out[index]);
    strm.avail_out = ChunkSize;
    ret = zng_deflate(&strm, Z_NO_FLUSH);
    CESIUM_ASSERT(ret != Z_STREAM_ERROR);
    index += ChunkSize - strm.avail_out;
  } while (strm.avail_in != 0);

  do {
    out.resize(index + ChunkSize);
    strm.next_out = reinterpret_cast<Bytef*>(&out[index]);
    strm.avail_out = ChunkSize;
    ret = zng_deflate(&strm, Z_FINISH);
    CESIUM_ASSERT(ret != Z_STREAM_ERROR);
    index += ChunkSize - strm.avail_out;
  } while (ret != Z_STREAM_END);

  zng_deflateEnd(&strm);
  out.resize(index);
  return true;
}

bool gunzip(
    const std::span<const std::byte>& data,
    std::vector<std::byte>& out) {
  int ret;
  unsigned int index = 0;
  zng_stream strm;
  std::memset(&strm, 0, sizeof(strm));

  ret = zng_inflateInit2(&strm, 16 + MAX_WBITS);
  if (ret != Z_OK) {
    return false;
  }

  strm.avail_in = static_cast<uInt>(data.size());
  strm.next_in = reinterpret_cast<const Bytef*>(data.data());

  do {
    out.resize(index + ChunkSize);
    strm.next_out = reinterpret_cast<Bytef*>(&out[index]);
    strm.avail_out = ChunkSize;
    ret = zng_inflate(&strm, Z_NO_FLUSH);
    CESIUM_ASSERT(ret != Z_STREAM_ERROR);
    switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_BUF_ERROR:
    case Z_MEM_ERROR:
      zng_inflateEnd(&strm);
      return false;
    }
    index += ChunkSize - strm.avail_out;
  } while (ret != Z_STREAM_END);

  zng_inflateEnd(&strm);
  out.resize(index);
  return true;
}

} // namespace CesiumUtility
