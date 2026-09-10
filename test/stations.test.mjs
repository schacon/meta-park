import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,writeFile,rm,readFile} from 'node:fs/promises';
import {join} from 'node:path';
import {tmpdir} from 'node:os';
import {compileStations} from '../src/compiler.mjs';
async function fixture(t) {
 const dir=await mkdtemp(join(tmpdir(),'station-mdx-'));
 t.after(()=>rm(dir,{recursive:true,force:true}));
 for(let i=1;i<=8;i++) {
  const folder=join(dir,String(i).padStart(2,'0'));await mkdir(folder);
  await writeFile(join(folder,'01-intro.mdx'),`# Station ${i}\n\n- First point\n- Second point\n`);
 }
 await mkdir(join(dir,'casts'));
 return dir;
}
const layout='examples/git-meta/layout.json';
test('station pages preserve camera layout, extract titles and sort filenames numerically',async t=>{
 const dir=await fixture(t);
 await writeFile(join(dir,'01/10-later.mdx'),'## Later\n\nLast page');
 await writeFile(join(dir,'02/02-terminal.mdx'),'<CommandLine cast="demo.cast" />');
 await writeFile(join(dir,'01/2-example.mdx'),'<Computer>\n<prompt>git meta get commit:HEAD owner</prompt>\n<output>s.chacon</output>\n</Computer>');
 await writeFile(join(dir,'casts/demo.cast'),JSON.stringify({version:2,width:60,height:16})+'\n'+JSON.stringify([.1,'o','\x1b[32mthree entries\x1b[0m']));
 const deck=await compileStations(dir,layout,{castDirectory:join(dir,'casts')});
 assert.equal(deck.slides.length,8);
 const station=deck.slides[0];
 assert.equal(station.card.code,'RC');assert.equal(station.id,'storage');assert.equal(station.title,'Station 1');
 assert.deepEqual(station.steps.map(p=>p.id),['01-intro','2-example','10-later']);
 assert.deepEqual(station.steps[0].blocks,[{kind:'li',text:'First point'},{kind:'li',text:'Second point'}]);
 assert.deepEqual(station.steps[1].component,{type:'Computer',prompt:'git meta get commit:HEAD owner',output:'s.chacon'});
 assert.equal(station.steps[1].kind,'component');assert.deepEqual(station.steps[1].blocks,[]);
 assert.equal(station.steps[2].title,'Later');assert.equal(station.steps[2].kind,'slide');
 assert.deepEqual(station.card.view,JSON.parse(await readFile(layout)).cards[0].view);
 assert.equal(deck.slides[7].card.hidden,true);
 assert.equal(deck.slides[1].steps[1].component.type,'CommandLine');
 assert.equal(deck.slides[1].steps[1].component.cast.frames.at(-1).lines[0][1][0][2],'three entries');
});
test('native scene components work alone or with Markdown',async t=>{
 const dir=await fixture(t);
 await writeFile(join(dir,'01/02-model.mdx'),'<Animate kind="bob"><Model /></Animate>');
 await writeFile(join(dir,'01/03-mixed.mdx'),'# Models\n\nA rotating cube.\n\n<Animate><Model /></Animate>');
 const steps=(await compileStations(dir,layout,{castDirectory:join(dir,'casts')})).slides[0].steps;
 assert.equal(steps[1].component.type,'Scene');assert.equal(steps[1].models[0].animation.kind,'bob');
 assert.equal('notes' in steps[1],false);
 assert.equal(steps[2].kind,'slide');assert.equal(steps[2].models.length,1);
});
test('invalid native components and empty stations fail with an actionable source path',async t=>{
 const dir=await fixture(t);const file=join(dir,'01/02-bad.mdx');
 await writeFile(file,'<Computer><prompt>hello</prompt></Computer>');
 await assert.rejects(compileStations(dir,layout,{castDirectory:join(dir,'casts')}),/02-bad.mdx: Computer needs/);
 await writeFile(file,'# Mixed\n\n<Computer><prompt>hello</prompt><output>OK</output></Computer>');
 await assert.rejects(compileStations(dir,layout,{castDirectory:join(dir,'casts')}),/02-bad.mdx: Unsupported MDX element: computer/);
 await writeFile(file,'<CommandLine><prompt>old</prompt><output>OK</output></CommandLine>');
 await assert.rejects(compileStations(dir,layout,{castDirectory:join(dir,'casts')}),/uses a cast file/);
 await writeFile(file,'<CommandLine src="missing.cast" />');
 await assert.rejects(compileStations(dir,layout,{castDirectory:join(dir,'casts')}),/02-bad.mdx:.*ENOENT/);
 await writeFile(file,'<Unknown />');
 await assert.rejects(compileStations(dir,layout,{castDirectory:join(dir,'casts')}),/02-bad.mdx:.*Unknown/);
 await rm(file);await rm(join(dir,'01/01-intro.mdx'));
 await assert.rejects(compileStations(dir,layout,{castDirectory:join(dir,'casts')}),/01: station needs/);
});

test('FSV serializes ordered systems and title-only detection requires exactly one h1',async t=>{
 const dir=await fixture(t),file=join(dir,'02/02-viewer.mdx');
 await writeFile(join(dir,'02/01-intro.mdx'),'# New Metadata Use Cases');
 await writeFile(file,'<FSV title="Systems"><system><label>trust</label><meta>identity, signoffs</meta></system><system><label>review</label><meta>comments</meta></system></FSV>');
 const steps=(await compileStations(dir,layout)).slides[1].steps;
 assert.equal(steps[0].titleOnly,true);
 assert.deepEqual(steps[1].component,{type:'FSV',title:'Systems',systems:[{label:'trust',meta:'identity, signoffs'},{label:'review',meta:'comments'}]});
 assert.equal(steps[1].kind,'component');assert.deepEqual(steps[1].blocks,[]);
 await writeFile(file,'# Title\n\nSome body');
 assert.equal((await compileStations(dir,layout)).slides[1].steps[1].titleOnly,undefined);
 await writeFile(file,'## Heading two');
 assert.equal((await compileStations(dir,layout)).slides[1].steps[1].titleOnly,undefined);
 for(const mdx of ['<FSV />','<FSV title="Empty" />','<FSV title="Bad"><system><label>trust</label></system></FSV>']){
  await writeFile(file,mdx);await assert.rejects(compileStations(dir,layout),/02-viewer.mdx:.*FSV/);
 }
});

test('cold storage and raptor records compile authored data with physical station order',async t=>{
 const dir=await fixture(t),cold=join(dir,'01/02-cold.mdx'),raptors=join(dir,'03/02-raptors.mdx');
 await writeFile(cold,await readFile('examples/git-meta/cold-storage.mdx','utf8'));
 await writeFile(join(dir,'03/01-intro.mdx'),'<RaptorWarning>Keep your hands inside the vehicle</RaptorWarning>');
 await writeFile(raptors,(await readFile('examples/git-meta/raptors.mdx','utf8')).replace('<Raptor>','<Raptor workers={7}>'));
 const deck=await compileStations(dir,layout),c=deck.slides[0].steps[1].component,r=deck.slides[2];
 assert.equal(c.type,'ColdStorage');assert.equal(c.canisters.length,4);assert.equal(c.canisters[0].species,'granularity');assert.match(c.canisters[1].description,/millions of keys/);
 assert.equal(r.card.code,'ENC-04');assert.deepEqual(r.steps[0].component,{type:'RaptorWarning',title:'Keep your hands inside the vehicle'});
 assert.equal(r.steps[1].component.raptors.length,4);assert.equal(r.steps[1].component.raptors[0].workers,7);assert.equal(r.steps[1].component.raptors[0].problems.length,2);
 const originalCold=await readFile(cold,'utf8');
 await writeFile(cold,originalCold.replace('<Species>granularity</Species>','<Label>granularity</Label><Species>Granulosaurus</Species>'));
 const labeled=(await compileStations(dir,layout)).slides[0].steps[1].component.canisters[0];
 assert.equal(labeled.label,'granularity');assert.equal(labeled.species,'Granulosaurus');
 await writeFile(cold,originalCold.replace('<Species>granularity</Species>','<Label> </Label><Species>Granulosaurus</Species>'));
 await assert.rejects(compileStations(dir,layout),/labels cannot be empty/);
 for(const mdx of ['<ColdStorage />','<ColdStorage><canister><Species>Empty</Species></canister></ColdStorage>']){await writeFile(cold,mdx);await assert.rejects(compileStations(dir,layout),/02-cold.mdx:.*ColdStorage/);}
 await rm(cold);
 for(const mdx of ['<Raptors />','<Raptors><Raptor><Label>Empty</Label><Problems /></Raptor></Raptors>','<Raptors><Raptor workers={20}><Label>X</Label><Problems><Problem>X</Problem></Problems></Raptor></Raptors>']){await writeFile(raptors,mdx);await assert.rejects(compileStations(dir,layout),/02-raptors.mdx:.*Raptor/);}
});
