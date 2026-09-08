# Park diorama

The user's supplied isometric island mockup is the visual reference. The park is
original Blender geometry with painted vertex colors, imported into Unreal.

The current composition has three irregular paddocks with chunky posts and a
compact raptor pen. The pen has concrete perimeter walls, eight shared upper
buttresses (four on each long side, two on each short side), a watchtower,
gatehouse, and three small raptors. An expanded island terrace supports the
entire pen and blends into the surrounding slopes. Ground elevations vary between paddocks;
paths connect their gates, the visitor centre, helipad and main entrance without
overlapping. Broadleaf trees have several silhouettes and a wide size range.

The volcano sits above a rocky waterfall and recessed lagoon. The shoreline is
part of the terrain, sloping through a narrow sand band to the water. Two rounded
offshore islands and a large faceted ocean boulder have shallow turquoise shelves.
The entrance sign reads “git-meta park” in extruded red letters with gold rims
and dark sides; its ramp connects to a simple T-shaped
dock and boat.

A top-aligned gray terminal sidebar uses regular monospace text and seven clickable slide locations, green utility bars, park counts,
and a presentation clock. Its footer reads “GIT FINDS A WAY.”

The overview centers the island within the scene viewport beside the sidebar.
Grounding dots and leader lines render behind all card backgrounds.

Seven numbered map cards correspond one-to-one with seven MDX slides: four
paddocks, visitor centre, helipad and main gate. Numbers 1–7, keypad numbers, and
card or sidebar clicks select an area. Each destination has an authored elevated camera position and sign orientation.
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
Escape cancels pending destinations. Left/right return to overview and wait for a second press
before visiting the adjacent slide, wrapping around the seven areas. Number and sidebar selections
continue directly through overview to their chosen destination.

The top bar identifies git-meta and the park monitoring system, with a live local
clock. Gate flames have subtle shape and brightness flicker. The waterfall has
small flowing color bands and surface motion, with larger rounded splash lobes that pulse in size at its base. Red beacons
on the eight raptor-pen buttresses blink in a repeating warning cycle.
Those animations are confined to the relevant geometry using material masks.
