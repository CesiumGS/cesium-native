# Write out a gltf test file from python code
# Unit cube - Triangle fan, indexed (UNSIGNED_INT)
#
# Uses pygltflib 1.16.2
# py -m pip install pygltflib
#
import numpy as np
import pygltflib

# Change extension to .gltf for text based
outputFileName = "cubeFanIndexed.glb"

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
        0, 1, 3, 2,

        # -XY face
        5, 7, 6, 4,

        # +YZ face
        4, 6, 3, 1,

        # -YZ face
        5, 0, 2, 7,

        # -XZ face
        5, 4, 1, 0,

        # +XZ face
        7, 2, 3, 6,
    ],
    dtype="uint32",
)
triangle_face_byte_length = 16

points_binary_blob = points.tobytes()
triangles_binary_blob = triangles.tobytes()

gltf = pygltflib.GLTF2(
    scene=0,
    scenes=[pygltflib.Scene(nodes=[0, 1, 2, 3, 4, 5])],
    nodes=[
        pygltflib.Node(mesh=0), 
        pygltflib.Node(mesh=1), 
        pygltflib.Node(mesh=2), 
        pygltflib.Node(mesh=3), 
        pygltflib.Node(mesh=4), 
        pygltflib.Node(mesh=5), 
    ],
    meshes=[
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=0),
                    indices=1,
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=0),
                    indices=2,
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=0),
                    indices=3,
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=0),
                    indices=4,
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=0),
                    indices=5,
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=0),
                    indices=6,
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
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
        pygltflib.Accessor(
            bufferView=1,
            componentType=pygltflib.UNSIGNED_INT,
            count=4,
            type=pygltflib.SCALAR,
            max=[int(triangles[0:4].max())],
            min=[int(triangles[0:4].min())],
        ),
        pygltflib.Accessor(
            bufferView=1,
            byteOffset=triangle_face_byte_length,
            componentType=pygltflib.UNSIGNED_INT,
            count=4,
            type=pygltflib.SCALAR,
            max=[int(triangles[4:8].max())],
            min=[int(triangles[4:8].min())],
        ),
        pygltflib.Accessor(
            bufferView=1,
            byteOffset=triangle_face_byte_length * 2,
            componentType=pygltflib.UNSIGNED_INT,
            count=4,
            type=pygltflib.SCALAR,
            max=[int(triangles[8:12].max())],
            min=[int(triangles[8:12].min())],
        ),
        pygltflib.Accessor(
            bufferView=1,
            byteOffset=triangle_face_byte_length * 3,
            componentType=pygltflib.UNSIGNED_INT,
            count=4,
            type=pygltflib.SCALAR,
            max=[int(triangles[12:16].max())],
            min=[int(triangles[12:16].min())],
        ),
        pygltflib.Accessor(
            bufferView=1,
            byteOffset=triangle_face_byte_length * 4,
            componentType=pygltflib.UNSIGNED_INT,
            count=4,
            type=pygltflib.SCALAR,
            max=[int(triangles[16:20].max())],
            min=[int(triangles[16:20].min())],
        ),
        pygltflib.Accessor(
            bufferView=1,
            byteOffset=triangle_face_byte_length * 5,
            componentType=pygltflib.UNSIGNED_INT,
            count=4,
            type=pygltflib.SCALAR,
            max=[int(triangles[20:24].max())],
            min=[int(triangles[20:24].min())],
        ),
    ],
    bufferViews=[
        pygltflib.BufferView(
            buffer=0,
            byteLength=len(points_binary_blob),
            target=pygltflib.ARRAY_BUFFER,
        ),
        pygltflib.BufferView(
            buffer=0,
            byteOffset=len(points_binary_blob),
            byteLength=len(triangles_binary_blob),
            target=pygltflib.ELEMENT_ARRAY_BUFFER,
        ),
    ],
    buffers=[
        pygltflib.Buffer(
            byteLength=len(points_binary_blob) + len(triangles_binary_blob)
        )
    ],
)
gltf.set_binary_blob(points_binary_blob + triangles_binary_blob)

gltf.save(outputFileName)