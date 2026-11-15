# GPU particles

**Goal:** Add snow and confetti to ParticleScene, with an adjustable active count.
**Architecture:** Allocate 100,000 fixed particle slots once. A normal RWStructuredBuffer owns simulation state; the vertex shader reads the same buffer as an SRV. CPU supplies the active count. One existing indirect draw argument buffer supplies six vertices and N instances without a dummy instance vertex buffer or engine API changes.
**Tech Stack:** JamEngine RenderGraph, D3D11, HLSL 5, ImGui, MSBuild.
**Spec:** Latest conversation: snow/confetti, normal RW buffer, no Append/Consume; increasing N initializes newly enabled slots; decreasing N stops processing the tail. Include pause, restart, wind and size controls. No asset dependencies.
**Global Constraints:** Continue the current dirty checkout and preserve prior changes; do not stage or commit. Only open ParticleScene during validation. Default example checks switch to ParticleScene.
**Review Focus:** 0/1/255/256/257/100000 counts; shrink then grow; paused initialization and restart; untouched inactive slots; SRV/UAV transitions between frames; transparent snow versus depth-writing two-sided paper.

## Task 1 — GPU lifecycle

- [x] Add a WARP test that compiles and executes the production shader with Debug and Release shader flags. Observe failure before the shader exists.
- [x] Implement 64-byte particle state and 48-byte simulation constants, guarded 256-thread dispatch, seeded initialization, falling/wind motion, and respawn.
- [x] Verify count boundaries, inactive slots, shrink/grow, pause, reset and respawn for both presets.

## Task 2 — Scene and rendering

- [x] Add ParticleScene with simulation → background/floor → particles → blit passes.
- [x] Add procedural snow circles and rotating two-sided paper, plus count/preset/pause/restart/wind/size controls.
- [x] Register scene/resources in the app and project; support ParticleScene --demo 1..2 and --particles 0..100000 for reproducible launches.
- [x] Extend the example runner to check both ParticleScene presets by default; document launch commands.

## Task 3 — Verification

- [x] Build Debug and Release, run the GPU lifecycle test, and run only ParticleScene examples in both configurations.
- [x] Inspect both screenshots and check zero/max count launches; run a fresh focused review and resolve substantive findings.

## Execution record

- Ruling: Continue in the existing checkout — the user requested continuation of substantial uncommitted engine work and authorized implementation with “진행”; a clean worktree would omit that foundation. No commits or unrelated edits.
- Ruling: Implement the latest snow/confetti choice; the earlier spark fountain is superseded for this scene.
- Ruling: Execute inline after the already discussed design and explicit authorization; no repeated approval step.
- GPU lifecycle RED: production shader absent; GREEN: both presets pass with Debug and Release shader flags.
- Scene RED: Debug asserted on discard for the default-usage draw buffer, then on a missing particle SRV. Corrected the upload's discard flag and requested both ShaderRead and ShaderReadWrite buffer capabilities. Release PNG creation alone had passed with no particles; visual inspection caught the missing SRV.
- Final validation: Debug/Release builds pass. Both presets pass 120-frame scene runs in each configuration; screenshots show particles. GPU lifecycle test passes both presets with both shader configurations. Twelve scene launches cover 0/1/100000 particles across both presets/configurations. Max-count images differ from zero-count backgrounds at over one million pixels. Seven malformed/out-of-range/wrong-scene CLI cases are rejected before scene startup.
- Fresh focused review found no important or critical defect. Its noted compute-only harness limitation is covered by the separate real scene runs and screenshot inspection; dynamic control behavior is covered at the shader lifecycle boundary and reviewed in the scene code.
- No shared engine implementation was changed, no other scenes were opened, and no changes were staged or committed.
