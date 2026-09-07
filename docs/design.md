# Park diorama

The user's supplied isometric island mockup is the visual reference. The park is
original Blender geometry with painted vertex colors, imported into Unreal.

The current composition has four irregular paddocks, each with twelve chunky
posts, two rails and a visible gate. Ground elevations vary between paddocks;
paths connect their gates, the visitor centre, helipad and main entrance without
overlapping. Broadleaf trees have several silhouettes and a wide size range.

The volcano sits above a rocky waterfall and recessed lagoon. The shoreline is
part of the terrain, sloping through a narrow sand band to the water. Two rounded
offshore islands and a large faceted ocean boulder have shallow turquoise shelves.
The entrance sign reads “git-meta park”; its ramp connects to a simple T-shaped
dock and boat.

A gray terminal sidebar uses bold monospace text, green utility bars, park counts,
and a presentation clock. Its footer reads “GIT FINDS A WAY.”

Seven numbered map cards correspond one-to-one with seven MDX slides: four
paddocks, visitor centre, helipad and main gate. Numbers 1–7, keypad numbers, and
card clicks select an area. Each destination has an authored elevated camera position and sign orientation.
Two fixed Bézier control points give every approach its own swoop, with a subtle
bank during flight. Dinosaur views align with the fence gates; the visitor
centre view faces its door, the entrance view faces the main gate, and the helipad
view looks slightly down from its approach path. An explicit subject target keeps
the camera aimed at that landmark while the sign rises into the same sightline. Its field of view and camera distance change together, avoiding
an abrupt switch from orthographic to perspective projection.

After arrival, an angled world-space sign rises from a ground plinth on twin
supports. The sign is blank while moving. A short Unix-style loading prompt
precedes its slide content and native component models. Leaving clears the
content, retracts the sign, and returns through the overview before the next stop.
Escape cancels pending destinations; left/right wrap around the seven areas.

The top bar identifies git-meta and the park monitoring system, with a live local
clock. Gate flames have subtle shape and brightness flicker. The waterfall has
small flowing color bands and surface motion, with gently moving foam at its base.
Those animations are confined to the relevant geometry using material masks.
