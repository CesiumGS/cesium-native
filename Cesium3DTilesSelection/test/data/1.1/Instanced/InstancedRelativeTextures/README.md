# Relative references test

Mainly for https://github.com/CesiumGS/cesium-native/issues/1378

- The `./tileset.json` refers to `i3dm/I3dmWithGlbWithExternalTexture.i3dm`.
- The `./i3dm/I3dmWithGlbWithExternalTexture.i3dm` refers to `../glb/ExternalTexture.glb`.
- The `./glb/ExternalTexture.glb` refers to three files:
  - `../textureA.png` (resolved to `./textureA.png`)
  - `textureB.png` (resolved to `./glb/textureB.png`)
  - `sub/textureC.png`  (resolved to `./glb/sub/textureC.png`)
  
  
Sandcastle:
```javascript
const viewer = new Cesium.Viewer("cesiumContainer");

//viewer.extend(Cesium.viewerCesium3DTilesInspectorMixin);

// Create the tileset and add it in the viewer
const tileset = await Cesium.Cesium3DTileset.fromUrl(
  "http://localhost:8003/tileset.json", {
  debugShowBoundingVolume: true,
});
viewer.scene.primitives.add(tileset);

//viewer.cesium3DTilesInspector._viewModel._tileset = tileset; // For splats
//tileset.maximumScreenSpaceError = 1000;

// Move the tileset to a certain position on the globe
const transform = Cesium.Transforms.eastNorthUpToFixedFrame(
  Cesium.Cartesian3.fromDegrees(-75.152408, 39.946975, 20)
);
const scale = 1.0;
const modelMatrix = Cesium.Matrix4.multiplyByUniformScale(
  transform,
  scale,
  new Cesium.Matrix4()
);
tileset.modelMatrix = modelMatrix;

// Zoom to the tileset, with a small offset so that
// it is fully visible
const offset = new Cesium.HeadingPitchRange(
  Cesium.Math.toRadians(-22.5),
  Cesium.Math.toRadians(-22.5),
  10.0
);
viewer.zoomTo(tileset, offset);
```

  
  