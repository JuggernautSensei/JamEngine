# JamEngine Build and Cleanup Implementation Plan

**Goal:** Restore builds and execution of the engine, every active sandbox example, tests and benchmarks while preserving existing C++ style.

**Architecture:** Keep the Visual Studio/MSBuild and D3D11 architecture. SandboxApp is the active example host; ExampleApp is explicitly deprecated in CLAUDE.md. Repair shared failures before removing obsolete implementations and their project references.

**Tech Stack:** C++20, Visual Studio v145, Direct3D 11, EnTT, ImGui, Google Test, vcpkg.

**Spec:** User requirements 1–9 in this session; repository coding conventions in CLAUDE.md. The user requests immediate implementation after planning.

## Constraints

- Preserve naming, brace formatting, ownership conventions, and the no-exceptions policy.
- Fix broken existing code as required to restore all active examples.
- Remove BasicRenderer and confirmed deprecated files; preserve assets used by active examples.
- Keep source changes in the current workspace for review without committing.

## Review Focus

- Debug versus Release: assertions must not contain required side effects.
- Every sandbox scene must run, including scenes missing from the current CLI switch.
- Default launch, invalid scene names, and bounded smoke execution must have useful exit codes.
- Resource paths and DLL loading must work from the documented launch directory.
- Removed files must have no surviving build, include, filter, or runtime references.

## Pre-flight

Shared interfaces: engine headers and library feed all applications; engine fixes precede app validation. Sandbox scene registration and resources are verified before deleting old application assets. Output paths and pragma library references must be updated together.

### Task 1: Restore shared engine build

- [x] Reproduce `Debug|x64` solution build failure in `build-baseline.log`.
- [x] Correct `JamEngine/String.h` and any subsequent root causes identified by compiler diagnostics.
- [x] Verify engine and dependent applications using MSBuild; add behavioral regressions in JamTest where appropriate.

### Task 2: Restore and exercise every active example

- [x] Inspect `SandboxApp/*Scene.*`, CLI registration, shader inputs, and resource paths.
- [x] Repair runtime failures and provide bounded execution for repeatable smoke checks.
- [x] Run all current example scenes and demos; remove the retired editor mode.

### Task 3: Validate auxiliary applications

- [x] Run JamTest and propagate actual test failures to the process exit code.
- [x] Build and run JamBenchmark; preserve the current tree's removal of JamShader and its solution registration.

### Task 4: Remove obsolete code and organize directories

- [x] Delete BasicRenderer, deprecated ExampleApp, obsolete duplicates, and confirmed unused old files.
- [x] Update vcxproj, filters, solution references, build outputs, and ignore rules together.
- [x] Document the supported build and launch commands and final architecture.

### Task 5: Final verification

- [x] Build Debug and Release from the cleaned project files; Debug was also rebuilt during cleanup.
- [x] Run the full unit suite and all example smoke checks; inspect process exit codes and runtime logs.
- [x] Review the diff and scan for stale references and generated tracked artifacts.
- [x] Record concrete results and remaining limitations in the final response.

## Final verification (2026-11-15)

- Current solution: JamEngine, SandboxApp, JamTest, JamBenchmark. Later scene additions are preserved, including the implemented StencilScene and SkinningScene.
- Debug and Release: 359 active unit tests passed in each configuration. Previously excluded legacy test suites are explicitly documented in docs/Cleanup.md.
- Debug and Release: all 37 execution cases for the current 17 scenes passed (exit status, error logs, screenshots, and picking IDs).
- SSR, picking, and particle WARP regression scripts passed. Both benchmark executables ran successfully. Default launch from another directory and invalid CLI exit codes passed.
- Project/filter audit found no missing files or stale items. git diff --check passed. Representative PBR and skinning screenshots were inspected.
- Evidence: Build/Logs/ and Build/Validation/. Changes remain in the working tree without a new commit.
