// Exercise FSV without depending on changes to the authored presentation.
import {mkdtemp,mkdir,writeFile,copyFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';
import {compileStations} from '../src/compiler.mjs';
const dir=await mkdtemp(join(tmpdir(),'park-fsv-test-'));
try {
 for(let i=1;i<=8;i++) {
  const folder=join(dir,String(i).padStart(2,'0'));await mkdir(folder);
  await writeFile(join(folder,'01-intro.mdx'),i===2?'# New Metadata Use Cases':`# Station ${i}\n\nFilesystem viewer`);
 }
 await copyFile('examples/git-meta/fsv.mdx',join(dir,'02/02-fsv.mdx'));
 const manifest=join(dir,'deck.json');await writeFile(manifest,JSON.stringify(await compileStations(dir,'examples/git-meta/layout.json')));
 const result=spawnSync(process.execPath,['scripts/unreal.mjs','smoke','-FSVTest',`-SlideManifest=${manifest}`,...process.argv.slice(2)],{stdio:'inherit'});
 if(result.error)throw result.error;process.exitCode=result.status??1;
} finally {await rm(dir,{recursive:true,force:true});}
