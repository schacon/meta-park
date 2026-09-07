# Verification — September 7, 2026

Tested on macOS with Unreal Engine 5.8 and Xcode 26.5 SDK.

- MDX compiler: five tests pass, including the eight-slide example, reusable JSX
  components, deterministic layouts, overrides, and invalid-input rejection.
- Native editor and game C++ targets compile successfully.
- Blender MCP rebuilt 21 independently exported mesh assets. Unreal imports their
  authored colors, normals, and torch emission into the packaged scene.
- BuildCookRun build, cook, stage, package, and archive complete successfully.
- Packaged overview reviewed for the angular coastline, wide shallow-water bands,
  stepped visitor center, thick helipad, gate lettering, and revised terminal sidebar.
  The bundled bold monospace font loads correctly, the entire footer is visible,
  and the DNA character is removed.

Editor and standalone packaged smoke tests passed all eight camera stops, raised
flights, initially retracted panels, and return to the overview. The four paddocks are shared by the
sample's eight slides; only the selected slide deploys.

Manual keyboard and mouse interaction was not automated. Non-Mac builds have not
been exercised. Visitor/staff numbers, utilities, and weather are illustrative;
dinosaur/enclosure counts and presentation timing come from the running scene.
