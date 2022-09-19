# Install script for directory: /home/betto/src/cesium-native-ooc/CesiumGltf

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/betto/src/cesium-native-ooc/buildD/CesiumGltf/libCesiumGltfd.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/CesiumGltf" TYPE FILE FILES
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/AccessorSparse.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/AccessorSparseIndices.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/AccessorSparseValues.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/AccessorSpec.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Animation.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/AnimationChannel.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/AnimationChannelTarget.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/AnimationSampler.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Asset.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Bounds.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/BufferSpec.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/BufferView.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Camera.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/CameraOrthographic.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/CameraPerspective.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Class.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ClassProperty.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ClassStatistics.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Enum.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/EnumValue.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionBufferExtMeshoptCompression.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionCesiumRTC.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionCesiumTileEdges.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtInstanceFeatures.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtInstanceFeaturesFeatureId.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtMeshFeatures.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtMeshFeaturesFeatureId.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtMeshFeaturesFeatureIdTexture.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtMeshGpuInstancing.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtMeshGpuInstancingExtFeatureMetadata.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtPrimitiveVoxels.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataClass.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataClassProperty.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataEnum.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataEnumValue.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataPropertyAttribute.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataPropertyAttributeProperty.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataPropertyTable.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataPropertyTableProperty.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataPropertyTexture.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataPropertyTextureProperty.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionExtStructuralMetadataSchema.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionKhrDracoMeshCompression.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionKhrMaterialsUnlit.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionKhrTextureBasisu.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionKhrTextureTransform.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionMeshPrimitiveExtFeatureMetadata.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionMeshPrimitiveExtStructuralMetadata.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionModelExtFeatureMetadata.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionModelExtStructuralMetadata.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionModelMaxarMeshVariants.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionModelMaxarMeshVariantsValue.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionNodeMaxarMeshVariants.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionNodeMaxarMeshVariantsMappingsValue.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ExtensionTextureWebp.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/FeatureIDAttribute.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/FeatureIDTexture.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/FeatureIDs.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/FeatureTable.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/FeatureTableProperty.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/FeatureTexture.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ImageSpec.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Material.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/MaterialNormalTextureInfo.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/MaterialOcclusionTextureInfo.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/MaterialPBRMetallicRoughness.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Mesh.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/MeshPrimitive.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/ModelSpec.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Node.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Padding.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/PropertyStatistics.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Sampler.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Scene.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Schema.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Skin.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Statistics.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/Texture.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/TextureAccessor.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/generated/include/CesiumGltf/TextureInfo.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/Accessor.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/AccessorView.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/AccessorWriter.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/Buffer.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/BufferCesium.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/FeatureIDTextureView.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/FeatureTexturePropertyView.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/FeatureTextureView.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/Image.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/ImageCesium.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/Ktx2TranscodeTargets.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/Library.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/MetadataArrayView.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/MetadataFeatureTableView.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/MetadataPropertyView.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/Model.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/NamedObject.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/PropertyType.h"
    "/home/betto/src/cesium-native-ooc/CesiumGltf/include/CesiumGltf/PropertyTypeTraits.h"
    )
endif()

