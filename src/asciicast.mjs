import xterm from '@xterm/headless';
import {readFile} from 'node:fs/promises';
import {resolve,basename} from 'node:path';
const {Terminal}=xterm;
const defaults=['#000000','#cd3131','#0dbc79','#e5e510','#2472c8','#bc3fbc','#11a8cd','#e5e5e5','#666666','#f14c4c','#23d18b','#f5f543','#3b8eea','#d670d6','#29b8db','#ffffff'];
const rgb=n=>'#'+n.toString(16).padStart(6,'0');
function size(cols,rows){if(!Number.isInteger(cols)||!Number.isInteger(rows)||cols<1||rows<1||cols>500||rows>200)throw new Error('Cast terminal size must be 1–500 columns by 1–200 rows');}
export async function loadCast(src,{castDirectory=resolve('casts'),...options}={}) {
 if(typeof src!=='string'||!src.trim())throw new Error('CommandLine needs a cast filename in ./casts/');
 const name=src.replace(/^\.\//,'').replace(/^casts\//,'');
 if(name!==basename(name)||!name.endsWith('.cast'))throw new Error('CommandLine recordings must be local ./casts/*.cast files');
 return compileCast(await readFile(resolve(castDirectory,name),'utf8'),options);
}
// Interpret terminal control sequences at build time with xterm, then bundle timed
// row patches. Native playback needs neither a browser nor an on-stage connection.
export async function compileCast(text,{speed=1,idleTimeLimit}={}) {
 if(!Number.isFinite(speed)||speed<=0)throw new Error('Cast speed must be positive');
 const lines=text.replace(/^\uFEFF/,'').split(/\r?\n/);
 let header;try{header=JSON.parse(lines.shift());}catch{throw new Error('Invalid asciicast header');}
 if(![2,3].includes(header.version))throw new Error('Supported asciicast versions are 2 and 3');
 let cols=header.version===3?header.term?.cols:header.width,rows=header.version===3?header.term?.rows:header.height;
 size(cols,rows);
 const theme=(header.version===3?header.term?.theme:header.theme)??{};
 const palette=[...defaults];
 if(theme.palette)theme.palette.split(':').forEach((c,i)=>{if(i<16&&/^#[0-9a-f]{6}$/i.test(c))palette[i]=c;});
 for(let i=16;i<232;i++){const n=i-16;const c=v=>v?55+v*40:0;palette[i]=rgb((c(Math.floor(n/36))<<16)|(c(Math.floor(n/6)%6)<<8)|c(n%6));}
 for(let i=232;i<256;i++){const v=8+(i-232)*10;palette[i]=rgb((v<<16)|(v<<8)|v);}
 const fg=/^#[0-9a-f]{6}$/i.test(theme.fg)?theme.fg:'#e0e8e0',bg=/^#[0-9a-f]{6}$/i.test(theme.bg)?theme.bg:'#080808';
 const limit=idleTimeLimit??header.idle_time_limit??Infinity;
 if(!(limit>0))throw new Error('Cast idleTimeLimit must be positive');
 const term=new Terminal({cols,rows,allowProposedApi:true,scrollback:0});
 let cursorVisible=true;
 term.parser.registerCsiHandler({prefix:'?',final:'l'},params=>{if(params.includes(25))cursorVisible=false;return false;});
 term.parser.registerCsiHandler({prefix:'?',final:'h'},params=>{if(params.includes(25))cursorVisible=true;return false;});
 term.parser.registerEscHandler({final:'c'},()=>{cursorVisible=true;return false;});
 let previous=[],frames=[],rawTime=0,time=0,last=0;
 const color=(cell,foreground)=>{
  if(foreground?cell.isFgRGB():cell.isBgRGB())return rgb(foreground?cell.getFgColor():cell.getBgColor());
  if(foreground?cell.isFgPalette():cell.isBgPalette()){
   let n=foreground?cell.getFgColor():cell.getBgColor();if(foreground&&cell.isBold()&&n<8)n+=8;return palette[n];
  }
  return foreground?fg:bg;
 };
 function frame(reset=false) {
  const changes=[];
  for(let y=0;y<rows;y++) {
   const line=term.buffer.active.getLine(term.buffer.active.viewportY+y),runs=[];
   for(let x=0;x<cols;x++) {
    const cell=line?.getCell(x);if(!cell||cell.getWidth()===0)continue;
    let foreground=color(cell,true),background=color(cell,false);
    if(cell.isInverse())[foreground,background]=[background,foreground];
    const chars=cell.isInvisible()?' ':cell.getChars()||' ';
    const flags=(cell.isBold()?1:0)|(cell.isDim()?2:0)|(cell.isUnderline()?4:0)|(cell.isStrikethrough()?8:0)|(cell.isBlink()?16:0);
    const width=cell.getWidth();
    const tail=runs.at(-1);
    if(tail&&tail[0]+tail[1]===x&&tail[3]===foreground&&tail[4]===background&&tail[5]===flags&&width===1) {tail[1]++;tail[2]+=chars;}
    else runs.push([x,width,chars,foreground,background,flags]);
   }
   while(runs.length) {
    const tail=runs.at(-1);if(tail[4]!==bg||tail[5])break;
    const trimmed=tail[2].replace(/ +$/,'');tail[1]-=tail[2].length-trimmed.length;tail[2]=trimmed;
    if(tail[1]>0)break;runs.pop();
   }
   const key=JSON.stringify(runs);
   if(reset||key!==previous[y]){changes.push([y,runs]);previous[y]=key;}
  }
  frames.push({time,cols,rows,lines:changes,cursor:[Math.min(cols-1,term.buffer.active.cursorX),term.buffer.active.cursorY,cursorVisible]});
 }
 try {
  frame(true);
  for(let i=0;i<lines.length;i++) {
   if(!lines[i].trim()||(header.version===3&&lines[i].startsWith('#')))continue;
   let event;try{event=JSON.parse(lines[i]);}catch{throw new Error(`Invalid cast event on line ${i+2}`);}
   if(!Array.isArray(event)||event.length!==3||!Number.isFinite(event[0])||event[0]<0||typeof event[1]!=='string')throw new Error(`Invalid cast event on line ${i+2}`);
   const [stamp,type,data]=event;
   rawTime=header.version===3?rawTime+stamp:stamp;
   if(rawTime<last)throw new Error(`Cast timestamps go backward on line ${i+2}`);
   time+=Math.min(rawTime-last,limit)/speed;last=rawTime;
   if(type==='o') {
    if(typeof data!=='string')throw new Error(`Cast output must be text on line ${i+2}`);
    await new Promise(done=>term.write(data,done));frame();
   } else if(type==='r') {
    const match=typeof data==='string'&&data.match(/^(\d+)x(\d+)$/);if(!match)throw new Error(`Invalid cast resize on line ${i+2}`);
    cols=Number(match[1]);rows=Number(match[2]);size(cols,rows);term.resize(cols,rows);previous=[];frame(true);
   }
   // Input, markers, exit and future event types carry timing, but aren't output.
  }
  if(header.version===2&&Number.isFinite(header.duration)&&header.duration>last)time+=Math.min(header.duration-last,limit)/speed;
  return {version:1,title:header.title??'Terminal recording',duration:time,cols:frames[0].cols,rows:frames[0].rows,fg,bg,frames};
 } finally {term.dispose();}
}
