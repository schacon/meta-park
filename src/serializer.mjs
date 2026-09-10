import {createHash} from 'node:crypto';
import {readFile} from 'node:fs/promises';
const reference=JSON.parse(await readFile(new URL('../examples/git-meta/serializer-reference.json',import.meta.url),'utf8'));
const sha1=value=>createHash('sha1').update(value).digest('hex');
export const gitObjectId=(type,data)=>{const bytes=Buffer.from(data);return sha1(Buffer.concat([Buffer.from(`${type} ${bytes.length}\0`),bytes]));};
export function serializeValues(values,{title='Git Object Explorer',author='schacon@gmail.com'}={}) {
 if(typeof title!=='string'||!title.trim())throw new Error('Serializer title must be nonempty text');
 if(!values.length)throw new Error('Serializer needs at least one Value');
 const seen=new Set();
 const rows=values.map((v,index)=>{
  for(const field of ['target','key','type','value'])if(typeof v[field]!=='string'||(field!=='value'&&!v[field].trim()))throw new Error(`Serializer Value needs ${field}`);
  if(!['string','set','list'].includes(v.type))throw new Error('Serializer Value type must be string, set or list');
  if(!/^[^\s/:]+(?::[^\s/:]+)*$/.test(v.key)||v.key.split(':').some(k=>k.startsWith('__')))throw new Error('Serializer key must contain nonempty colon-separated segments without reserved names');
  if(v.hidden!==undefined&&typeof v.hidden!=='boolean')throw new Error('Serializer hidden must be a boolean');
  let target=v.target;
  if(target.startsWith('commit:')) {
   const id=target.slice(7),matches=reference.commits.filter(c=>c.oid.startsWith(id));
   if(matches.length===1)target=`commit:${matches[0].oid}`;
   else if(!/^[0-9a-f]{40}$/.test(id))throw new Error('Serializer commit needs a full object ID or a known demo commit prefix');
  }
  const [kind,...tail]=target.split(':'),name=tail.join(':');
  if(!['commit','branch','path','project'].includes(kind)||kind!=='project'&&!name||kind==='project'&&name)throw new Error('Serializer supports commit, branch, path and project targets');
  if(name.split('/').some(s=>s==='..'||s==='.'||!s)&&kind!=='project')throw new Error('Serializer target contains an invalid path segment');
  const id=target+'\0'+v.key;if(seen.has(id))throw new Error('Serializer target/key pairs must be unique');seen.add(id);
  const source=reference.rows.find(r=>r.target===target&&r.key===v.key);
  const history=source?.history??[];
  const stamp=history.at(-1)?.timestamp??1789042129000+index;
  const base=kind==='project'?'project':kind==='commit'?`commit/${name.slice(0,2)}/${name}`:kind==='path'?`path/${name.split('/').map(s=>s.startsWith('~')||s.startsWith('__')?'~'+s:s).join('/')}/__target__`:`${kind}/${sha1(name).slice(0,2)}/${name}`;
  const path=`${base}/${v.key.split(':').join('/')}`;
  const entries=v.type==='string'?[v.value]:v.value.split(',').map(s=>s.trim()).filter(Boolean);
  const unique=v.type==='set'?[...new Set(entries)]:entries;
  const usedEntries=new Set();let lastTimestamp=-Infinity;
  const blobs=unique.map((value,i)=>{
   const existing=source?.entries.find(e=>e.value===value&&!usedEntries.has(e.timestamp));
   if(existing)usedEntries.add(existing.timestamp);
   const timestamp=Math.max(existing?.timestamp??stamp+i,lastTimestamp+1);lastTimestamp=timestamp;
   const object=gitObjectId('blob',value);
   const suffix=v.type==='string'?'__value':v.type==='set'?`__set/${object}`:`__list/${timestamp}-${sha1(value).slice(0,5)}`;
   return {path:`${path}/${suffix}`,object,value,bytes:Buffer.byteLength(value)};
  });
  return {target,shortTarget:kind==='commit'?`commit:${name.slice(0,7)}`:target,key:v.key,type:v.type,value:v.value,hidden:!!v.hidden,path,author:history.at(-1)?.email??author,created:history[0]?.timestamp??stamp,modified:stamp,history,blobs,excluded:v.key.startsWith('local:')};
 });
 function buildTree(rowIndices) {
 const root={name:'refs/meta/local/main^{tree}',path:'',type:'tree',children:new Map(),rows:[]};
 function insert(path,value,row) {
  let node=root;const parts=path.split('/');
  for(const [i,name] of parts.entries()) {
   if(row>=0&&!node.rows.includes(row))node.rows.push(row);
   if(!node.children.has(name))node.children.set(name,{name,path:parts.slice(0,i+1).join('/'),type:i===parts.length-1?'blob':'tree',children:new Map(),rows:[]});
   node=node.children.get(name);
  }
  if(row>=0)node.rows.push(row);node.value=value;node.object=gitObjectId('blob',value);node.bytes=Buffer.byteLength(value);
 }
 rowIndices.forEach(i=>{const r=rows[i];if(!r.excluded)r.blobs.forEach(b=>insert(b.path,b.value,i));});
 insert('README.md',reference.readme,-1);
 function hash(node) {
  if(node.type==='blob')return;
  const entries=[...node.children.values()].sort((a,b)=>Buffer.compare(Buffer.from(a.name+(a.type==='tree'?'/':'')),Buffer.from(b.name+(b.type==='tree'?'/':''))));
  for(const n of entries)hash(n);
  node.object=gitObjectId('tree',Buffer.concat(entries.map(n=>Buffer.concat([Buffer.from(`${n.type==='tree'?'40000':'100644'} ${n.name}\0`),Buffer.from(n.object,'hex')]))));
 }
 hash(root);
 const nodes=[];
 function flatten(n,parent=-1,depth=0){const index=nodes.length;nodes.push({name:n.name,path:n.path,type:n.type,object:n.object,value:n.value??'',bytes:n.bytes??0,rows:n.rows,parent,depth});for(const child of [...n.children.values()].sort((a,b)=>(a.type==='tree'?0:1)-(b.type==='tree'?0:1)||a.name.localeCompare(b.name)))flatten(child,index,depth+1);}
 flatten(root);
 return {tree:root.object,nodes,rows:rowIndices};
 }
 const initial=rows.flatMap((r,i)=>r.hidden?[]:[i]),all=rows.map((_,i)=>i);
 const snapshots=[buildTree(initial)];
 if(initial.length!==all.length)snapshots.push(buildTree(all));
 snapshots.forEach((snapshot,i)=>{
  snapshot.message=i===0?'Initial metadata':'Add metadata';
  snapshot.parent=i?snapshots[i-1].commit:null;
  const identity=`Git-meta Demo <${author}> ${1789042129+i*60} +0000`;
  const body=`tree ${snapshot.tree}\n${snapshot.parent?`parent ${snapshot.parent}\n`:''}author ${identity}\ncommitter ${identity}\n\n${snapshot.message}\n`;
  snapshot.commit=gitObjectId('commit',body);snapshot.commitBody=body;
 });
 const full=snapshots.at(-1);
 return {type:'Serializer',title,ref:reference.ref,tree:full.tree,rows,nodes:full.nodes,snapshots};
}
