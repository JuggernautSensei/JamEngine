# Mixamo skeletal animation example

User request: implement a skeletal animation example, permit engine changes, explain the changed code and execution pseudocode, and use the usual animation technique.

## Design

- Asset: the user's existing `Downloads/Unarmed Walk Forward.fbx`, exported by Mixamo. Inspection with Assimp 6.0.2 found four mesh parts, 69 joints per mesh, one animation, and three embedded textures.
- CPU animation sampling: independent translation/rotation/scale keys, linear interpolation for vectors and shortest-path quaternion SLERP for rotations. Keep every node, including FBX helpers, in parent-first order. Preserve exact bind matrices for unanimated nodes.
- GPU linear blend skinning: four normalized influences, byte joint indices, at most 256 joints per mesh. Keep inverse bind matrices per mesh. Unweighted vertices use the identity skin transform.
- Jam uses row vectors: `global = local * parentGlobal`; palette entry is `inverseBind * animatedJointGlobal * inverse(animatedMeshGlobal)`.
- Keep animation data separate from the existing static `.jmdl` serialization. The example loads the source FBX through Assimp and owns all copied data. No animation serialization, retargeting, state machine, or root motion extraction in this example.
- Example controls: pause, speed, seek, bind pose, wireframe and skeleton display. Render every mesh part with its material.

## Execution

- [ ] Add regression tests and observe a failure in the old importer.
- [ ] Add owned skeleton/clip data, pose sampling, palette building, importer skin attributes, matrix conversion and embedded material mapping.
- [ ] Add Mixamo FBX, shader and scene; register the scene and development validation target.
- [ ] Verify CPU sampling/import regressions, GPU skinning, Debug/Release builds and scene captures at multiple poses.
- [ ] Review changes; document exact engine modifications, pseudocode, controls, provenance and remaining limitations.

## Pseudocode

```text
load FBX -> copy mesh attributes, hierarchy, inverse binds, animation keys
each frame:
    advance playback time (loop/clamp)
    for each animated node:
        local = SRT(lerp(scale), slerp(rotation), lerp(translation))
    for each node in parent-first order:
        global[node] = local[node] * global[parent]
    for each mesh part:
        palette[joint] = inverseBind[joint] * global[joint] * inverse(global[meshNode])
        upload palette and mesh world transform
        vertex shader: weighted sum of four joint transforms -> camera transform
        draw the mesh part
```
