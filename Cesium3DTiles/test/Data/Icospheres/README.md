
A fairly hand-crafted tileset consisting of four spheres with a 
discrete level of detail.

The spheres are icospheres, i.e. based on icosahedrons, created
by subdivision of the faces. The icosphere at level `n` 
therefore has `20*(4^n)` faces.

To view this tileset in a local viewer, serve it with a local server 
as described in https://github.com/CesiumGS/3d-tiles-samples, and
use the following viewer code:

    var viewer = new Cesium.Viewer('cesiumContainer');
    viewer.globe = false;
    var tileset = viewer.scene.primitives.add(new Cesium.Cesium3DTileset({
        url : 'http://localhost:8003/tilesets/Icospheres/tileset.json',
        maximumScreenSpaceError: 100,
        debugWireframe: true
    }));

    viewer.camera.lookAt(
        new Cesium.Cartesian3(1.0, 1.0, 1.0),,
        new Cesium.Cartesian3(0.0, 0.0, 5.0));


This tileset is in the [public domain](https://creativecommons.org/publicdomain/zero/1.0/).