# Write out a gltf test file from python code
# Unit cube - Triangle strip, no indices
#
# Uses pygltflib 1.16.2
# py -m pip install pygltflib
#
import numpy as np
import pygltflib

# Change extension to .gltf for text based
outputFileName = "cubeStrip.glb"

points = np.array(
    [
        # +XY face
        [-0.5, 0.5, 0.5],
        [-0.5, -0.5, 0.5],
        [0.5, 0.5, 0.5],
        [0.5, -0.5, 0.5],

        # +YZ face
        [0.5, 0.5, -0.5],
        [0.5, -0.5, -0.5],

        # -XY face
        [-0.5, 0.5, -0.5],
        [-0.5, -0.5, -0.5],

        # -YZ face
        [-0.5, 0.5, 0.5],
        [-0.5, -0.5, 0.5],

        # Break with degenerate
        [-0.5, -0.5, 0.5],

        # -XZ face
        [-0.5, -0.5, -0.5],
        [0.5, -0.5, 0.5],
        [0.5, -0.5, -0.5],

        # Break with degenerate
        [0.5, -0.5, -0.5],
        [0.5, 0.5, -0.5],

        # +XZ face
        [0.5, 0.5, -0.5],
        [-0.5, 0.5, -0.5],
        [0.5, 0.5, 0.5],
        [-0.5, 0.5, 0.5],
    ],
    dtype="float32",
)

points_binary_blob = points.tobytes()
gltf = pygltflib.GLTF2(
    scene=0,
    scenes=[pygltflib.Scene(nodes=[0])],
    nodes=[pygltflib.Node(mesh=0)],
    meshes=[
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=0),
                    mode=5 #"TRIANGLE_STRIP"
                )
            ]
        )
    ],
    accessors=[
        pygltflib.Accessor(
            bufferView=0,
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
            byteLength=len(points_binary_blob),
            target=pygltflib.ARRAY_BUFFER,
        ),
    ],
    buffers=[
        pygltflib.Buffer(
            byteLength=len(points_binary_blob)
        )
    ],
)
gltf.set_binary_blob(points_binary_blob)

gltf.save(outputFileName)