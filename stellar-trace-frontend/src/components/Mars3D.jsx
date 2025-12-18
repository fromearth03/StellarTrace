import { Canvas, useLoader } from '@react-three/fiber';
import { OrbitControls, Sphere } from '@react-three/drei';
import * as THREE from 'three';

function Mars() {
  // Load Mars texture from local public folder
  const marsTexture = useLoader(THREE.TextureLoader, '/mars_color.jpg');
  
  return (
    <Sphere args={[2.5, 128, 128]} rotation={[0, 0, 0.1]}>
      <meshStandardMaterial
        map={marsTexture}
        roughness={0.95}
        metalness={0.1}
      />
    </Sphere>
  );
}

function Atmosphere() {
  return (
    <Sphere args={[2.58, 64, 64]}>
      <meshBasicMaterial
        color="#ff9966"
        transparent
        opacity={0.08}
        side={THREE.BackSide}
      />
    </Sphere>
  );
}

const Mars3D = () => {
  return (
    <div className="w-full h-full">
      <Canvas camera={{ position: [0, 0, 8], fov: 45 }}>
        {/* Lighting */}
        <ambientLight intensity={0.3} />
        <directionalLight position={[-5, 3, 5]} intensity={1.5} color="#fff5e6" />
        <pointLight position={[5, 5, 5]} intensity={0.5} color="#ffffff" />
        
        {/* Mars with rotation */}
        <group rotation={[0.1, 0, 0]}>
          <Mars />
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

export default Mars3D;
