// A bounded teaching model: a common ancestor, concurrent string writes,
// set additions and list appends. Deletions/tombstones are outside this demo.
export function mergeExchangeValue(type, base, local, remote) {
 const equal=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
 if(equal(local,base))return structuredClone(remote);
 if(equal(remote,base)||equal(local,remote))return structuredClone(local);
 if(type==='string')return local;
 if(type==='set')return [...new Set([...local,...remote])].sort();
 if(type==='list')return [...new Map([...remote,...local].map(e=>[e.id,e])).values()].sort((a,b)=>a.id.localeCompare(b.id));
 throw new Error(`Unsupported exchange type: ${type}`);
}
export function isFastForward(commits, current, proposed) {
 const seen=new Set();
 while(proposed&&!seen.has(proposed)) {
  if(proposed===current)return true;
  seen.add(proposed);proposed=commits[proposed]?.parent;
 }
 return false;
}
export function exchangeDemo({title='Two keepers, one dinosaur'}={}) {
 if(typeof title!=='string'||!title.trim())throw new Error('Exchange title must be nonempty text');
 const definitions=[
  {type:'string',key:'dino:name',base:'Rex',local:'Chomper',remote:'Tiny'},
  {type:'set',key:'feeding:foods',base:['apples','ferns'],local:['apples','ferns','leaves'],remote:['apples','ferns','moss']},
  {type:'list',key:'feeding:times',base:[{id:'1000-a',value:'08:00'}],local:[{id:'1000-a',value:'08:00'},{id:'3000-c',value:'16:00'}],remote:[{id:'1000-a',value:'08:00'},{id:'2000-b',value:'12:00'}]},
 ];
 const objects=definitions.map(s=>{
  const rows=value=>s.type==='string'?[{text:value,origin:value===s.base?'base':value===s.local?'local':'remote'}]:value.map(v=>{
   const id=s.type==='list'?v.id:v;
   const contains=items=>items.some(e=>(s.type==='list'?e.id:e)===id);
   return {text:s.type==='list'?v.value:v,id,origin:contains(s.base)?'base':contains(s.local)?'local':'remote'};
  });
  return {type:s.type,key:s.key,base:rows(s.base),local:rows(s.local),remote:rows(s.remote),result:rows(mergeExchangeValue(s.type,s.base,s.local,s.remote))};
 });
 // Columns are always Keeper 1, shared record, Keeper 2. A local merge never
 // mutates the shared record; Keeper 2's copy stays at R after M is published.
 const steps=[
  {action:'create',label:'Keeper 1 writes',states:['base','base','empty'],record:'B',headline:'Keeper 1 starts the care record.',note:'One name, two foods, one feeding time.',command:'keeper1$ git meta push'},
  {action:'copy',label:'Keeper 2 copies',states:['base','base','base'],record:'B',headline:'Keeper 2 copies the same starting record.',note:'Both keepers now work from edition B.',command:'keeper2$ git meta setup'},
  {action:'edit',label:'Both keepers edit',states:['local','base','remote'],record:'B',headline:'Both keepers edit their own copies at the same time.',note:'Each changes the name, adds one food, and appends one feeding time.',command:'# Independent local edits; the shared record stays at B'},
  {action:'push_remote',label:'Keeper 2 publishes',states:['local','remote','remote'],record:'R',headline:'Keeper 2 publishes first. The record advances to R.',note:'Tiny, moss, and 12:00 reach the shared record. Keeper 1 still has their own edits.',command:'keeper2$ git meta push'},
  {action:'blocked',label:'Keeper 1 blocked',states:['local','remote','remote'],record:'R',headline:'Keeper 1 tries to push an older branch. Blocked!',note:'FF-only protects edition R: this push would drop Keeper 2’s work.',command:'keeper1$ git meta push'},
  {action:'merge',label:'Merge at Keeper 1',states:['merged','remote','remote'],record:'R',headline:'Keeper 1 merges the latest record into their own notes.',note:'Local name wins. Foods combine once each. Both feeding-time appends survive.',command:'# The push retry fetches R and merges locally; the shared record stays at R'},
  {action:'publish',label:'Publish the merge',states:['merged','merged','remote'],record:'M',headline:'Keeper 1 publishes the merged record. Accepted!',note:'M follows R, so no published work is dropped. Keeper 2’s copy stays at R.',command:'keeper1$ git meta push  # retry succeeds'},
 ].map((s,phase)=>({...s,phase,log:phase<3?[0]:phase<6?[0,1]:[0,1,2]}));
 const history={B:{parent:null},L:{parent:'B'},R:{parent:'B'},M:{parent:'R'}};
 const records=[{id:'B',parent:null,author:'Keeper 1',state:'base'},{id:'R',parent:'B',author:'Keeper 2',state:'remote'},{id:'M',parent:'R',author:'Keeper 1',state:'merged'}].map(r=>({...r,entries:objects.map(o=>({key:o.key,type:o.type,values:o[r.state==='merged'?'result':r.state]}))}));
 return {type:'Exchange',title,objects,steps,history,records};
}
