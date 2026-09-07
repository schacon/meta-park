# Slide Engine

Write MDX. Build an Unreal game. Fly through your presentation over a shaded, low-poly Isla Nublar inspired by Jurassic Park’s UNIX interface.

A native Unreal Engine 5.8 slideshow engine with an eight-slide example adapted from the sibling [git-meta README](../git-meta/README.md). MDX is evaluated at build time; Unreal renders the resulting text and actors without a browser or JavaScript runtime.

## Run

Requires Node.js 22+, Unreal Engine **5.8**, and its supported C++ toolchain. On this Mac the default engine path is `/Users/Shared/Epic Games/UE_5.8`. Elsewhere set `UE_ROOT` to your engine installation.

```sh
npm install
npm run build             # MDX → unreal/Content/Slides/deck.json
npm test
npm run unreal:build      # Compile the native editor module
npm run unreal:prepare    # Create the entry map (first run only)
npm run unreal:play       # Open the island overview
```

To produce a standalone game:

```sh
npm run build
npm run unreal:package    # Build, cook and archive into dist/
```

Run `unreal:build` and `unreal:prepare` before the first package. The prepare step also creates the native material used by the retro landscape. Packaging targets the host platform. The packaged application loads the embedded deck and opens on the island overview. No sibling repository, Node installation or editor is needed to run the packaged game.

## Park workstation

The application opens on the island overview with every slide retracted. Click
**Next slide** (or press Right) to fly to the first habitat. Displays unfold as
the camera approaches and retract as it leaves. **O** returns to the map.

The sidebar shows the current slide and total, live scene counts, deployed
displays, and a countdown. `durationMinutes` in the layout sets the timer (20 by
default). It starts when you begin the tour; **P** or **Pause timer** pauses it.
The clock keeps running during an overview visit, unless paused.

The Mr. DNA–inspired sidebar mascot is a native 3D helix character, rendered by an
isolated scene-capture camera into a Slate image and continuously rotated.
The scene uses faceted trees, a volcano, lagoon and waterfall, white paddock
fences, entrance gate, visitor buildings, helipad and dock. Each habitat has one
cartoon dinosaur. The overview uses an orthographic camera to match the supplied
island-diorama mockup. The interface adds a workstation frame and subtle scanlines.

## Controls

| Key | Action |
| --- | --- |
| Right / Space / Page Down | Fly to the next slide |
| Left / Page Up | Fly to the previous slide |
| Home | Return to slide one |
| O | Bird's-eye view of the whole presentation |
| F | Toggle free flight; toggling off returns to the current slide |
| Mouse + W/A/S/D | Look and move in free flight |
| Q / E, Shift | Descend / ascend, move faster |
| N | Show current speaker notes for 20 seconds |
| P | Pause or resume the countdown |

## Author slides

The example lives in `examples/git-meta/deck.mdx`. Blank lines around Markdown inside components are significant MDX syntax.

```mdx
import {Deck, Slide, Model, Animate, Notes} from '../../src/components.jsx'

<Deck title="My talk">

<Slide id="hello" title="A world of ideas" accent="#563d72">

## CHAPTER ONE

Markdown paragraphs, **emphasis**, lists and fenced code.

<Animate kind="spin" speed={25}>
  <Model mesh="/Engine/BasicShapes/Cube.Cube" position={[0, 760, 0]} scale={[2, 2, 2]} />
</Animate>

<Notes>Only visible when the presenter presses N.</Notes>
</Slide>

</Deck>
```

Build another deck with `npm run build -- path/to/deck.mdx path/to/layout.json`; omit the layout argument to use automatic placement. The output replaces the active deck.

MDX uses the official compiler and supports JavaScript expressions, imports and reusable JSX function components. See `examples/git-meta/components.jsx` for composition examples. Imported components must ultimately produce the supported scene primitives or Markdown elements. Authoring code runs with Node permissions: compile decks you trust.

Supported Markdown: headings, paragraphs, lists, blockquotes and fenced code. Inline emphasis and links currently render as plain text; links are not interactive. HTML widgets, React hooks, browser APIs, images and tables are not supported. Unsupported block elements fail the build. This is a native scene component contract rather than a React DOM renderer.

## Models and animation

`Model` accepts `mesh` (Unreal object path), `position`, `rotation` and `scale`. Import custom meshes into `unreal/Content/Models` using the Unreal editor and copy their object paths:

```mdx
<Model mesh="/Game/Models/Robot.Robot" position={[0,760,0]} scale={[1,1,1]} />
```

For custom behavior, create an Actor Blueprint in the same folder. Its construction script, components, tick logic and timelines run natively:

```mdx
<Model actor="/Game/Models/BP_Robot.BP_Robot_C" />
```

The `Models` directory is always cooked so assets referenced only by JSON survive packaging. If you use another folder, add it to `DirectoriesToAlwaysCook` in `unreal/Config/DefaultGame.ini`. An unresolved asset produces an Unreal error log.

`Animate` wraps one or more models. `spin` rotates around yaw at `speed` degrees per second. `bob` moves on world Z by `amplitude` centimeters, with `speed` in radians per second. Motion runs continuously across the whole presentation. Nested animation wrappers are rejected; use Blueprint timelines for combined or event-driven motion.

## Island layout and flight route

The Git Meta example now uses a rough Isla Nublar map: ocean, sandy coastline,
mountain ridge, forest and eight fenced dinosaur habitats. The geography is a
creative approximation, not a canonical reconstruction of a film map.

Edit `examples/git-meta/layout.json`:

- `scene: "isla-nublar"` selects the island environment.
- `seed: 1993` deterministically shuffles the assignment of slides to habitats.
  Change the integer seed and run `npm run build` for another route.
- `habitats` defines each area's stable `id`, display `label`, `species`,
  `[x, y, groundHeight]` position and dinosaur color. Supply one per slide.
- Slide panels sit 1,550 cm above their habitat. `slides` can override individual
  positions, yaw, camera distance and transition time just as in a regular layout.
- `transition: 5` gives the camera time to climb and cross the island. Island
  flights arc upward and look down during the middle of the journey.

The slide narrative remains ordered; the geographic stops are shuffled. The
current route is T. rex → Stegosaurus → Velociraptor → Dilophosaurus → Gallimimus
→ Triceratops → Parasaurolophus → Brachiosaurus. North is world +Y. Press **O**
to see the island, habitats and shoreline together.

Dinosaur models are deliberately simple native shapes with different silhouettes.
Terrain and models receive sunlight, dynamic shadows and ambient occlusion;
the slide text stays readable in the original workstation-style windows.
The same seed also makes forest placement repeatable.

## Generic layout

Without overrides, slides follow a rising arc: radius 4,200 cm, angular step 0.48 radians, elevation step 380 cm. Each panel faces its own camera stop. Smoothstep movement and quaternion rotation blend between stops; a new navigation command starts from the camera's current transform.

For decks without the island scene, a layout can contain partial overrides:

```json
{
  "version": 1,
  "transition": 2.2,
  "slides": {
    "hello": {
      "position": [0, 0, 0],
      "yaw": 0,
      "cameraDistance": 1550,
      "transition": 3
    }
  }
}
```

Positions use Unreal centimeters: X forward, Y right, Z up. Panel-local +X faces the camera, +Y places models beside the panel. Rotation arrays are `[pitch, yaw, roll]` in degrees. Layout keys are stable slide IDs; overrides never change deck order. Unknown IDs, duplicate IDs, invalid vectors and nonpositive timing or camera distances fail compilation.

Panels are 1,440 × 900 units. Keep content concise; the first version does not paginate overflow automatically. Generic camera paths interpolate directly between stops. Island paths add altitude for terrain clearance; free flight and arbitrary custom layouts do not perform collision avoidance. Free flight makes the full spatial layout available to explore.

## Project structure

- `src/`: MDX scene components and compiler/validation.
- `examples/git-meta/`: eight-slide talk, reusable components and layout.
- `unreal/Source/SlideEngine/`: native panels, actors, animations and camera controls.
- `unreal/Content/Slides/deck.json`: compiled presentation, staged into the game.
- `scripts/`: MDX build, Unreal build, map creation and packaging.
- `test/`: compiler and layout behavior tests.

The sample describes the README's proposal, including its reported benchmarks. The git-meta project identifies `spec/README.md` as canonical for current semantics.

Implementation references: [MDX compiler](https://mdxjs.com/packages/mdx/) and [Unreal world-space widget components](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UWidgetComponent).

## Native integration check

After building and preparing, run `node scripts/unreal.mjs smoke`. This explicit
test mode visits every slide, checks the final camera position, writes screenshots
to `unreal/Saved/Screenshots/slide-01.png` through `slide-08.png`, and exits.
Normal play and packaged games never auto-advance. The test also captures `overview.png`.

Packaging includes Unreal's platform bundling step so the Mac app contains its
cooked content and native libraries. macOS is the platform exercised here; other host platforms still
need their own build and runtime verification.

## Preview

![Opening slide](docs/screenshots/slide-01.png)

![Bird's-eye view](docs/screenshots/overview.png)

See [verification notes](docs/verification.md) for tested behavior and limits.
