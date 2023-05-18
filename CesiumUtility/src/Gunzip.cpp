#include "CesiumUtility/Gunzip.h"
#define ZLIB_CONST
#include "zlib.h"

#define CHUNK 65536

bool CesiumUtility::gunzip(
    const gsl::span<const std::byte>& data,
    std::vector<std::byte>& out) {
  int ret;
  int index = 0;
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit2(&strm, 16 + MAX_WBITS);
  if (ret != Z_OK) {
    return false;
  }

  strm.avail_in = static_cast<uInt>(data.size());
  strm.next_in = reinterpret_cast<const Bytef*>(data.data());

  do {
    out.resize(index + CHUNK);
    strm.next_out = reinterpret_cast<Bytef*>(&out[index]);
    strm.avail_out = CHUNK;
    ret = inflate(&strm, Z_NO_FLUSH);
    switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      inflateEnd(&strm);
      return false;
    }
    index += CHUNK - strm.avail_out;
  } while (ret != Z_STREAM_END);

  inflateEnd(&strm);
  out.resize(index);
  return true;
}
