#pragma once

/**
 * @brief Classes that streamline access of metadata in glTF and 3D Tiles.
 *
 * @mermaid-interactive{dependencies/CesiumMetadata}
 */
namespace CesiumMetadata {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMMETADATA_BUILDING
#define CESIUMMETADATA_API __declspec(dllexport)
#else
#define CESIUMMETADATA_API __declspec(dllimport)
#endif
#else
#define CESIUMMETADATA_API
#endif
