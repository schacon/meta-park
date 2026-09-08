import {compileDeck, compileStations} from '../src/compiler.mjs';
import {mkdir, writeFile, stat} from 'node:fs/promises';
const input = process.argv[2] ?? 'slides';
const isDirectory = (await stat(input)).isDirectory();
const layout = process.argv[3] ?? (isDirectory ? 'examples/git-meta/layout.json' : undefined);
const deck = isDirectory ? await compileStations(input,layout) : await compileDeck(input,layout);
await mkdir('unreal/Content/Slides',{recursive:true});
await writeFile('unreal/Content/Slides/deck.json',JSON.stringify(deck,null,2)+'\n');
console.log(`Compiled ${deck.slides.length} slides: ${deck.title}`);
