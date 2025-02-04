# Dependencies

Cesium Native relies on a number of third-party dependencies. These dependencies are automatically resolved and built using [vcpkg](https://vcpkg.io/) at compile time. 

| Dependency                                                                                                          | Usage                                                                                                             |
| ------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------- |
| [ada](https://github.com/ada-url/ada)                                                                               | Used to parse and manipulate URIs.                                                                                |
| [Async++](https://github.com/Amanieu/asyncplusplus)                                                                 | Used by CesiumAsync for cross-platform concurrency.                                                               |
| [Catch2](https://github.com/catchorg/Catch2)                                                                        | Test framework used by CesiumNativeTests.                                                                         |
| [draco](https://github.com/google/draco)                                                                            | Required to decode meshes and point clouds compressed with Draco.                                                 |
| [earcut](https://github.com/mapbox/earcut.hpp)                                                                      | Polygon triangulation library for used by CartographicPolygon.                                                    |
| [expected-lite](https://github.com/martinmoene/expected-lite)                                                       | Implementation of the `std::expected` proposal for returning either an expected value or an error value.          |
| [glm](https://github.com/g-truc/glm)                                                                                | C++ mathematics library powering the high-precision math needed for geospatial software.                          |
| [meshoptimizer](https://github.com/zeux/meshoptimizer)                                                              | Required to decode meshes compressed with meshoptimizer.                                                          |
| [httplib](https://github.com/yhirose/cpp-httplib)                                                                   | Used by CesiumIonClient to interact with the Cesium ion REST API.                                                 |
| [Ktx](https://github.com/CesiumGS/KTX-Software)                                                                     | Required to load KTX GPU compressed textures.                                                                     |
| [libmorton](https://github.com/Forceflow/libmorton)                                                                 | Implementation of Morton codes used for implicit tiling.                                                          |
| [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo)                                                     | Decodes JPEG images.                                                                                              |
| [libwebp](https://github.com/webmproject/libwebp)                                                                   | Decodes WebP images.                                                                                              |
| [modp_b64](https://github.com/chromium/chromium/tree/15996b5d2322b634f4197447b10289bddc2b0b32/third_party/modp_b64) | Decodes and encodes base64.                                                                                       |
| [OpenSSL](https://github.com/openssl/openssl)                                                                       | Required by s2geometry, and also used to generate unique authorization tokens for authenticating with Cesium ion. |
| [PicoSHA2](https://okdshin/PicoSHA2)                                                                                | Generates SHA256 hashes for use with the Cesium ion REST API.                                                     |
| [RapidJSON](https://github.com/Tencent/rapidjson)                                                                   | For JSON reading and writing.                                                                                     |
| [s2geometry](https://github.com/google/s2geometry)                                                                  | Spatial indexing library designed for geospatial use and required by some tilesets.                               |
| [spdlog](https://github.com/gabime/spdlog)                                                                          | Logging.                                                                                                          |
| [sqlite3](https://www.sqlite.org/index.html)                                                                        | Used to cache HTTP responses.                                                                                     |
| [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)                                                | A simple image loader.                                                                                            |
| [tinyxml2](https://github.com/leethomason/tinyxml2)                                                                 | XML parser for interacting with XML APIs such as those implementing the Web Map Service standard.                 |
| [zlib-ng](https://github.com/zlib-ng/zlib-ng)                                                                       | An optimized zlib implementation for working with Gzipped data.                                                   |

The following chart illustrates the connections between the Cesium Native libraries and third-party dependencies:

\svg-interactive{dependency-graph, 600px}