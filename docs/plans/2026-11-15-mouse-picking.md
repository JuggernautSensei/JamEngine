# ID buffer picking

**Goal:** Implement MousePickingScene with GPU ID picking and visible selection outlines.
**Architecture:** A single opaque MRT pass writes RGBA color and R32_UInt object IDs using the same depth test. IDs start at 1; ID 0 is cleared every frame and means no selection. On a click, copy only that pixel to a reusable 1×1 staging texture and read the ID. A final pass draws the selected visible silhouette from the ID buffer.
**Tech Stack:** JamEngine RenderGraph, D3D11/HLSL 5, ImGui, MSBuild, WARP regression test.
**Spec:** Conversation approved ID buffer picking, clear value 0, unused object ID 0, and immediate implementation. Use existing meshes and engine APIs. Readback occurs only on a click.
**Global Constraints:** Continue in this dirty shared checkout; preserve prior work, do not stage or commit. Validate only MousePickingScene and update the default example target. No unrelated engine API changes.
**Review Focus:** Integer precision, depth/order independence, zero/background selection, stale IDs after clearing, readback resource hazards, mouse/viewport bounds and resizing, UI capture, visible outline boundaries.

## Tasks

- [x] Write a WARP test using the production MRT/composite shaders; observe failure before shaders exist.
- [x] Implement opaque MRT output and ID-based outline/debug rendering. Test clear 0, near/far overlap in both submission orders, a large exact ID, partial 1×1 readback, outline and no-selection behavior using Debug/Release shader flags.
- [x] Implement MousePickingScene with existing cube/sphere/bunny meshes, overlapping objects, left-click selection, background deselection, ID visualization and outline controls. Reuse one staging pixel and skip CPU readback without a request.
- [x] Register scene/resources and CLI --pick-x/--pick-y for reproducible clicks; update README and the default example target.
- [x] Build Debug/Release and run only MousePickingScene, inspect normal/selected/debug captures, verify click logs and rejected CLI input, and run a fresh focused review.

## Execution record

- Ruling: Execute the discussed design inline without repeated approval, as explicitly requested. Keep the current checkout because it contains the engine foundation from the ongoing session.
- Ruling: Use click-only synchronous readback through existing APIs; continuous hover/asynchronous readback is outside the requested scope.
- Ruling: A final shader outline uses the visible ID silhouette, including hard-edged meshes, without modifying object geometry or adding stencil passes.
- GPU RED: production shader files absent. GPU GREEN: actual MRT/outline shaders pass both shader configurations.
- Fresh review: cached Input mouse coordinates lag one frame. Capture fresh client cursor coordinates in the mouse-down event instead; event-driven capture also preserves short clicks between updates. No shared input implementation changes.
- Outline regression RED: radius-2 ring sampling missed a one-pixel hole. Sample the complete radius neighborhood, stopping as soon as an edge is found.
- Integration validation now checks normal, selected, background and ID views by default, with exact selection-log expectations. No-click runs must not perform a readback.
- Final verification: Debug/Release builds pass; 4/4 MousePickingScene cases pass in each configuration at 60 frames. WARP MRT/outline tests pass both shader configurations, including the narrow-hole regression. An additional 22 scene launches verify all six mesh IDs, the bunny ear gap as ID0, floor/background, last viewport pixel, out-of-bounds coordinates and INT_MAX coordinates; valid clicks read exactly once and outside clicks never read. Eight malformed/incomplete/wrong-scene CLI cases reject before startup.
- Inspected normal, selected and ID visualization PNGs; the selected visible sphere has a gold silhouette and ID view shows distinct front/back/ear regions. Fresh read-only recheck reports no remaining substantive findings. Interactive mouse event timing was verified by code review; automated requests use the shared RequestPick path through the CLI.
- No shared engine implementation changed; no other scenes were opened, and nothing was staged or committed.
