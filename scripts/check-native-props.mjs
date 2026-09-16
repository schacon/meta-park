import {mkdtemp,mkdir,writeFile,copyFile,rm} from 'node:fs/promises';
import {tmpdir,homedir} from 'node:os';
import {join,resolve} from 'node:path';
import {spawnSync} from 'node:child_process';
import {compileStations} from '../src/compiler.mjs';
const dir=await mkdtemp(join(tmpdir(),'park-props-test-'));
try {
 for(let i=1;i<=8;i++) {
  const folder=join(dir,String(i).padStart(2,'0'));await mkdir(folder);
  await writeFile(join(folder,'01-intro.mdx'),i===1?'# Git Meta\n\nNew metadata approach for Git':i===2?'# New Metadata Use Cases':i===3?'<RaptorWarning>Problems with existing solutions</RaptorWarning>':`# Station ${i}`);
 }
 await copyFile('slides/01/02-what-is.mdx',join(dir,'01/02-what-is.mdx'));
 await copyFile('examples/git-meta/cold-storage.mdx',join(dir,'01/03-cold-storage.mdx'));
 await copyFile('examples/git-meta/raptors.mdx',join(dir,'03/02-raptors.mdx'));
 const manifest=join(dir,'deck.json');await writeFile(manifest,JSON.stringify(await compileStations(dir,'examples/git-meta/layout.json')));
 const flags=process.argv.slice(2).filter(a=>a!=='--packaged');
 let result;
 if(process.argv.includes('--packaged')) {
  if(process.platform!=='darwin')throw new Error('--packaged currently tests the macOS application');
  const bundle=resolve('dist/Mac/SlideEngine.app');
  const id=spawnSync('/usr/libexec/PlistBuddy',['-c','Print CFBundleIdentifier',join(bundle,'Contents/Info.plist')],{encoding:'utf8'});
  if(id.status!==0)throw new Error('Build the application before testing its package');
  const container=join(homedir(),'Library/Containers',id.stdout.trim(),'Data/Library/Application Support/SlideEngine');await mkdir(container,{recursive:true});
  const staged=join(container,'native-props-test.json');await copyFile(manifest,staged);
  try {result=spawnSync(join(bundle,'Contents/MacOS/SlideEngine'),['-windowed','-SlideSmokeTest','-NativePropsTest',`-SlideManifest=${staged}`,'-stdout',...flags],{stdio:'inherit',timeout:180000});}
  finally {await rm(staged,{force:true});}
 } else result=spawnSync(process.execPath,['scripts/unreal.mjs','smoke','-NativePropsTest',`-SlideManifest=${manifest}`,...flags],{stdio:'inherit'});
 if(result.error)throw result.error;process.exitCode=result.status??1;
} finally {await rm(dir,{recursive:true,force:true});}
