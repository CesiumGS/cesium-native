# Write out a gltf test file from python code
# Unit cube - Triangle strip, indexed (UNSIGNED_SHORT)
#
# Uses pygltflib 1.16.2
# py -m pip install pygltflib
#
import numpy as np
import pygltflib

# Change extension to .gltf for text based
outputFileName = "cubeStripIndexed.glb"

points = np.array(
    [
        [-0.5, -0.5, 0.5],
        [0.5, -0.5, 0.5],
        [-0.5, 0.5, 0.5],
        [0.5, 0.5, 0.5],
        [0.5, -0.5, -0.5],
        [-0.5, -0.5, -0.5],
        [0.5, 0.5, -0.5],
        [-0.5, 0.5, -0.5],
    ],
    dtype="float32",
)

triangles = np.array(
    [
        # +XY face
        [2, 0, 3],
        [0, 3, 1],

        # +YZ face
        [3, 1, 6],
        [1, 6, 4],

        # -XY face
        [6, 4, 7],
        [4, 7, 5],

        # -YZ face
        [7, 5, 2],
        [5, 2, 0],

        # -XZ face
        [0, 5, 1],
        [5, 1, 4],

        # +XZ face
        [6, 7, 3],
        [7, 3, 2],
    ],
    dtype="uint16",
)

triangles_binary_blob = triangles.flatten().tobytes()
points_binary_blob = points.tobytes()
gltf = pygltflib.GLTF2(
    scene=0,
    scenes=[pygltflib.Scene(nodes=[0])],
    nodes=[pygltflib.Node(mesh=0)],
    meshes=[
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=1),
                    indices=0, 
                    mode=5 #"TRIANGLE_STRIP"
                )
            ]
        )
    ],
    accessors=[
        pygltflib.Accessor(
            bufferView=0,
            componentType=pygltflib.UNSIGNED_SHORT,
            count=triangles.size,
            type=pygltflib.SCALAR,
            max=[int(triangles.max())],
            min=[int(triangles.min())],
        ),
        pygltflib.Accessor(
            bufferView=1,
            componentType=pygltflib.FLOAT,
            count=len(points),
            type=pygltflib.VEC3,
            max=points.max(axis=0).tolist(),
            min=points.min(axis=0).tolist(),
        ),
    ],
    bufferViews=[
        pygltflib.BufferView(
            buffer=0,
            byteLength=len(triangles_binary_blob),
            target=pygltflib.ELEMENT_ARRAY_BUFFER,
        ),
        pygltflib.BufferView(
            buffer=0,
            byteOffset=len(triangles_binary_blob),
            byteLength=len(points_binary_blob),
            target=pygltflib.ARRAY_BUFFER,
        ),
    ],
    buffers=[
        pygltflib.Buffer(
            byteLength=len(triangles_binary_blob) + len(points_binary_blob)
        )
    ],
)
gltf.set_binary_blob(triangles_binary_blob + points_binary_blob)

gltf.save(outputFileName)