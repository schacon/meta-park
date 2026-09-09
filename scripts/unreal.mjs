import {spawnSync} from 'node:child_process';
import {existsSync,mkdirSync,copyFileSync} from 'node:fs';
import {homedir} from 'node:os';
import {resolve,join} from 'node:path';
const root=process.env.UE_ROOT??'/Users/Shared/Epic Games/UE_5.8';
const project=resolve('unreal/SlideEngine.uproject');
const mac=process.platform==='darwin';
const win=process.platform==='win32';
const platform=mac?'Mac':win?'Win64':'Linux';
const editor=join(root,'Engine/Binaries',platform,mac?'UnrealEditor.app/Contents/MacOS/UnrealEditor':win?'UnrealEditor.exe':'UnrealEditor');
function run(exe,args) { if(!existsSync(exe)) throw new Error(`Missing ${exe}. Set UE_ROOT to your Unreal Engine installation.`); const r=spawnSync(exe,args,{stdio:'inherit',...(task==='smoke'?{timeout:180000}:{})}); if(r.error) throw r.error; if(r.status!==0) process.exit(r.status??1); }
const task=process.argv[2];
if(task==='build') run(join(root,'Engine/Build/BatchFiles',mac?'Mac/Build.sh':win?'Build.bat':'Linux/Build.sh'),['SlideEngineEditor',platform,'Development',project,'-WaitMutex']);
else if(task==='present') {
 const app=resolve('dist',platform,mac?'SlideEngine.app/Contents/MacOS/SlideEngine':win?'SlideEngine.exe':'SlideEngine.sh');
 if(!existsSync(app))throw new Error('Build the app first with npm run unreal:package (or use npm run unreal:play).');
 let manifest=resolve('unreal/Content/Slides/deck.json');
 if(mac) {
  // Packaged macOS apps are sandboxed: stage authored data in their own container.
  const info=spawnSync('/usr/libexec/PlistBuddy',['-c','Print CFBundleIdentifier',resolve('dist/Mac/SlideEngine.app/Contents/Info.plist')],{encoding:'utf8'});
  if(info.status!==0)throw new Error('Cannot read the packaged app bundle identifier');
  const directory=join(homedir(),'Library/Containers',info.stdout.trim(),'Data/Library/Application Support/SlideEngine');
  mkdirSync(directory,{recursive:true});const staged=join(directory,'deck.json');copyFileSync(manifest,staged);manifest=staged;
 }
 run(app,['-windowed',`-SlideManifest=${manifest}`,...process.argv.slice(3)]);
}
else if(task==='prepare') run(editor,[project,'-unattended','-nullrhi',`-ExecutePythonScript=${resolve('scripts/prepare.py')}`,'-stdout']);
else if(task==='import-assets') run(editor,[project,'-unattended','-nullrhi',`-ExecutePythonScript=${resolve('scripts/import_park_assets.py')}`,'-stdout']);
else if(task==='smoke') run(editor,[project,'/Game/Maps/Presentation','-game','-windowed','-ResX=1600','-ResY=1000','-unattended','-nosound','-SlideSmokeTest','-stdout',...process.argv.slice(3)]);
else if(task==='play') run(editor,[project,'/Game/Maps/Presentation','-game','-windowed','-ResX=1600','-ResY=1000','-log']);
else if(task==='package') run(join(root,'Engine/Build/BatchFiles',win?'RunUAT.bat':'RunUAT.sh'),['BuildCookRun',`-project=${project}`,'-noP4',`-platform=${platform}`,'-clientconfig=Development','-build','-cook','-stage','-package','-pak','-archive',`-archivedirectory=${resolve('dist')}`]);
else throw new Error('Expected build, prepare, play, present or package');
