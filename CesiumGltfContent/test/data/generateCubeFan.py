# Write out a gltf test file from python code
# Unit cube - Triangle fan, no indices
#
# Uses pygltflib 1.16.2
# py -m pip install pygltflib
#
import numpy as np
import pygltflib

# Change extension to .gltf for text based
outputFileName = "cubeFan.glb"

face0_points = np.array(
    [
        # +XY face
        [-0.5, -0.5, 0.5],
        [0.5, -0.5, 0.5],
        [0.5, 0.5, 0.5],
        [-0.5, 0.5, 0.5],
    ],
    dtype="float32",
)

face1_points = np.array(
    [
        # -XY face
        [-0.5, -0.5, -0.5],
        [-0.5, 0.5, -0.5],
        [0.5, 0.5, -0.5],
        [0.5, -0.5, -0.5],
    ],
    dtype="float32",
)

face2_points = np.array(
    [
        # +YZ face
        [0.5, -0.5, -0.5],
        [0.5, 0.5, -0.5],
        [0.5, 0.5, 0.5],
        [0.5, -0.5, 0.5],
    ],
    dtype="float32",
)

face3_points = np.array(
    [
        # -YZ face
        [-0.5, -0.5, -0.5],
        [-0.5, -0.5, 0.5],
        [-0.5, 0.5, 0.5],
        [-0.5, 0.5, -0.5],
    ],
    dtype="float32",
)

face4_points = np.array(
    [
        # -XZ face
        [-0.5, -0.5, -0.5],
        [0.5, -0.5, -0.5],
        [0.5, -0.5, 0.5],
        [-0.5, -0.5, 0.5],
    ],
    dtype="float32",
)

face5_points = np.array(
    [
        # +XZ face
        [-0.5, 0.5, -0.5],
        [-0.5, 0.5, 0.5],
        [0.5, 0.5, 0.5],
        [0.5, 0.5, -0.5],
    ],
    dtype="float32",
)

face0_binary_blob = face0_points.tobytes()
face1_binary_blob = face1_points.tobytes()
face2_binary_blob = face2_points.tobytes()
face3_binary_blob = face3_points.tobytes()
face4_binary_blob = face4_points.tobytes()
face5_binary_blob = face5_points.tobytes()

facePointsLength = len(face0_points)
faceBlobByteLength = len(face0_binary_blob)

gltf = pygltflib.GLTF2(
    scene=0,
    scenes=[pygltflib.Scene(nodes=[0, 1, 2, 3, 4, 5])],
    nodes=[
        pygltflib.Node(mesh=0), 
        pygltflib.Node(mesh=1), 
        pygltflib.Node(mesh=2),
        pygltflib.Node(mesh=3),
        pygltflib.Node(mesh=4),
        pygltflib.Node(mesh=5)],
    meshes=[
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=0),
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=1),
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=2),
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=3),
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=4),
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        ),
        pygltflib.Mesh(
            primitives=[
                pygltflib.Primitive(
                    attributes=pygltflib.Attributes(POSITION=5),
                    mode=6 #"TRIANGLE_FAN"
                )
            ]
        )
    ],
    accessors=[
        pygltflib.Accessor(
            bufferView=0,
            componentType=pygltflib.FLOAT,
            count=facePointsLength,
            type=pygltflib.VEC3,
            max=face0_points.max(axis=0).tolist(),
            min=face0_points.min(axis=0).tolist(),
        ),
        pygltflib.Accessor(
            bufferView=1,
            componentType=pygltflib.FLOAT,
            count=facePointsLength,
            type=pygltflib.VEC3,
            max=face1_points.max(axis=0).tolist(),
            min=face1_points.min(axis=0).tolist(),
        ),
        pygltflib.Accessor(
            bufferView=2,
            componentType=pygltflib.FLOAT,
            count=facePointsLength,
            type=pygltflib.VEC3,
            max=face2_points.max(axis=0).tolist(),
            min=face2_points.min(axis=0).tolist(),
        ),
        pygltflib.Accessor(
            bufferView=3,
            componentType=pygltflib.FLOAT,
            count=facePointsLength,
            type=pygltflib.VEC3,
            max=face3_points.max(axis=0).tolist(),
            min=face3_points.min(axis=0).tolist(),
        ),
        pygltflib.Accessor(
            bufferView=4,
            componentType=pygltflib.FLOAT,
            count=facePointsLength,
            type=pygltflib.VEC3,
            max=face4_points.max(axis=0).tolist(),
            min=face4_points.min(axis=0).tolist(),
        ),
        pygltflib.Accessor(
            bufferView=5,
            componentType=pygltflib.FLOAT,
            count=facePointsLength,
            type=pygltflib.VEC3,
            max=face5_points.max(axis=0).tolist(),
            min=face5_points.min(axis=0).tolist(),
        ),
    ],
    bufferViews=[
        pygltflib.BufferView(
            buffer=0,
            byteLength=faceBlobByteLength,
            target=pygltflib.ARRAY_BUFFER,
        ),
        pygltflib.BufferView(
            buffer=0,
            byteOffset=faceBlobByteLength,
            byteLength=faceBlobByteLength,
            target=pygltflib.ARRAY_BUFFER,
        ),
        pygltflib.BufferView(
            buffer=0,
            byteOffset=faceBlobByteLength*2,
            byteLength=faceBlobByteLength,
            target=pygltflib.ARRAY_BUFFER,
        ),
        pygltflib.BufferView(
            buffer=0,
            byteOffset=faceBlobByteLength*3,
            byteLength=faceBlobByteLength,
            target=pygltflib.ARRAY_BUFFER,
        ),
        pygltflib.BufferView(
            buffer=0,
            byteOffset=faceBlobByteLength*4,
            byteLength=faceBlobByteLength,
            target=pygltflib.ARRAY_BUFFER,
        ),
        pygltflib.BufferView(
            buffer=0,
            byteOffset=faceBlobByteLength*5,
            byteLength=faceBlobByteLength,
            target=pygltflib.ARRAY_BUFFER,
        ),
    ],
    buffers=[
        pygltflib.Buffer(
            byteLength=faceBlobByteLength * 6
        )
    ],
)
gltf.set_binary_blob(face0_binary_blob + 
    face1_binary_blob + 
    face2_binary_blob + 
    face3_binary_blob +
    face4_binary_blob +
    face5_binary_blob)

gltf.save(outputFileName)