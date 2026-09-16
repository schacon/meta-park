import {loadSlideImage} from './slide-image.mjs';
import {speedGraph} from './speed-graph.mjs';
import {exchangeDemo} from './exchange.mjs';
import {scalarDemo} from './scalar.mjs';
import {serializeValues} from './serializer.mjs';
import {build} from 'esbuild';
import {loadCast} from './asciicast.mjs';
import {islandLayout} from './island.mjs';
import mdx from '@mdx-js/esbuild';
import {mkdtemp, rm, readFile, readdir} from 'node:fs/promises';
import {resolve, join} from 'node:path';
import {pathToFileURL} from 'node:url';
import {createElement} from 'react';

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
  if (node.type===components.Value) return [{type:'value',props:{...node.props,key:node.key},children:expand(node.props.children)}];
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
  if (layout.cards !== undefined) {
    if (!island || !Array.isArray(layout.cards) || layout.cards.length !== slides.length) fail('Cards must match the island slide count');
    const cardIds = new Set(), codes = new Set();
    for (const card of layout.cards) {
      if (!slides.some(s => s.props.id === card.slide) || cardIds.has(card.slide)) fail('Each card must reference a unique known slide');
      if (![card.code,card.label,card.status].every(v=>typeof v==='string' && v.length>0) || codes.has(card.code)) fail('Cards need unique codes, labels and statuses');
      vector(card.position,undefined,'Card position');
      if(card.hidden!==undefined&&typeof card.hidden!=='boolean')fail('Card hidden must be a boolean');
      if(card.view) {
        vector(card.view.anchor,undefined,'Sign anchor'); vector(card.view.eye,undefined,'Camera eye');
        if(card.view.look!==undefined)vector(card.view.look,undefined,'Camera subject');
        if(!Array.isArray(card.view.path)||card.view.path.length!==2)fail('Flight path needs two control points');
        card.view.path.forEach(p=>vector(p,undefined,'Flight path'));
        if(!Number.isFinite(card.view.signYaw))fail('Sign yaw must be finite');
        positive(card.view.signHeight,1100,'Sign height');
        if(card.view.frameWidth!==undefined)positive(card.view.frameWidth,4500,'Camera frame width');
      }
      cardIds.add(card.slide); codes.add(card.code);
    }
  }
  const ids = new Set();
  const result = slides.map((s, i) => {
    const {id, title, accent = '#563d72'} = s.props;
    if (!id || typeof id !== 'string' || ids.has(id)) fail(`Missing or duplicate slide id: ${id}`);
    if (!title || typeof title !== 'string') fail(`Slide ${id} needs a title`);
    if (!/^#[\da-f]{6}$/i.test(accent)) fail(`Invalid accent on ${id}`);
    ids.add(id);
    const blocks = [], models = [];
    function visit(n, animation = null, listDepth = 0) {
      if (typeof n === 'string') { if (n.trim()) blocks.push({kind:'p', text:n.trim()}); return; }
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
      if (n.type === 'li') {
        const nested = n.children.filter(c => typeof c !== 'string' && ['ul','ol'].includes(c.type));
        const text = n.children.filter(c => !nested.includes(c)).map(plain).join('').trim();
        if (text) blocks.push({kind:'li',text,...(listDepth>1?{depth:listDepth-1}:{})});
        nested.forEach(c => visit(c,animation,listDepth));
        return;
      }
      if (['h1','h2','h3','p','pre','blockquote'].includes(n.type)) { blocks.push({kind:n.type,text:plain(n)}); return; }
      if (['ul','ol'].includes(n.type)) { n.children.filter(c=>typeof c!=='string'||c.trim()).forEach(c => visit(c,animation,listDepth+1)); return; }
      fail(`Unsupported MDX element: ${n.type}. Use Markdown or native scene components.`);
    }
    s.children.forEach(n => visit(n));
    const override = layout.slides?.[id] ?? {};
    const card = layout.cards?.find(c => c.slide === id);
    const habitat = island?.route[i];
    const routeCard=card??(habitat?{slide:id,code:`AREA-${i+1}`,label:habitat.label,status:'OK',position:habitat.position}:null);
    const nativeCard=routeCard?{...routeCard,view:routeCard.view??{
      anchor:[routeCard.position[0],routeCard.position[1]-2200,0],
      eye:[routeCard.position[0]+2400,routeCard.position[1]-5000,routeCard.position[2]+1500],
      signYaw:-45,signHeight:1100,
      path:[[routeCard.position[0]*1.5,-26000,30000],[routeCard.position[0]+6000,routeCard.position[1]-7000,7000]]
    }}:null;
    const angle = i * 0.48;
    const position = vector(override.position,habitat ? [habitat.position[0],habitat.position[1],habitat.position[2]+3200] : [Math.round(4200*Math.sin(angle)),Math.round(4200*(1-Math.cos(angle))),i*380],`${id}.position`);
    return {id,title,accent,blocks,models,position,card:nativeCard,habitat:habitat?.id??"",yaw: (()=>{const y=override.yaw??(island ? -90 : i*27.5); if(!Number.isFinite(y))fail('yaw must be finite'); return y;})(), cameraDistance:positive(override.cameraDistance,island ? 2050 : 1550,'cameraDistance'), transition:positive(override.transition,layout.transition??(island ? 5 : 2.2),'transition')};
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

// MDX pages use this native component registry without needing import statements.
const components = Object.fromEntries(['Model','Animate','Computer','CommandLine','FSV','Exchange','SpeedGraph','Scalar','Image','Serializer','Value','ColdStorage','Species','Description','RaptorWarning','Raptors','Raptor','Label','Problems','Problem'].map(name =>
  [name, props => createElement(name.toLowerCase(), props)]));
async function readPage(input) {
  const temp = await mkdtemp(resolve('.slide-build-'));
  try {
    const outfile = join(temp,'page.mjs');
    await build({entryPoints:[resolve(input)],outfile,bundle:true,platform:'node',format:'esm',plugins:[mdx()],packages:'external',logLevel:'silent'});
    return expand((await import(pathToFileURL(outfile).href)).default({components}));
  } finally { await rm(temp,{recursive:true,force:true}); }
}
const meaningful = nodes => nodes.filter(n => typeof n !== 'string' || n.trim());
const element = n => typeof n === 'string' ? n : createElement(n.type,n.props,...n.children.map(element));
export async function compileStations(directory, layoutFile, options = {}) {
  const layout = JSON.parse(await readFile(layoutFile,'utf8'));
  if (!Array.isArray(layout.cards) || !layout.cards.length) fail('Station layout needs cards');
  const stations = [];
  for (const [index, card] of layout.cards.entries()) {
    const folder = join(directory,String(index+1).padStart(2,'0'));
    const names = (await readdir(folder)).filter(n => /\.mdx$/i.test(n))
      .sort((a,b)=>a.localeCompare(b,'en',{numeric:true}) || a.localeCompare(b));
    if (!names.length) fail(`${folder}: station needs at least one MDX file`);
    const steps = [];
    for (const name of names) {
      const source = join(folder,name);
      try {
        const nodes = meaningful(await readPage(source));
        const visible = nodes;
        if (!visible.length) fail('Page needs content');
        const imagePage=visible.length===1&&visible[0].type==='image';
        if(imagePage&&meaningful(visible[0].children).length)fail('Image does not accept children');
        const image=imagePage?await loadSlideImage(visible[0].props,source):null;
        const standalone = visible.length===1 && ['computer','commandline','fsv','exchange','speedgraph','scalar','serializer','coldstorage','raptorwarning','raptors','model','animate'].includes(visible[0].type);
        let component = null;
        if (standalone && visible[0].type==='speedgraph') {
          if(meaningful(visible[0].children).length)fail('SpeedGraph does not accept children');
          component=speedGraph(visible[0].props);
        }
        if (standalone && visible[0].type==='exchange') {
          if(meaningful(visible[0].children).length)fail('Exchange does not accept children');
          component=exchangeDemo(visible[0].props);
        }
        if (standalone && visible[0].type==='scalar') {
          if(meaningful(visible[0].children).length)fail('Scalar does not accept children');
          component=scalarDemo(visible[0].props);
        }
        if (standalone && visible[0].type==='serializer') {
          const values=meaningful(visible[0].children).map(n=>{
            if(n.type!=='value'||meaningful(n.children).length)fail('Serializer only accepts self-closing Value entries');
            return n.props;
          });
          component=serializeValues(values,visible[0].props);
        }
        if (standalone && visible[0].type==='coldstorage') {
          const canisters=meaningful(visible[0].children).map(item=>{
            if(item.type!=='canister')fail('ColdStorage only accepts canister children');
            const fields=meaningful(item.children);
            if(fields.some(n=>!['species','description','label'].includes(n.type))||fields.filter(n=>n.type==='label').length>1||fields.filter(n=>n.type==='species').length!==1||fields.filter(n=>n.type==='description').length!==1)fail('Each ColdStorage canister needs one Species and one Description');
            const species=plain(fields.find(n=>n.type==='species')).trim(),description=plain(fields.find(n=>n.type==='description')).trim();
            if(!species||!description)fail('ColdStorage species and descriptions cannot be empty');
            const labelNode=fields.find(n=>n.type==='label'),label=labelNode?plain(labelNode).trim():null;
            if(labelNode&&!label)fail('ColdStorage labels cannot be empty');
            return {species,description,...(label?{label}:{})};
          });
          if(canisters.length!==4)fail('ColdStorage needs exactly four canisters');
          component={type:'ColdStorage',canisters};
        }
        if (standalone && visible[0].type==='raptorwarning') {
          const title=plain(visible[0]).trim();
          if(!title)fail('RaptorWarning needs warning text');
          component={type:'RaptorWarning',title};
        }
        if (standalone && visible[0].type==='raptors') {
          const raptors=meaningful(visible[0].children).map(item=>{
            if(item.type!=='raptor')fail('Raptors only accepts Raptor children');
            const fields=meaningful(item.children);
            if(fields.length!==2||fields.filter(n=>n.type==='label').length!==1||fields.filter(n=>n.type==='problems').length!==1)fail('Each Raptor needs one Label and one Problems list');
            const label=plain(fields.find(n=>n.type==='label')).trim();
            const problems=meaningful(fields.find(n=>n.type==='problems').children).map(n=>{
              if(n.type!=='problem'||!plain(n).trim())fail('Raptor Problems needs nonempty Problem children');
              return plain(n).trim();
            });
            if(!label||!problems.length)fail('Each Raptor needs a label and at least one problem');
            const workers=item.props.workers;
            if(workers!==undefined&&(!Number.isInteger(workers)||workers<0||workers>8))fail('Raptor workers must be an integer from 0 to 8');
            return {label,problems,...(workers!==undefined?{workers}:{})};
          });
          if(raptors.length!==4)fail('Raptors needs exactly four Raptor entries for the pen');
          component={type:'Raptors',raptors};
        }
        if (standalone && visible[0].type==='fsv') {
          const title=visible[0].props.title;
          if(typeof title!=='string'||!title.trim())fail('FSV needs a title');
          const systems=meaningful(visible[0].children).map(system=>{
            if(system.type!=='system')fail('FSV only accepts system children');
            const fields=meaningful(system.children);
            if(fields.length!==2||fields.filter(n=>n.type==='label').length!==1||fields.filter(n=>n.type==='meta').length!==1)fail('Each FSV system needs one label and one meta description');
            const label=plain(fields.find(n=>n.type==='label')),meta=plain(fields.find(n=>n.type==='meta'));
            if(!label.trim()||!meta.trim())fail('FSV labels and meta descriptions cannot be empty');
            return {label,meta};
          });
          if(!systems.length)fail('FSV needs at least one system');
          component={type:'FSV',title,systems};
        }
        if (standalone && visible[0].type==='commandline') {
          if(meaningful(visible[0].children).length)fail('CommandLine uses a cast file instead of prompt/output children');
          const {src,cast,speed,idleTimeLimit}=visible[0].props;
          if(src&&cast)fail('CommandLine accepts either src or cast, not both');
          const recording=src??cast;
          component={type:'CommandLine',src:recording,cast:await loadCast(recording,{castDirectory:options.castDirectory,speed,idleTimeLimit})};
        }
        if (standalone && visible[0].type==='computer') {
          const children = meaningful(visible[0].children);
          if (children.length!==2 || children.filter(n=>n.type==='prompt').length!==1 || children.filter(n=>n.type==='output').length!==1)
            fail('Computer needs exactly one prompt and one output');
          component = {type:visible[0].type==='computer'?'Computer':'CommandLine',prompt:plain(children.find(n=>n.type==='prompt')),output:plain(children.find(n=>n.type==='output'))};
          if (!component.prompt.trim()) fail('Computer prompt cannot be empty');
        }
        const heading = visible.find(n=>['h1','h2','h3'].includes(n.type));
        const title = heading ? plain(heading) : image ? image.alt : component ? component.type : name.replace(/\.mdx$/i,'');
        const body = nodes.filter(n=>n!==heading && !((component||image) && n===visible[0]));
        const page = makeManifest(createElement('deck',{},createElement('slide',{id:card.slide,title},...body.map(element)))).slides[0];
        steps.push({id:name.replace(/\.mdx$/i,''),source,kind:standalone?'component':'slide',component:component??(standalone?{type:'Scene'}:null),title,...(image?{image}:{}),...(visible.length===1&&visible[0].type==='h1'?{titleOnly:true}:{}),blocks:page.blocks,models:page.models});
      } catch (error) { throw new Error(`${source}: ${error.message}`,{cause:error}); }
    }
    stations.push({card,steps});
  }
  const deck = makeManifest(createElement('deck',{title:'git-meta park'},...stations.map(({card,steps})=>
    createElement('slide',{id:card.slide,title:steps[0].title}))),layout);
  deck.slides.forEach((station,i)=>Object.assign(station,stations[i].steps[0],{id:station.id,steps:stations[i].steps}));
  return deck;
}
