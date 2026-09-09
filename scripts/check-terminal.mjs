// A deterministic recording fixture keeps interaction tests independent of authored casts.
import {mkdtemp,mkdir,writeFile,rm,copyFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';
import {compileStations} from '../src/compiler.mjs';
const dir=await mkdtemp(join(tmpdir(),'park-terminal-test-'));
try {
 for(let i=1;i<=8;i++) {
  const folder=join(dir,String(i).padStart(2,'0'));await mkdir(folder);
  await writeFile(join(folder,'01-intro.mdx'),`# Station ${i}\n\nTerminal playback`);
 }
 await writeFile(join(dir,'01/02-computer.mdx'),'<Computer><prompt>git meta set</prompt><output>OK</output></Computer>');
 await writeFile(join(dir,'01/03-command.mdx'),'<CommandLine cast="demo.cast" />');
 await mkdir(join(dir,'casts'));
 await copyFile('test/fixtures/git-meta.cast',join(dir,'casts/demo.cast'));
 const manifest=join(dir,'deck.json');await writeFile(manifest,JSON.stringify(await compileStations(dir,'examples/git-meta/layout.json',{castDirectory:join(dir,'casts')})));
 const result=spawnSync(process.execPath,['scripts/unreal.mjs','smoke','-TerminalTest',`-SlideManifest=${manifest}`,...process.argv.slice(2)],{stdio:'inherit'});
 if(result.error)throw result.error;process.exitCode=result.status??1;
} finally {await rm(dir,{recursive:true,force:true});}
