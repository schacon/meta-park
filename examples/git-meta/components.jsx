import React from 'react';
import {Model, Animate} from '../../src/components.jsx';
export function MetadataOrbit() {
  return <>{[0,1,2].map(i => <Animate key={i} kind="spin" speed={15+i*15}><Model mesh="/Engine/BasicShapes/Cube.Cube" position={[i*100,-1050,i*190-190]} scale={[1.1,1.1,1.1]} /></Animate>)}</>;
}
export function DataStore() {
  return <Animate kind="bob" speed={1.5} amplitude={25}><Model mesh="/Engine/BasicShapes/Cylinder.Cylinder" position={[0,-1050,0]} scale={[2.5,2.5,3.5]} /></Animate>;
}
