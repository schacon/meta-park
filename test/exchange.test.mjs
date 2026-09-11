import test from 'node:test';
import assert from 'node:assert/strict';
import {mergeExchangeValue,exchangeDemo,isFastForward} from '../src/exchange.mjs';
import {compileStations} from '../src/compiler.mjs';
import {mkdtemp,mkdir,writeFile,rm} from 'node:fs/promises';
import {join} from 'node:path';
import {tmpdir} from 'node:os';
test('shared-ancestor strings prefer local only when both sides changed',()=>{
 assert.equal(mergeExchangeValue('string','draft','approved','needs-work'),'approved');
 assert.equal(mergeExchangeValue('string','draft','draft','needs-work'),'needs-work');
 assert.equal(mergeExchangeValue('string','draft','approved','draft'),'approved');
});
test('sets merge independent additions and deduplicate concurrent additions',()=>{
 assert.deepEqual(mergeExchangeValue('set',['review'],['review','ready','shared'],['review','urgent','shared']),['ready','review','shared','urgent']);
});
test('lists merge by identity, sort by timestamp/hash ID, and retain duplicate text',()=>{
 const base=[{id:'1000-a',value:'opened'}];
 const local=[...base,{id:'3000-b',value:'checked'}],remote=[...base,{id:'3000-a',value:'checked'}];
 assert.deepEqual(mergeExchangeValue('list',base,local,remote),[base[0],remote[1],local[1]]);
 assert.deepEqual(local,[...base,{id:'3000-b',value:'checked'}]);
});
test('FF-only rejects divergence, accepts a reconciled descendant, rejects a race',()=>{
 const {history}=exchangeDemo();
 assert.equal(isFastForward(history,'R','L'),false);
 assert.equal(isFastForward(history,'R','M'),true);
 assert.equal(isFastForward(history,'B','R'),true);
 assert.equal(isFastForward(history,'R','R'),true);
 const raced={...history,R2:{parent:'R'}};
 assert.equal(isFastForward(raced,'R2','M'),false);
 assert.equal(isFastForward({...raced,M2:{parent:'R2'}},'R2','M2'),true);
});
test('Exchange compiles at station 06 with replayable stages and rejects invalid authoring',async t=>{
 const dir=await mkdtemp(join(tmpdir(),'exchange-mdx-'));t.after(()=>rm(dir,{recursive:true,force:true}));
 for(let i=1;i<=8;i++){const folder=join(dir,String(i).padStart(2,'0'));await mkdir(folder);await writeFile(join(folder,'01.mdx'),'# Intro');}
 const file=join(dir,'06/02.mdx');await writeFile(file,'<Exchange />');
 const manifest=await compileStations(dir,'examples/git-meta/layout.json');const c=manifest.slides[5].steps[1].component;
 assert.equal(c.type,'Exchange');assert.equal(c.steps.length,7);assert.equal(c.objects.length,3);
 assert.equal(c.steps[0].label,'Keeper 1 writes');
 assert.equal(c.steps[1].label,'Keeper 2 copies');
 assert.deepEqual(c.objects.map(o=>o.base.map(v=>v.text)),[['Rex'],['apples','ferns'],['08:00']]);
 assert.deepEqual(c.objects[0].result.map(r=>r.text),['Chomper']);
 assert.deepEqual(c.objects[1].result.map(r=>r.text),['apples','ferns','leaves','moss']);
 assert.deepEqual(c.objects[2].result.map(r=>r.text),['08:00','12:00','16:00']);
 for(const step of c.steps)assert.ok(!step.command.includes('--ff-only'));
 await writeFile(file,'<Exchange>Unexpected</Exchange>');await assert.rejects(compileStations(dir,'examples/git-meta/layout.json'),/Exchange does not accept children/);
 await writeFile(file,'<Exchange title={3} />');await assert.rejects(compileStations(dir,'examples/git-meta/layout.json'),/Exchange title/);
});

test('keepers publish full append-only snapshots and merge locally before retrying',()=>{
 const d=exchangeDemo();
 assert.deepEqual(d.steps.map(s=>s.action),['create','copy','edit','push_remote','blocked','merge','publish']);
 assert.deepEqual(d.steps.map(s=>s.log),[[0],[0],[0],[0,1],[0,1],[0,1],[0,1,2]]);
 assert.deepEqual(d.steps[2].states,['local','base','remote']);
 assert.deepEqual(d.steps[4].states,['local','remote','remote']);
 assert.deepEqual(d.steps[5].states,['merged','remote','remote']);
 assert.deepEqual(d.steps[6].states,['merged','merged','remote']);
 assert.deepEqual(d.records.map(r=>r.parent),[null,'B','R']);
 for(const r of d.records)assert.deepEqual(r.entries.map(e=>e.key),['dino:name','feeding:foods','feeding:times']);
 assert.deepEqual(d.records[0].entries.map(e=>e.values.length),[1,2,1]);
 assert.deepEqual(d.records[1].entries.map(e=>e.values.length),[1,3,2]);
 assert.deepEqual(d.records[2].entries.map(e=>e.values.length),[1,4,3]);
 assert.equal(d.records[0].entries[0].values[0].text,'Rex');
 assert.equal(d.records[1].entries[0].values[0].text,'Tiny');
 assert.equal(d.records[2].entries[0].values[0].text,'Chomper');
});
