// A build-time shuffle keeps slide content in order while changing the habitat route.
export function islandLayout(layout, count) {
  if (layout.scene !== 'isla-nublar') return null;
  if (!Number.isInteger(layout.seed) || layout.seed < 0 || layout.seed > 0xffffffff) throw new Error('Island seed must be an unsigned 32-bit integer');
  if (!Array.isArray(layout.habitats) || layout.habitats.length !== count) throw new Error('Island layout needs one habitat per slide');
  const ids = new Set();
  for (const h of layout.habitats) {
    if (!h.id || typeof h.id !== 'string' || ids.has(h.id)) throw new Error('Habitat IDs must be unique strings');
    ids.add(h.id);
    if (!h.label || typeof h.label !== 'string' || !['brachiosaurus','tyrannosaurus','triceratops','velociraptor','dilophosaurus','gallimimus','stegosaurus','parasaurolophus'].includes(h.species)) throw new Error('Invalid habitat label or dinosaur species');
    if (!Array.isArray(h.position) || h.position.length !== 3 || !h.position.every(Number.isFinite)) throw new Error('Habitat position needs three finite numbers');
    if (!/^#[\da-f]{6}$/i.test(h.color)) throw new Error('Invalid habitat color');
  }
  let state = layout.seed;
  const random = () => { state = (Math.imul(state,1664525)+1013904223)>>>0; return state/4294967296; };
  const route = [...layout.habitats];
  for (let i=route.length-1;i>0;i--) { const j=Math.floor(random()*(i+1)); [route[i],route[j]]=[route[j],route[i]]; }
  return {scene:'isla-nublar',seed:layout.seed,habitats:layout.habitats,route};
}
