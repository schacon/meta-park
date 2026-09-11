import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,copyFile,readFile,writeFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {compileStations} from '../src/compiler.mjs';
import {loadSlideImage} from '../src/slide-image.mjs';
const chart=resolve('slides/07/images/setup-performance.png');
test('image page follows Scalar on the board and embeds the original PNG',async t=>{
 const dir=await mkdtemp(join(tmpdir(),'slide-image-'));t.after(()=>rm(dir,{recursive:true,force:true}));
 for(let i=1;i<=8;i++){const folder=join(dir,String(i).padStart(2,'0'));await mkdir(folder);await writeFile(join(folder,'01-intro.mdx'),'## Intro');}
 await writeFile(join(dir,'07/02-demo.mdx'),'<Scalar />');
 await copyFile(chart,join(dir,'07/chart.png'));
 await writeFile(join(dir,'07/03-chart.mdx'),'<Image src="./chart.png" alt="Setup timings" />');
 const steps=(await compileStations(dir,'examples/git-meta/layout.json')).slides[6].steps;
 assert.equal(steps[1].component.type,'Scalar');assert.equal(steps[2].kind,'slide');assert.equal(steps[2].component,null);
 assert.deepEqual(steps[2].blocks,[]);assert.equal(steps[2].title,'Setup timings');
 assert.equal(steps[2].image.width,1942);assert.equal(steps[2].image.height,1126);
 assert.deepEqual(Buffer.from(steps[2].image.data,'base64'),await readFile(chart));
 await writeFile(join(dir,'07/03-chart.mdx'),'<Image src="./chart.png" alt="Chart">Unexpected</Image>');
 await assert.rejects(compileStations(dir,'examples/git-meta/layout.json'),/Image does not accept children/);
});
test('image loading rejects missing descriptions, remote paths and non-PNG files',async()=>{
 const source=resolve('slides/07/03-setup-performance.mdx');
 await assert.rejects(loadSlideImage({src:'./images/setup-performance.png'},source),/alt text/);
 await assert.rejects(loadSlideImage({src:'https://example.com/chart.png',alt:'Chart'},source),/local PNG/);
 await assert.rejects(loadSlideImage({src:'01-scaling.mdx',alt:'Not an image'},source),/PNG file/);
});
