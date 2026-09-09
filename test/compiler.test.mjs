import {test} from 'node:test';
import assert from 'node:assert/strict';
import React from 'react';
import {compileDeck,makeManifest} from '../src/compiler.mjs';
const h=React.createElement;
const slide=(id='one',children=[])=>h('slide',{id,title:id},...children);
const deck=(...slides)=>h('deck',{title:'Test'},...slides);
test('example compiles seven visible slides and a hidden bonus and reusable animated MDX components',async()=>{
 const out=await compileDeck('examples/git-meta/deck.mdx','examples/git-meta/layout.json');
 assert.equal(out.slides.length,8);
 assert.equal(out.slides.filter(s=>!s.card.hidden).length,7);
 assert.equal(out.slides[7].card.hidden,true);
 assert.equal(out.durationSeconds,2100);
 assert.equal(out.slides[0].models.length,3);
 assert.equal(out.slides[0].models[0].animation.kind,'spin');
 assert.match(out.slides[3].blocks.find(b=>b.kind==='pre').text,/git meta set commit:/);
 assert.equal(out.slides[6].cameraDistance,2050);
 assert.equal('notes' in out.slides[6],false);
});
test('default route is deterministic and overrides do not reorder slides',()=>{
 const tree=deck(slide('one'),slide('two'));
 assert.deepEqual(makeManifest(tree),makeManifest(tree));
 const out=makeManifest(tree,{slides:{two:{position:[100,200,300],yaw:90}}});
 assert.deepEqual(out.slides.map(s=>s.id),['one','two']);
 assert.deepEqual(out.slides[1].position,[100,200,300]);
 assert.equal(out.slides[1].yaw,90);
});
test('rejects duplicates, stale overrides, invalid geometry and invalid animations',()=>{
 assert.throws(()=>makeManifest(deck(slide(),slide())),/duplicate/);
 assert.throws(()=>makeManifest(deck(slide()),{slides:{typo:{}}}),/unknown slide/);
 assert.throws(()=>makeManifest(deck(slide()),{slides:{one:{position:[0,NaN,0]}}}),/finite/);
 assert.throws(()=>makeManifest(deck(slide()),{transition:0}),/positive/);
 assert.throws(()=>makeManifest(deck(slide()),{durationMinutes:0}),/positive/);
 assert.equal(makeManifest(deck(slide()),{durationMinutes:7.5}).durationSeconds,450);
 assert.throws(()=>makeManifest(deck(slide('one',[h('animate',{kind:'unknown'},h('model'))]))),/animation/);
});
test('rejects unsupported DOM rather than silently dropping authored content',()=>{
 assert.throws(()=>makeManifest(deck(slide('one',[h('button',null,'click')]))),/Unsupported/);
});


test('island habitat shuffle is deterministic, complete, and preserves the narrative',async()=>{
 const {readFile}=await import('node:fs/promises');
 const {islandLayout}=await import('../src/island.mjs');
 const layout=JSON.parse(await readFile('examples/git-meta/layout.json','utf8'));
 const a=islandLayout(layout,8), b=islandLayout(layout,8);
 assert.deepEqual(a.route,b.route);
 assert.equal(new Set(a.route.map(h=>h.id)).size,layout.habitats.length);
 assert.notDeepEqual(a.route,layout.habitats);
 assert.notDeepEqual(a.route,islandLayout({...layout,seed:42},8).route);
 const out=await compileDeck('examples/git-meta/deck.mdx','examples/git-meta/layout.json');
 assert.deepEqual(out.slides.map(s=>s.id),['opening','context','targets','authoring','storage','merging','scale','beach-bar']);
 assert.deepEqual(out.slides.map(s=>s.habitat),a.route.slice(0,8).map(h=>h.id));
 for(let i=0;i<8;i++) assert.deepEqual(out.slides[i].position,[...a.route[i].position.slice(0,2),a.route[i].position[2]+3200]);
 assert.throws(()=>islandLayout({...layout,seed:-1},8),/seed/);
 assert.throws(()=>islandLayout({...layout,habitats:[]},8),/one habitat/);
 assert.throws(()=>islandLayout({...layout,habitats:Array(8).fill(layout.habitats[0])},8),/unique/);
});

test('every slide has one numbered destination, including all three facilities',async()=>{
 const {readFile}=await import('node:fs/promises');
 const layout=JSON.parse(await readFile('examples/git-meta/layout.json','utf8'));
 const out=await compileDeck('examples/git-meta/deck.mdx','examples/git-meta/layout.json');
 assert.deepEqual(out.slides.map(s=>s.card.code),['ENC-01','ENC-02','ENC-03','ENC-04','VC','HELI','GATE','BAR']);
 assert.equal(new Set(out.slides.map(s=>s.card.slide)).size,8);
 assert.equal(new Set(out.slides.map(s=>JSON.stringify(s.card.view.eye))).size,8);
 assert.equal(out.slides[7].card.hidden,true);
 assert.equal(out.slides[7].card.view.frameWidth,5500);
 for(const s of out.slides)assert.equal(s.card.view.path.length,2);
 for(const s of out.slides) {assert.equal(s.id,s.card.slide);assert.equal(s.transition,.8);}
 const tree=deck(...out.slides.map(s=>slide(s.id)));
 assert.throws(()=>makeManifest(tree,{...layout,cards:layout.cards.map((c,i)=>i?c:{...c,hidden:'yes'})}),/boolean/);
 assert.throws(()=>makeManifest(tree,{...layout,cards:layout.cards.map((c,i)=>i?c:{...c,view:{...c.view,path:[]}})}),/two control/);
 assert.throws(()=>makeManifest(tree,{...layout,cards:layout.cards.map((c,i)=>i?c:{...c,view:{...c.view,eye:[0,NaN,0]}})}),/finite/);
 assert.throws(()=>makeManifest(tree,{...layout,cards:layout.cards.slice(1)}),/count/);
 assert.throws(()=>makeManifest(tree,{...layout,cards:layout.cards.map((c,i)=>i?c:{...c,slide:'missing'})}),/unique known/);
 assert.throws(()=>makeManifest(tree,{...layout,cards:layout.cards.map((c,i)=>i?c:{...c,position:[0,NaN,0]})}),/finite/);
});
