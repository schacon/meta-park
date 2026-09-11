// Timings transcribed from the supplied setup benchmark. Setup completion
// excludes background history indexing; values inherit the source's rounding.
export function speedGraph({metric='setup'}={}) {
 if(!['setup','total'].includes(metric))throw new Error('SpeedGraph metric must be setup or total');
 const rows=[
  {label:'100k',total:12.8,index:.8,parts:[2.8,4.6,.8,3.8,.8]},
  {label:'1M',total:79.5,index:15.7,parts:[38.5,13.9,7.6,3.7,15.7]},
  {label:'5M',total:553,index:109,parts:[306,79,51,7,109]},
  {label:'5M shallow',total:93,index:2.5,parts:[70,17,0,4.3,2.5]},
 ].map(({label,total,index,parts})=>{const seconds=metric==='setup'?Number((total-index).toFixed(1)):total;return {label,seconds,phases:parts.map((duration,i)=>({name:['fetch','hydrate','repack','materialize','index'][i],seconds:duration,inNeck:metric==='total'||i<4})),time:metric==='setup'?`${Math.round(seconds)} s`:`${seconds} s`};});
 return {type:'SpeedGraph',metric,title:'How long does setup take?',subtitle:metric==='setup'?'Neck height = setup time. Indexing continues in the background.':'Dashed line: setup returns. Indexing continues above it.',rows};
}
