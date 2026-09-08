# Park asset workflow

The source of the park is [blender/park.blend](blender/park.blend), authored through
Blender MCP. [blender/park-review.png](blender/park-review.png) is the latest render.
The exports directory contains independent FBX assets and a placement manifest;
Unreal imports them into /Game/Models/Park.

To rebuild the scene, execute this through Blender MCP, replacing the path with
your checkout:

    __file__ = '/path/to/slide-engine/scripts/build_park_assets.py'
    exec(compile(open(__file__).read(), __file__, 'exec'))

The script creates its own scene and preserves any existing Blender scene.
The saved library contains the current scene and named asset collections.
Individual parts remain editable; the export creates a temporary combined mesh
per collection, then removes only that temporary object.

Then run:

    npm run build
    npm run unreal:import-assets

After changing C++ code, also run npm run unreal:build. Package with
npm run unreal:package.

Blender coordinates are metres; the native placement manifest multiplies them by
100 for Unreal centimetres. The FBX exporter bakes a Y reflection before Unreal's
handedness conversion, so imported meshes match the layout coordinates. Linear
vertex colors avoid a second gamma conversion; the native material is two-sided
so the folded palm fronds render from both directions. M_ParkPaint uses vertex
alpha as an emission mask, lighting only the gate torch flames.

The layout file controls the four paddock locations and elevations.
Rebuild the Blender scene after changing those locations: terrain plateaus and
paths are authored around them. The sample maps seven MDX slides to four paddocks,
the visitor centre, helipad and main gate. Card locations are declared separately
in the layout's cards list and can be rebuilt with npm run build.

Park statuses in the terminal callouts are illustrative. Dinosaur, enclosure,
tree, slide and deployed-display counts come from the running scene.

The workstation font is Roboto Mono Bold, distributed with its OFL license in
unreal/Content/Fonts. That directory is staged with packaged builds.

Dinosaurs wander in the native runtime; the source Blender poses stay editable.
The articulated kit lives in `blender/dinosaur-rigs.blend`, with an editable joint
hierarchy for each active species. Rebuild it with:

    /Applications/Blender.app/Contents/MacOS/Blender --background --python scripts/build_dinosaur_rigs.py

`npm run unreal:import-assets` also imports these 40 body/limb meshes and stages
`dinosaur-rigs.json`. Upper-leg, lower-leg, and foot meshes have joint-centered
origins. The native two-bone IK solver keeps supporting feet planted in world
space, and lifts each foot during recovery. The paint material no longer deforms
leg vertices. For a walking review capture:

    node scripts/unreal.mjs smoke -DinoWalkReview=3 -SlideTestWidth=1440 -SlideTestHeight=900

This captures a 16-second close view of the selected paddock at eight frames per
second in `unreal/Saved/Screenshots/walk-3-*.png`. Use 1, 2, or 4 for the other pens.
