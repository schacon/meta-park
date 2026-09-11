// Presentation aliases stand for content-addressed objects. The demo treats
// k1/k2 as older than the prune cutoff; command labels omit timestamps.
export function scalarDemo({title='gitviz — 3D Object Store Visualizer'}={}) {
 if(typeof title!=='string'||!title.trim())throw new Error('Scalar title must be nonempty text');
 const commits=[
  {id:'C1',tree:'T1',parent:'',entries:[0,1],delta:'k1 k2'},
  {id:'C2',tree:'T2',parent:'C1',entries:[0,1,2,3],delta:'k3 k4'},
  {id:'C3',tree:'T3',parent:'C2',entries:[2,3],delta:'prune to\n2 values'},
  {id:'C4',tree:'T4',parent:'C3',entries:[2,3,4],delta:'add k5'},
 ];
 const space=(count,blobs,firstCommit=0)=>({commits:count,blobs,firstCommit});
 const empty=()=>space(0,[]),full=()=>space(4,[0,1,2,3,4]);
 const u2=()=>space(4,[0,2,3,4]),shallow=()=>space(1,[2,3,4],3);
 const step=(label,action,room,description,command,detail,spaces,extra={})=>({label,action,room,from:-1,commit:-1,actor:room===0?'user1':room===3?'user3':'user2',description,command,detail,spaces,...extra});
 const steps=[
  step('u1 commit A B','write',0,'User 1 writes C1. T1 lists k1 → A and k2 → B; both blobs enter storage.','git meta set path:demo k1 A\ngit meta set path:demo k2 B\ngit meta serialize','C1  tree T1  |  Δ k1 k2',[space(1,[0,1]),empty(),empty(),empty()],{commit:0}),
  step('u1 commit +C +D','write',0,'C2 retains k1/k2 and adds k3 → C and k4 → D. Only C and D are new blobs.','git meta set path:demo k3 C\ngit meta set path:demo k4 D\ngit meta serialize','C2  parent C1  tree T2  |  Δ k3 k4',[space(2,[0,1,2,3]),empty(),empty(),empty()],{commit:1}),
  step('u1 prune to C D','write',0,'Prune writes C3 and T3 with only C/D. Earlier commits, trees and blobs remain in history.','git meta prune --since 2026-09-01','C3  parent C2  tree T3  |  0 new blobs',[space(3,[0,1,2,3]),empty(),empty(),empty()],{commit:2}),
  step('u1 commit +E','write',0,'C4 adds k5 → E. Its tip tree T4 now contains three values: C, D and E.','git meta set path:demo k5 E\ngit meta serialize','C4  parent C3  tree T4  |  1 new blob: E',[full(),empty(),empty(),empty()],{commit:3}),
  step('u1 push','push',1,'Push copies all four commits, four trees and five blobs to the shared remote.','git meta push','user1 to remote  |  4 commits + 4 trees + 5 blobs',[full(),full(),empty(),empty()],{from:0,actor:'user1'}),
  step('u2 clone','clone',2,'User 2 clones all commit and tree history, without blobs.','git meta setup  # (git clone --filter=blob:none)','4 commits + 4 trees; 0 blobs',[full(),full(),space(4,[]),empty()],{from:1}),
  step('u2 hydrate C D E','hydrate',2,'User 2 hydrates T4: only tip blobs C, D and E move over. A/B remain remote.','git meta setup  # continuing: hydrate tip','hydrate T4  |  transfer 3 blobs: C + D + E',[full(),full(),space(4,[2,3,4]),empty()],{from:1}),
  step('u2 get: missing A','missing',2,'Get finds k1 in local history, but A is missing. Its promisor entry points to the shared remote.','git meta get path:demo k1','k1 -> A  |  missing blob; promisor fetch pending',[full(),full(),space(4,[2,3,4]),empty()]),
  step('u2 fetch A','fetch',2,'The pending get fetches A and returns its value. B stays remote.','git meta get path:demo k1  # continuing the same read','automatic promisor fetch  |  transfer exactly A',[full(),full(),u2(),empty()],{from:1}),
  step('u3 shallow clone','shallow',3,'User 3 clones depth 1: only C4, T4 and blobs C/D/E. Earlier history is unavailable locally.','git meta setup  # .git-meta depth: 1','shallow boundary at C4  |  1 commit + 1 tree + 3 blobs',[full(),full(),u2(),shallow()],{from:1}),
  step('u3 get: no history','unavailable',3,'User 3 cannot find k1: T4 has only k3/k4/k5, and earlier commits are outside the shallow history. No blob is fetched.','git meta get path:demo k1','k1: not found in local history  |  deepen history to discover older keys',[full(),full(),u2(),shallow()]),
 ];
 return {type:'Scalar',title,commits,steps};
}
