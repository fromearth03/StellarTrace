import { motion } from 'framer-motion';

const MilkyWay3D = () => {
  return (
    <div className="absolute inset-0 w-full h-full overflow-hidden">
      {/* High-res space background */}
      <motion.div
        className="absolute inset-0 w-full h-full"
        style={{
          backgroundImage: 'url(/space_bg.jpg)',
          backgroundSize: 'cover',
          backgroundPosition: 'center',
          filter: 'brightness(0.5)',
        }}
        initial={{ scale: 1.1 }}
        animate={{ 
          scale: 1.05,
        }}
        transition={{
          duration: 30,
          repeat: Infinity,
          repeatType: "reverse",
          ease: "easeInOut"
        }}
      />
      
      {/* Subtle overlay */}
      <div className="absolute inset-0 bg-gradient-to-b from-black/30 via-transparent to-black/40" />
    </div>
  );
};

export default MilkyWay3D;
