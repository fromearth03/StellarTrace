import { Canvas, useLoader } from '@react-three/fiber';
import { OrbitControls, Sphere } from '@react-three/drei';
import * as THREE from 'three';

function Earth() {
  // Load NASA Earth textures
  const earthTexture = useLoader(THREE.TextureLoader, 'https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/planets/earth_atmos_2048.jpg');
  const normalMap = useLoader(THREE.TextureLoader, 'https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/planets/earth_normal_2048.jpg');
  const specularMap = useLoader(THREE.TextureLoader, 'https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/planets/earth_specular_2048.jpg');
  
  return (
    <Sphere args={[2.5, 128, 128]} rotation={[0, 0, 0.1]}>
      <meshStandardMaterial
        map={earthTexture}
        normalMap={normalMap}
        roughnessMap={specularMap}
        roughness={0.9}
        metalness={0.1}
      />
    </Sphere>
  );
}

function Clouds() {
  const cloudTexture = useLoader(THREE.TextureLoader, 'https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/planets/earth_clouds_1024.png');
  
  return (
    <Sphere args={[2.52, 128, 128]}>
      <meshStandardMaterial
        map={cloudTexture}
        transparent
        opacity={0.4}
        depthWrite={false}
      />
    </Sphere>
  );
}

function Atmosphere() {
  return (
    <Sphere args={[2.58, 64, 64]}>
      <meshBasicMaterial
        color="#6ba3ff"
        transparent
        opacity={0.15}
        side={THREE.BackSide}
      />
    </Sphere>
  );
}

const Earth3D = () => {
  return (
    <div className="w-full h-full">
      <Canvas camera={{ position: [0, 0, 8], fov: 45 }}>
        {/* Lighting */}
        <ambientLight intensity={0.3} />
        <directionalLight position={[-5, 3, 5]} intensity={1.5} color="#fff5e6" />
        <pointLight position={[5, 5, 5]} intensity={0.5} color="#ffffff" />
        
        {/* Earth with rotation */}
        <group rotation={[0.1, 0, 0]}>
          <Earth />
          <Clouds />
          <Atmosphere />
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

export default Earth3D;