import test from 'node:test';
import assert from 'node:assert/strict';
import {speedGraph} from '../src/speed-graph.mjs';
import {compileStations} from '../src/compiler.mjs';
test('speed comparison separates setup completion from background indexing',()=>{
 const setup=speedGraph(),total=speedGraph({metric:'total'});
 assert.deepEqual(setup.rows.map(r=>r.time),['12 s','64 s','444 s','91 s']);
 assert.deepEqual(total.rows.map(r=>r.seconds),[12.8,79.5,553,93]);
 assert.equal(setup.rows[2].seconds,444);
 assert.deepEqual(total.rows[2].phases.map(p=>p.seconds),[306,79,51,7,109]);
 assert.equal(total.rows[3].phases[2].seconds,0);
 assert.equal(setup.rows[0].phases[4].inNeck,false);
 assert.equal(total.rows[0].phases[4].inNeck,true);
 assert.equal(setup.rows[3].label,'5M shallow');
 assert.ok(setup.rows.every((r,i)=>r.seconds<total.rows[i].seconds));
 assert.throws(()=>speedGraph({metric:'fetch'}),/metric must be/);
});
test('the authored dinosaur screen follows Scalar as a physical component',async()=>{
 const deck=await compileStations('slides','examples/git-meta/layout.json');
 const steps=deck.slides[6].steps;
 const scalar=steps.findIndex(s=>s.component?.type==='Scalar');
 assert.ok(scalar>=0);assert.equal(steps[scalar+1].component.type,'SpeedGraph');
 assert.equal(steps[scalar+1].kind,'component');assert.equal(steps[scalar+1].component.rows.length,4);
});
