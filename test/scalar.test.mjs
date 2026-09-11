import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,writeFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {scalarDemo} from '../src/scalar.mjs';
import {compileStations} from '../src/compiler.mjs';
test('Scalar keeps full history for User 2 and only the shallow tip for User 3',()=>{
 const {steps,commits}=scalarDemo();
 assert.deepEqual(commits.map(c=>c.entries),[[0,1],[0,1,2,3],[2,3],[2,3,4]]);
 assert.deepEqual(commits.map(c=>c.parent),['','C1','C2','C3']);
 assert.deepEqual(steps[2].spaces[0].blobs,[0,1,2,3]);
 assert.deepEqual(steps[3].spaces[0].blobs,[0,1,2,3,4]);
 assert.deepEqual(steps[4].spaces[1],steps[3].spaces[0]);
 assert.deepEqual(steps[5].spaces[2],{commits:4,blobs:[],firstCommit:0});
 assert.deepEqual(steps[6].spaces[2].blobs,[2,3,4]);
 assert.deepEqual(steps[7].spaces[2],steps[6].spaces[2]);
 assert.equal(steps[7].action,'missing');
 assert.deepEqual(steps[8].spaces[2].blobs,[0,2,3,4]);
 assert.deepEqual(steps[9].spaces[3],{commits:1,blobs:[2,3,4],firstCommit:3});
 assert.deepEqual(steps[10].spaces,steps[9].spaces);
 assert.equal(steps[10].action,'unavailable');assert.equal(steps[10].from,-1);
 const localKeys=commits.slice(3,4).flatMap(c=>c.entries);
 assert.ok(!localKeys.includes(0)&&!localKeys.includes(1));
 for(const step of steps){
  assert.equal(step.spaces.length,4);
  for(const space of step.spaces)for(const id of space.blobs)assert.ok(commits.slice(space.firstCommit,space.firstCommit+space.commits).some(c=>c.entries.includes(id)));
 }
});
test('Scalar compiles as a standalone native component and rejects children',async t=>{
 const dir=await mkdtemp(join(tmpdir(),'scalar-mdx-'));t.after(()=>rm(dir,{recursive:true,force:true}));
 for(let i=1;i<=8;i++){const folder=join(dir,String(i).padStart(2,'0'));await mkdir(folder);await writeFile(join(folder,'01.mdx'),'# Intro');}
 const file=join(dir,'07/02.mdx');await writeFile(file,'<Scalar title="Metadata history" />');
 const manifest=await compileStations(dir,'examples/git-meta/layout.json');const component=manifest.slides[6].steps[1].component;
 assert.equal(component.type,'Scalar');assert.equal(component.title,'Metadata history');assert.equal(component.steps.length,11);
 await writeFile(file,'<Scalar>Unexpected</Scalar>');await assert.rejects(compileStations(dir,'examples/git-meta/layout.json'),/Scalar does not accept children/);
 await writeFile(file,'<Scalar title={3} />');await assert.rejects(compileStations(dir,'examples/git-meta/layout.json'),/Scalar title/);
});
