import {test} from 'node:test';
import assert from 'node:assert/strict';
import {serializeValues,gitObjectId} from '../src/serializer.mjs';
import {mkdtemp,mkdir,writeFile,rm,readFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {compileStations} from '../src/compiler.mjs';
const row=(target,key,type,value,extra={})=>({target,key,type,value,...extra});
test('Serializer matches reference repository blob IDs and storage paths',()=>{
 const d=serializeValues([row('commit:5aa110f','review:status','string','merged'),row('branch:main','deploy:env','string','production'),row('path:main.rs','tags','set','rust, rust'),row('commit:5aa110f','ci:runs','list','build:passed, test:passed')]);
 assert.equal(d.rows[0].blobs[0].object,'57cfab44328ba19278fced5269d7b09f4f5baf33');
 assert.equal(d.rows[1].blobs[0].path,'branch/b2/main/deploy/env/__value');
 assert.equal(d.rows[1].blobs[0].object,'7ca67535d6075dea3e902c761c7f2b8a25ff690b');
 assert.equal(d.rows[2].blobs[0].path,'path/main.rs/__target__/tags/__set/6ef8575b5fa418c41c0989ed2dcaadbb61226c13');assert.equal(d.rows[2].blobs.length,1);
 assert.match(d.rows[3].blobs[0].path,/__list\/1789042129005-9ea14$/);assert.equal(d.rows[3].blobs[0].object,'27ccdf20db32d653de38afa4b013c474c36c8322');
 assert.equal(d.rows[3].blobs[1].object,'3da17af6cf178f9de954b30d8aefb11fb513922d');
 const duplicate=serializeValues([row('commit:5aa110f','ci:runs','list','build:passed, build:passed')]).rows[0].blobs;
 assert.equal(duplicate.length,2);assert.notEqual(duplicate[0].path,duplicate[1].path);assert.equal(duplicate[0].object,duplicate[1].object);
 assert.equal(d.nodes[0].object,d.tree);assert.equal(serializeValues([...d.rows].reverse()).tree,d.tree);
});
test('Serializer preserves raw strings, builds incremental commits and escapes reserved paths',()=>{
 const d=serializeValues([row('project','title','string','a, b\n世界'),row('path:__private/~x','tags','set','x',{hidden:true}),row('project','local:secret','string','local only')]);
 assert.equal(d.rows[0].blobs[0].value,'a, b\n世界');assert.equal(d.rows[0].blobs[0].object,gitObjectId('blob','a, b\n世界'));
 assert.equal(d.rows[1].hidden,true);assert.match(d.rows[1].path,/path\/~__private\/~~x\/__target__\/tags/);
 assert.equal(d.snapshots.length,2);assert.deepEqual(d.snapshots[0].rows,[0,2]);assert.deepEqual(d.snapshots[1].rows,[0,1,2]);
 assert.ok(!d.snapshots[0].nodes.some(n=>n.rows.includes(1)));
 assert.equal(d.snapshots[1].parent,d.snapshots[0].commit);
 assert.match(d.snapshots[1].commitBody,new RegExp(`parent ${d.snapshots[0].commit}`));
 for(const snap of d.snapshots){assert.equal(gitObjectId('commit',snap.commitBody),snap.commit);assert.equal(snap.nodes[0].name,'refs/meta/local/main^{tree}');}
 assert.notEqual(d.snapshots[0].tree,d.snapshots[1].tree);
 assert.ok(d.nodes.some(n=>n.rows.includes(1)));assert.ok(!d.nodes.some(n=>n.rows.includes(2)));
 for(const values of [[],[row('commit:nope','key','string','x')],[row('project','a::b','set','x')],[row('project','key','invalid','x')],[row('project','key','string','a'),row('project','key','string','b')]])assert.throws(()=>serializeValues(values));
});
test('MDX Serializer preserves the React key attribute and authors all values',async t=>{
 const dir=await mkdtemp(join(tmpdir(),'serializer-mdx-'));t.after(()=>rm(dir,{recursive:true,force:true}));
 for(let i=1;i<=8;i++){await mkdir(join(dir,`0${i}`));await writeFile(join(dir,`0${i}/01.mdx`),'# Intro');}
 await writeFile(join(dir,'05/02.mdx'),await readFile('examples/git-meta/serializer.mdx','utf8'));
 const c=(await compileStations(dir,'examples/git-meta/layout.json')).slides[4].steps[1].component;
 assert.equal(c.snapshots[0].rows.length,7);assert.equal(c.snapshots[1].rows.length,10);
 assert.equal(c.type,'Serializer');assert.equal(c.rows.length,10);assert.equal(c.rows[0].key,'review:status');assert.equal(c.rows[0].value,'approved, later merged');assert.equal(c.rows.filter(r=>r.hidden).length,3);
 const actualDemo=c.rows.map((r,i)=>({...r,value:i===0?'merged':r.value}));
 assert.equal(serializeValues(actualDemo).tree,'065b63844499b736dce00543e529c5b38ece7054');
});
