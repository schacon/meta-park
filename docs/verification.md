# Verification — September 7, 2026

Tested on macOS with Unreal Engine 5.8 and Xcode 26.5 SDK.

- Six MDX/compiler tests pass, including the seven-slide example, reusable JSX
  components, deterministic layouts, invalid-input rejection, and one-to-one card
  assignments for the four paddocks and three facilities.
- Native editor and game C++ targets compile successfully.
- Blender's 21 imported mesh assets remain in the native scene.
- BuildCookRun build, cook, stage, package and archive succeeded.
- Editor and packaged integration checks passed. The smoke test opens every
  numbered card, switches with number and
  arrow commands, interrupts a zoom, and cancels the queued destination with Escape.
  It checks camera position, arrival before card expansion, an unobstructed flight,
  and continuous perspective projection throughout the transitions. Each arrival
  must match its authored camera position and look direction, with the physical
  sign fully raised.
- Screenshots cover the numbered overview, expanded slides, mid-zoom card hiding,
  all seven subject-facing arrivals (dinosaurs through their gateways, visitor
  door, main gate, and helipad), a content-free loading prompt, and cards reappearing near
  the end of the overview return.

The terminal top bar includes a live local clock. Gate flames use masked
material flicker and movement; the waterfall uses flowing color bands and gentle
surface/foam movement. The weather section and DNA character are removed. The sidebar footer reads
“GIT FINDS A WAY”; the slide count and countdown remain visible.

Manual keyboard/mouse events were not automated: the smoke test invokes the same
key handler used by runtime input. Non-Mac builds have not been exercised.
Visitor/staff numbers and utility levels are illustrative; dinosaur/enclosure
counts and presentation timing come from the running scene.
