import {build} from 'esbuild';
import {islandLayout} from './island.mjs';
import mdx from '@mdx-js/esbuild';
import {mkdtemp, rm, readFile} from 'node:fs/promises';
import {resolve, join} from 'node:path';
import {pathToFileURL} from 'node:url';

const fail = message => { throw new Error(message); };
const vector = (v, fallback, label) => {
  v ??= fallback;
  if (!Array.isArray(v) || v.length !== 3 || !v.every(Number.isFinite)) fail(`${label} must contain three finite numbers`);
  return v;
};
const positive = (v, fallback, label) => {
  v ??= fallback;
  if (!Number.isFinite(v) || v <= 0) fail(`${label} must be positive`);
  return v;
};
function expand(node) {
  if (node == null || typeof node === 'boolean') return [];
  if (Array.isArray(node)) return node.flatMap(expand);
  if (typeof node !== 'object') return [String(node)];
  if (typeof node.type === 'function') return expand(node.type(node.props));
  if (typeof node.type === 'symbol') return expand(node.props.children);
  return [{type:node.type, props:node.props, children:expand(node.props.children)}];
}
const plain = node => typeof node === 'string' ? node : node.children.map(plain).join('');
export function makeManifest(tree, layout = {}) {
  if (layout.scene !== undefined && !['gallery','isla-nublar'].includes(layout.scene)) fail('Unsupported scene');
  if (layout.version !== undefined && layout.version !== 1) fail('Unsupported layout version');
  const roots = expand(tree).filter(n => typeof n !== 'string' || n.trim());
  if (roots.length !== 1 || roots[0].type !== 'deck') fail('MDX must contain one Deck');
  const root = roots[0];
  const slides = root.children.filter(n => typeof n !== 'string' || n.trim());
  if (!slides.length || slides.some(n => n.type !== 'slide')) fail('Deck must contain Slide components');
  const island = islandLayout(layout, slides.length);
  const ids = new Set();
  const result = slides.map((s, i) => {
    const {id, title, accent = '#563d72'} = s.props;
    if (!id || typeof id !== 'string' || ids.has(id)) fail(`Missing or duplicate slide id: ${id}`);
    if (!title || typeof title !== 'string') fail(`Slide ${id} needs a title`);
    if (!/^#[\da-f]{6}$/i.test(accent)) fail(`Invalid accent on ${id}`);
    ids.add(id);
    const blocks = [], models = []; let notes = '';
    function visit(n, animation = null) {
      if (typeof n === 'string') { if (n.trim()) blocks.push({kind:'p', text:n.trim()}); return; }
      if (n.type === 'notes') { notes += plain(n); return; }
      if (n.type === 'animate') {
        if (animation) fail('Nested Animate components are unsupported');
        const {kind = 'spin', speed = 25, amplitude = 35} = n.props;
        if (!['spin','bob'].includes(kind) || !Number.isFinite(speed) || !Number.isFinite(amplitude)) fail('Invalid animation');
        n.children.forEach(c => visit(c, {kind,speed,amplitude})); return;
      }
      if (n.type === 'model') {
        const {mesh = '/Engine/BasicShapes/Cube.Cube', actor, position, rotation, scale} = n.props;
        if (typeof mesh !== 'string' || !mesh.startsWith('/') || (actor && (typeof actor !== 'string' || !actor.startsWith('/Game/')))) fail('Model needs an Unreal asset path');
        models.push({mesh, actor:actor ?? '', position:vector(position,[0,720,0],'Model position'), rotation:vector(rotation,[0,0,0],'Model rotation'), scale:vector(scale,[2,2,2],'Model scale'), animation}); return;
      }
      if (['h1','h2','h3','p','pre','li','blockquote'].includes(n.type)) { blocks.push({kind:n.type,text:plain(n)}); return; }
      if (['ul','ol'].includes(n.type)) { n.children.forEach(c => visit(c,animation)); return; }
      fail(`Unsupported MDX element: ${n.type}. Use Markdown or native scene components.`);
    }
    s.children.forEach(n => visit(n));
    const override = layout.slides?.[id] ?? {};
    const habitat = island?.route[i];
    const angle = i * 0.48;
    const position = vector(override.position,habitat ? [habitat.position[0],habitat.position[1],habitat.position[2]+1550] : [Math.round(4200*Math.sin(angle)),Math.round(4200*(1-Math.cos(angle))),i*380],`${id}.position`);
    return {id,title,accent,blocks,models,notes,position,habitat:habitat?.id??"",yaw: (()=>{const y=override.yaw??(island ? -90 : i*27.5); if(!Number.isFinite(y))fail('yaw must be finite'); return y;})(), cameraDistance:positive(override.cameraDistance,island ? 2050 : 1550,'cameraDistance'), transition:positive(override.transition,layout.transition??(island ? 5 : 2.2),'transition')};
  });
  for (const id of Object.keys(layout.slides??{})) if (!ids.has(id)) fail(`Layout references unknown slide: ${id}`);
  return {version:1,title:root.props.title??'Presentation',durationSeconds:positive(layout.durationMinutes,20,'durationMinutes')*60,scene:island?.scene??'gallery',seed:island?.seed??0,habitats:island?.habitats??[],slides:result};
}
export async function compileDeck(input, layoutFile) {
  // Bundle beside the project so React and local MDX imports resolve consistently.
  const temp = await mkdtemp(resolve('.slide-build-'));
  try {
    const outfile = join(temp,'deck.mjs');
    await build({entryPoints:[resolve(input)],outfile,bundle:true,platform:'node',format:'esm',plugins:[mdx()],packages:'external',logLevel:'silent'});
    const mod = await import(pathToFileURL(outfile).href);
    const layout = layoutFile ? JSON.parse(await readFile(layoutFile,'utf8')) : {};
    return makeManifest(mod.default({}),layout);
  } finally { await rm(temp,{recursive:true,force:true}); }
}
