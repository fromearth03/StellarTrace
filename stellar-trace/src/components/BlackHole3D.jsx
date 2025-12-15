import { Canvas, useLoader } from '@react-three/fiber';
import { OrbitControls, Sphere } from '@react-three/drei';
import * as THREE from 'three';

function Moon() {
  // Load Moon textures from Three.js examples (same source as Earth/Mars)
  const moonTexture = useLoader(THREE.TextureLoader, 'https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/planets/moon_1024.jpg');
  
  return (
    <Sphere args={[2.5, 128, 128]} rotation={[0, 0, 0.1]}>
      <meshStandardMaterial
        map={moonTexture}
        roughness={1}
        metalness={0}
      />
    </Sphere>
  );
}

const BlackHole3D = () => {
  return (
    <div className="w-full h-full">
      <Canvas camera={{ position: [0, 0, 8], fov: 45 }}>
        {/* Lighting */}
        <ambientLight intensity={0.3} />
        <directionalLight position={[-5, 3, 5]} intensity={1.5} color="#ffffff" />
        <pointLight position={[5, 5, 5]} intensity={0.5} color="#ffffff" />
        
        {/* Moon with rotation */}
        <group rotation={[0.1, 0, 0]}>
          <Moon />
        </group>
        
        {/* Camera controls for interaction */}
        <OrbitControls
          enableZoom={false}
          enablePan={false}
          autoRotate
          autoRotateSpeed={0.5}
          minPolarAngle={Math.PI / 3}
          maxPolarAngle={Math.PI / 1.5}
        />
      </Canvas>
    </div>
  );
};

export default BlackHole3D;
