import {readFile} from 'node:fs/promises';
import {dirname,resolve} from 'node:path';

// Embed the original PNG so a staged manifest works without external file access.
export async function loadSlideImage({src,alt},source) {
 if(typeof src!=='string'||!src.trim()||/^[a-z][a-z\d+.-]*:|^\/\//i.test(src))throw new Error('Image src must be a local PNG path relative to the MDX file');
 if(typeof alt!=='string'||!alt.trim())throw new Error('Image needs nonempty alt text');
 const bytes=await readFile(resolve(dirname(source),src));
 if(bytes.length<33||!bytes.subarray(0,8).equals(Buffer.from([137,80,78,71,13,10,26,10]))||bytes.toString('ascii',12,16)!=='IHDR')throw new Error('Image must be a PNG file');
 const width=bytes.readUInt32BE(16),height=bytes.readUInt32BE(20);
 if(!width||!height||width>16384||height>16384||bytes.length>32*1024*1024)throw new Error('Image exceeds supported PNG dimensions or file size');
 return {mime:'image/png',data:bytes.toString('base64'),width,height,alt};
}
