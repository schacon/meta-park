// Exercise multiple plain and interactive pages without editing the user's slides.
import {mkdtemp,mkdir,writeFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';
import {compileStations} from '../src/compiler.mjs';
const dir=await mkdtemp(join(tmpdir(),'park-station-test-'));
try {
 for(let i=1;i<=8;i++) {
  const folder=join(dir,String(i).padStart(2,'0'));await mkdir(folder);
  await writeFile(join(folder,'01-intro.mdx'),`# Station ${i}\n\n- First page\n- Arrow keys advance within the station`);
 }
 await writeFile(join(dir,'01/02-details.mdx'),'# Details\n\nThe physical sign and camera stay in place.\n\n- Swipe content within the frame\n- Step backward to revisit');
 await writeFile(join(dir,'01/03-computer.mdx'),'<Computer><prompt>git meta get owner</prompt><output>s.chacon</output></Computer>');
 await writeFile(join(dir,'01/04-summary.mdx'),'# Back to the sign\n\nThe computer retracts before leaving this station.');
 const manifest=join(dir,'deck.json');
 await writeFile(manifest,JSON.stringify(await compileStations(dir,'examples/git-meta/layout.json')));
 const result=spawnSync(process.execPath,['scripts/unreal.mjs','smoke','-StationContentTest',`-SlideManifest=${manifest}`, ...process.argv.slice(2)],{stdio:'inherit'});
 if(result.error)throw result.error;
 process.exitCode=result.status??1;
} finally {await rm(dir,{recursive:true,force:true});}
