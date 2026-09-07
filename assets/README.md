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
paths are authored around them. The sample currently reuses those four areas for
its eight slides. Only the selected slide panel deploys, preventing coincident
slides from overlapping.

Park statuses in the terminal callouts are illustrative. Dinosaur, enclosure,
tree, slide and deployed-display counts come from the running scene.

The workstation font is Roboto Mono Bold, distributed with its OFL license in
unreal/Content/Fonts. That directory is staged with packaged builds.
