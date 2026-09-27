import fs from 'fs';

function generateModel(filename, numVars, numCons) {
  let mps = `NAME          ${filename.replace('.mps','')}\nROWS\n N  COST\n`;
  for (let i = 1; i <= numCons; i++) {
    let t = (i % 3 === 0) ? 'E' : (i % 2 === 0) ? 'G' : 'L';
    mps += ` ${t}  C${i}\n`;
  }
  mps += 'COLUMNS\n';
  for (let j = 1; j <= numVars; j++) {
    let v = `X${j}`;
    let cost = (j % 5 + 1) * 2.0;
    let c1 = ((j - 1) % numCons) + 1;
    let c2 = (j % numCons) + 1;
    mps += `    ${v.padEnd(8)}  COST      ${cost.toFixed(1).padEnd(8)}  C${c1.toString().padEnd(4)}  1.0\n`;
    if (c1 !== c2) {
      mps += `    ${v.padEnd(8)}  C${c2.toString().padEnd(4)}  2.0\n`;
    }
  }
  mps += 'RHS\n';
  for (let i = 1; i <= numCons; i++) {
    mps += `    RHS1      C${i.toString().padEnd(4)}  ${(i * 10.0).toFixed(1)}\n`;
  }
  mps += 'BOUNDS\n';
  for (let j = 1; j <= numVars; j++) {
    let v = `X${j}`;
    if (j % 10 === 0) {
      mps += ` FX BND      ${v.padEnd(8)}  5.0\n`;
    } else if (j % 7 === 0) {
      mps += ` FR BND      ${v.padEnd(8)}\n`;
    } else {
      mps += ` UP BND      ${v.padEnd(8)}  50.0\n`;
      mps += ` LO BND      ${v.padEnd(8)}  0.0\n`;
    }
  }
  mps += 'ENDATA\n';
  fs.writeFileSync(filename, mps);
  console.log(`Successfully generated ${filename} (${numVars} vars, ${numCons} rows)`);
}

generateModel('synthetic_114_vars.mps', 114, 67);
generateModel('synthetic_300_vars.mps', 300, 150);
generateModel('synthetic_1000_vars.mps', 1000, 400);
