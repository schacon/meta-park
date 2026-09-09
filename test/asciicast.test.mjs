import {test} from 'node:test';
import assert from 'node:assert/strict';
import {compileCast,loadCast} from '../src/asciicast.mjs';
const cast=(header,...events)=>[header,...events].map(JSON.stringify).join('\n');
const v2={version:2,width:20,height:4};
function screen(recording,frame=recording.frames.length-1){const rows=[];for(const f of recording.frames.slice(0,frame+1))for(const [y,runs] of f.lines)rows[y]=runs;return rows;}
const text=row=>row.map(r=>r[2]).join('');
test('v2 timing, input exclusion, carriage returns and erase use terminal semantics',async()=>{
 const result=await compileCast(cast(v2,[.2,'o','old'],[.3,'i','SECRET'],[.4,'o','\rnew\x1b[K'],[1,'o','\r\nx\bY']));
 assert.deepEqual(result.frames.map(f=>f.time),[0,.2,.4,1]);
 assert.equal(text(screen(result)[0]),'new');assert.equal(text(screen(result)[1]),'Y');
 assert.equal(JSON.stringify(result).includes('SECRET'),false);
});
test('v3 relative intervals, comments, resize and idle limit',async()=>{
 const recording=cast({version:3,term:{cols:20,rows:4},idle_time_limit:2},[.5,'o','A'],[4,'m','marker'],[.5,'r','30x5'],[.5,'o','B']);
 const result=await compileCast(recording+'\n# comment',{speed:2});
 assert.deepEqual(result.frames.map(f=>f.time),[0,.25,1.5,1.75]);
 assert.equal(result.frames.at(-1).cols,30);assert.equal(result.frames.at(-1).rows,5);assert.equal(result.duration,1.75);
});
test('ANSI palette, 256-color, truecolor, inverse, bold, underline and theme',async()=>{
 const result=await compileCast(cast({...v2,theme:{fg:'#abcdef',bg:'#123456',palette:'#000000:#110000'}},[0,'o','\x1b[31mR\x1b[38;5;196mP\x1b[38;2;1;2;3mT\x1b[0;7mI\x1b[0;1;4mB']));
 const runs=screen(result)[0];
 assert.equal(runs[0][3],'#110000');assert.equal(runs[1][3],'#ff0000');assert.equal(runs[2][3],'#010203');
 assert.deepEqual(runs[3].slice(3,5),['#123456','#abcdef']);assert.equal(runs[4][5],5);
});
test('cursor addressing, clearing, alternate screen and split escape sequences',async()=>{
 const result=await compileCast(cast(v2,[0,'o','main'],[.1,'o','\x1b[?1049h\x1b[2;3Halt\x1b[?25l'],[.2,'o','\x1b[?1049l\x1b[2'],[.3,'o','J\x1b[Hdone\x1b[?25h']));
 assert.equal(text(screen(result,2)[1]),'  alt');assert.equal(result.frames[2].cursor[2],false);
 assert.equal(text(screen(result)[0]),'done');assert.equal(result.frames.at(-1).cursor[2],true);
});
test('malformed casts report useful errors',async()=>{
 await assert.rejects(compileCast('not JSON'),/header/);
 await assert.rejects(compileCast(cast({version:1})),/versions/);
 await assert.rejects(compileCast(cast({...v2,width:0})),/size/);
 await assert.rejects(compileCast(cast(v2,[2,'o','A'],[1,'o','B'])),/backward/);
 await assert.rejects(compileCast(cast(v2,[0,'r','oops'])),/resize/);
 await assert.rejects(compileCast(cast(v2,[0,'o',3])),/text/);
});
test('local project casts accept filenames or casts paths, and reject URLs',async t=>{
 const {mkdtemp,writeFile,rm}=await import('node:fs/promises');
 const {tmpdir}=await import('node:os');const {join}=await import('node:path');
 const castDirectory=await mkdtemp(join(tmpdir(),'cast-input-'));t.after(()=>rm(castDirectory,{recursive:true,force:true}));
 await writeFile(join(castDirectory,'demo.cast'),cast(v2,[0,'o','local']));
 for(const src of ['demo.cast','casts/demo.cast','./casts/demo.cast'])assert.equal(text(screen(await loadCast(src,{castDirectory}))[0]),'local');
 await writeFile(join(castDirectory,'demo.cast'),cast(v2,[0,'o','replacement']));
 assert.equal(text(screen(await loadCast('casts/demo.cast',{castDirectory}))[0]),'replacement');
 await assert.rejects(loadCast('missing.cast',{castDirectory}),/ENOENT/);
 for(const src of ['https://asciinema.org/a/123.cast','../demo.cast','/tmp/demo.cast'])await assert.rejects(loadCast(src,{castDirectory}),/local/);
});
