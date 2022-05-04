In `3DTILES_implicit_tiling`, the _root_ availability subtree describes:

1. Which tiles exist at levels 0-n.
2. Which tiles have content at levels 0-n.
3. Which tiles at level n+1 have further subtrees.

cesium-native assumes that any tile holding a further subtree _exists_, but it doesn't assume that it has content. To determine if a tile at level n+1 has content, that tile's subtree must first be loaded.

This "gate" is implemented in `Tileset::addTileToLoadQueue`. A tile without a loaded subtree will not actually be added to the load queue, and will remain in the `Unloaded` state. However, it will - by other mechanisms - be added to the _subtree_ load queue.

Thus, a tile which is the root of a subtree will wait in the `Unloaded` state until the subtree data is fully loaded, and only then will will the content be loaded, if it exists. If the availability data indicates the tile does not have content, it will be immediately transitioned into the `ContentLoaded` state without any network requests.

This is really important, because it ensures:

1. We don't call `createImplicitChildrenIfNeeded` before we know whether the implicit children exist. And that we _do_ call it after we know whether or not the implicit children exist. This method is only called once on the transition from `ContentLoaded` to `Done`, but the mechanism above ensures we never arrive at that state too early.
2. We don't erroneously upsample tiles that seemingly have no children, but that will once we load availability.

## Application to quantized-mesh and layers

In quantized-mesh, we already know that a tile has content available, but then we learn about availability in deeper levels from the content of that tile. We can't prevent content load until we have availability data, because then we'd never have content or availability data. However, getting both back at the same time is nearly as good. We just need to make sure we load the tile in any layers for which this tile is at an availability level, and add the availability data to the appropriate context, prior to allowing the tile to enter the `ContentLoaded` state. That way we will always have the necessary availability information prior to calling `createImplicitChildrenIfNeeded`. This currently happens in `Tileset::requestTileContent`.

## Level skipping with implicit tiling and quantized-mesh

Because `createImplicitChildrenIfNeeded` is only called after a tile hits the `ContentLoaded` state, implicitly-tiled tilesets can't skip levels. They always have to load level zero tiles before they can load the level one tiles. This is in contrast to explicit tiling, which can skip levels and load more detailed tiles first.
