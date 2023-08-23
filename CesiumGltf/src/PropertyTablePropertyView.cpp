#include "CesiumGltf/PropertyTablePropertyView.h"

using namespace CesiumGltf;

// Re-initialize consts here to avoid "undefined reference" errors with GCC /
// Clang.
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidValueBufferView;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetBufferView;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetBufferView;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidValueBuffer;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetBuffer;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetBuffer;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorBufferViewOutOfBounds;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorBufferViewSizeNotDivisibleByTypeSize;
const PropertyViewStatusType PropertyTablePropertyViewStatus::
    ErrorBufferViewSizeDoesNotMatchPropertyTableCount;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetType;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayOffsetsNotSorted;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorStringOffsetsNotSorted;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorArrayOffsetOutOfBounds;
const PropertyViewStatusType
    PropertyTablePropertyViewStatus::ErrorStringOffsetOutOfBounds;
