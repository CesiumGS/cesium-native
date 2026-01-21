#include "CesiumGeometry/AxisAlignedBox.h"
#include <Cesium3DTilesSelection/GltfModifier.h>

namespace Cesium3DTilesSelection {
class ManifoldGltfModifier : public GltfModifier {
  /**
   * @brief Implement this method to apply custom modification to a glTF model.
   * It is called by the @ref Tileset from within a worker thread.
   *
   * This method will be called for each @ref Tile during the content load
   * process if @ref trigger has been called at least once. It will also be
   * called again for already-loaded tiles for successive calls to
   * @ref trigger.
   *
   * @param input The input to the glTF modification.
   * @return A future that resolves to a @ref GltfModifierOutput with the
   * new model, or to `std::nullopt` if the model does not need to be modified.
   */
  virtual CesiumAsync::Future<std::optional<GltfModifierOutput>>
  apply(GltfModifierInput&& input) override;

  CesiumGeometry::AxisAlignedBox box;
};
}