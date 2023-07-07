The original image, kota.jpg, is a photo taken by Kevin Ring and licensed under the same Apache 2.0 terms as the rest of cesium-native.

The other images were created using the `toktx` tool included in https://github.com/KhronosGroup/KTX-Software v4.2.0~11. They were created with the following commands:

| *Filename* | *Command* |
|------------|-----------|
| `kota-automipmap.ktx2` | `toktx --t2 --zcmp --encode uastc --automipmap kota-automipmap kota.jpg` |
| `kota-onelevel.ktx2` | `toktx --t2 --zcmp --encode uastc kota-onelevel kota.jpg` |
| `kota-mipmaps.ktx2` | `toktx --t2 --zcmp --encode uastc --genmipmap kota-mipmaps kota.jpg` |
