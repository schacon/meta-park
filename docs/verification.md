# Verification — September 7, 2026

Tested on macOS with Unreal Engine 5.8 and Xcode 26.5 SDK.

- MDX compiler: four tests pass, including the eight-slide example, reusable JSX
  components, deterministic layouts, overrides and invalid-input rejection.
- Native editor and game C++ targets: compile successfully.
- BuildCookRun: build, cook, stage, package and archive complete successfully.
- Standalone archived app: launches without Unreal Editor and exits successfully
  after reaching all eight camera stops and the overview.
- Captured packaged-game screenshots reviewed for the opening slide and overview.
  The panel overlap is fixed; the opening slide text and footer are unobstructed.

The smoke test calls the same camera navigation method as the keyboard controls.
Manual keyboard and mouse interaction was not automated because the computer-use
connection was unavailable. Custom imported assets/Blueprints and non-Mac builds
have not been exercised; the sample uses reusable MDX components and engine meshes.

The overview sees the backs of some panels because slides face their individual
camera stops. The spatial path and platforms remain visible.
