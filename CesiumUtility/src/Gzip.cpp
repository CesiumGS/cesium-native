#include "CesiumUtility/Gzip.h"
#define ZLIB_CONST
#include <zlib.h>

#include <cstring>

namespace CesiumUtility {

namespace {
constexpr unsigned int ChunkSize = 65536;
}

/*static*/ bool Gzip::isGzip(const gsl::span<const std::byte>& data) {
  if (data.size() < 3) {
    return false;
  }
  return data[0] == std::byte{31} && data[1] == std::byte{139};
}

/*static*/ bool Gzip::gzip(
    const gsl::span<const std::byte>& data,
    std::vector<std::byte>& out) {
  int ret;
  unsigned int index = 0;
  z_stream strm;
  std::memset(&strm, 0, sizeof(strm));

  ret = deflateInit2(
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
    ret = deflate(&strm, Z_NO_FLUSH);
    switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      deflateEnd(&strm);
      return false;
    }
    index += ChunkSize - strm.avail_out;
  } while (ret != Z_STREAM_END);

  deflateEnd(&strm);
  out.resize(index);
  return true;
}

/*static*/ bool Gzip::gunzip(
    const gsl::span<const std::byte>& data,
    std::vector<std::byte>& out) {
  int ret;
  unsigned int index = 0;
  z_stream strm;
  std::memset(&strm, 0, sizeof(strm));

  ret = inflateInit2(&strm, 16 + MAX_WBITS);
  if (ret != Z_OK) {
    return false;
  }

  strm.avail_in = static_cast<uInt>(data.size());
  strm.next_in = reinterpret_cast<const Bytef*>(data.data());

  do {
    out.resize(index + ChunkSize);
    strm.next_out = reinterpret_cast<Bytef*>(&out[index]);
    strm.avail_out = ChunkSize;
    ret = inflate(&strm, Z_NO_FLUSH);
    switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      inflateEnd(&strm);
      return false;
    }
    index += ChunkSize - strm.avail_out;
  } while (ret != Z_STREAM_END);

  inflateEnd(&strm);
  out.resize(index);
  return true;
}

} // namespace CesiumUtility
