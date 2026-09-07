import {compileDeck} from '../src/compiler.mjs';
import {mkdir, writeFile} from 'node:fs/promises';
const input = process.argv[2] ?? 'examples/git-meta/deck.mdx';
const layout = process.argv[3] ?? (process.argv[2] ? undefined : 'examples/git-meta/layout.json');
const deck = await compileDeck(input,layout);
await mkdir('unreal/Content/Slides',{recursive:true});
await writeFile('unreal/Content/Slides/deck.json',JSON.stringify(deck,null,2)+'\n');
console.log(`Compiled ${deck.slides.length} slides: ${deck.title}`);
