import { motion, AnimatePresence } from 'framer-motion';
import { Search, Clock, X } from 'lucide-react';
import { useState, useEffect } from 'react';
import MilkyWay3D from './MilkyWay3D';

const Observatory = ({ onSearch }) => {
  const [query, setQuery] = useState('');
  const [isFocused, setIsFocused] = useState(false);
  const [recentSearches, setRecentSearches] = useState([]);
  const [showSuggestions, setShowSuggestions] = useState(false);

  // Load recent searches from localStorage on mount
  useEffect(() => {
    const saved = localStorage.getItem('stellarTraceRecentSearches');
    if (saved) {
      setRecentSearches(JSON.parse(saved));
    }
  }, []);

  // Save to localStorage whenever recentSearches changes
  const saveSearch = (searchQuery) => {
    const trimmed = searchQuery.trim();
    if (!trimmed) return;

    const updated = [trimmed, ...recentSearches.filter(s => s !== trimmed)].slice(0, 5);
    setRecentSearches(updated);
    localStorage.setItem('stellarTraceRecentSearches', JSON.stringify(updated));
  };

  const deleteSearch = (searchQuery, e) => {
    e.stopPropagation();
    const updated = recentSearches.filter(s => s !== searchQuery);
    setRecentSearches(updated);
    localStorage.setItem('stellarTraceRecentSearches', JSON.stringify(updated));
  };

  const handleSubmit = (e) => {
    e.preventDefault();
    if (query.trim()) {
      saveSearch(query);
      onSearch(query);
    }
  };

  const handleRecentClick = (searchQuery) => {
    setQuery(searchQuery);
    setShowSuggestions(false);
    onSearch(searchQuery);
  };

  // Filter recent searches based on current query
  const filteredSearches = query.trim() 
    ? recentSearches.filter(s => s.toLowerCase().includes(query.toLowerCase()))
    : recentSearches;

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      transition={{ duration: 0.4 }}
      className="relative w-full h-full flex items-center justify-center overflow-hidden"
    >
      {/* 3D Milky Way Galaxy Background */}
      <MilkyWay3D />

      {/* Search Interface */}
      <motion.div
        className="relative z-10 w-full max-w-5xl px-4 sm:px-6 md:px-8"
        initial={{ y: 100, opacity: 0 }}
        animate={{ y: 0, opacity: 1 }}
        transition={{ delay: 0.5, duration: 1 }}
      >
        <motion.div
          className="mb-8 sm:mb-12 text-center"
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ delay: 0.8 }}
        >
          <h1 className="text-4xl sm:text-5xl md:text-6xl font-light tracking-wider mb-2 sm:mb-3" style={{ fontFamily: 'Inter' }}>
            STELLAR<span className="font-semibold">TRACE</span>
          </h1>
          <p className="text-xs sm:text-sm font-mono tracking-widest text-gray-400 uppercase">
            Scientific Research Observatory
          </p>
          <p className="text-lg sm:text-xl md:text-2xl font-light tracking-wide text-gray-300 mt-4 sm:mt-6">
            Explore the Cosmos
          </p>
        </motion.div>

        <form onSubmit={handleSubmit} className="relative">
          <motion.div
            className="relative"
            whileHover={{ scale: 1.01 }}
            transition={{ duration: 0.2 }}
          >
            <div className="absolute inset-0 bg-overlay-light backdrop-blur-sm border border-white/20" 
                 style={{ borderRadius: '2px' }} 
            />
            
            <div className="relative flex items-center gap-3 sm:gap-4 px-4 sm:px-6">
              <div className="flex-shrink-0">
                <Search className="text-gray-400" size={20} />
              </div>
              
              <input
                type="text"
                value={query}
                onChange={(e) => setQuery(e.target.value)}
                onFocus={() => {
                  setIsFocused(true);
                  setShowSuggestions(true);
                }}
                onBlur={() => {
                  setIsFocused(false);
                  setTimeout(() => setShowSuggestions(false), 200);
                }}
                placeholder="ENTER SEARCH PARAMETERS"
                className="flex-1 py-6 sm:py-8 bg-transparent text-white placeholder-gray-500 outline-none font-mono text-sm sm:text-base md:text-lg tracking-wider"
                autoFocus
              />
            </div>

            {/* Corner Brackets */}
            <div className="absolute top-0 left-0 w-4 h-4 sm:w-6 sm:h-6 border-t-2 border-l-2 border-white/40" />
            <div className="absolute top-0 right-0 w-4 h-4 sm:w-6 sm:h-6 border-t-2 border-r-2 border-white/40" />
            <div className="absolute bottom-0 left-0 w-4 h-4 sm:w-6 sm:h-6 border-b-2 border-l-2 border-white/40" />
            <div className="absolute bottom-0 right-0 w-4 h-4 sm:w-6 sm:h-6 border-b-2 border-r-2 border-white/40" />
          </motion.div>

          {/* Recent Searches Dropdown */}
          <AnimatePresence>
            {showSuggestions && filteredSearches.length > 0 && (
              <motion.div
                initial={{ opacity: 0, y: -10 }}
                animate={{ opacity: 1, y: 0 }}
                exit={{ opacity: 0, y: -10 }}
                transition={{ duration: 0.2 }}
                className="absolute top-full mt-3 w-full z-20"
              >
                <div className="relative">
                  <div className="absolute inset-0 bg-overlay-dark backdrop-blur-md border border-white/20" />
                  <div className="relative p-2">
                    <div className="text-[10px] font-mono text-gray-500 tracking-widest px-3 py-2">
                      RECENT SEARCHES
                    </div>
                    {filteredSearches.map((search, index) => (
                      <motion.div
                        key={index}
                        initial={{ opacity: 0, x: -20 }}
                        animate={{ opacity: 1, x: 0 }}
                        transition={{ delay: index * 0.05 }}
                        className="relative group cursor-pointer"
                        onMouseDown={() => handleRecentClick(search)}
                      >
                        <div className="absolute inset-0 bg-white/5 opacity-0 group-hover:opacity-100 transition-opacity" />
                        <div className="relative flex items-center gap-3 px-3 py-3">
                          <Clock size={14} className="text-gray-500 flex-shrink-0" />
                          <span className="text-sm text-gray-300 font-mono group-hover:text-white transition-colors flex-1">
                            {search}
                          </span>
                          <button
                            onMouseDown={(e) => deleteSearch(search, e)}
                            className="opacity-0 group-hover:opacity-100 transition-opacity text-gray-500 hover:text-red-400 p-1"
                            title="Delete"
                          >
                            <X size={14} />
                          </button>
                        </div>
                      </motion.div>
                    ))}
                  </div>
                  <div className="absolute top-0 left-0 w-3 h-3 border-t border-l border-white/30" />
                  <div className="absolute top-0 right-0 w-3 h-3 border-t border-r border-white/30" />
                  <div className="absolute bottom-0 left-0 w-3 h-3 border-b border-l border-white/30" />
                  <div className="absolute bottom-0 right-0 w-3 h-3 border-b border-r border-white/30" />
                </div>
              </motion.div>
            )}
          </AnimatePresence>

          <motion.p
            className="mt-3 sm:mt-4 text-[10px] sm:text-xs font-mono text-gray-500 text-center tracking-wider"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            transition={{ delay: 1.2 }}
          >
            [ PRESS ENTER TO INITIATE SEARCH SEQUENCE ]
          </motion.p>
        </form>
      </motion.div>

      {/* Bottom HUD */}
      <motion.div
        className="absolute bottom-4 sm:bottom-8 left-4 sm:left-8 font-mono text-[8px] sm:text-xs text-gray-600 tracking-wider hidden sm:block"
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        transition={{ delay: 1.5 }}
      >
        <div>SYS.STATUS: OPERATIONAL</div>
        <div>TELEMETRY: ACTIVE</div>
        <div>COORD: 41.2565°N, 95.9345°W</div>
      </motion.div>

      <motion.div
        className="absolute bottom-4 sm:bottom-8 right-4 sm:right-8 font-mono text-[8px] sm:text-xs text-gray-600 tracking-wider text-right hidden sm:block"
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        transition={{ delay: 1.5 }}
      >
        <div>UTC: {new Date().toISOString().slice(0, 19).replace('T', ' ')}</div>
        <div>NETWORK: SECURE</div>
      </motion.div>

      {/* Mobile-only simplified HUD */}
      <motion.div
        className="absolute bottom-2 left-2 right-2 font-mono text-[9px] text-gray-600 tracking-wider flex justify-between sm:hidden"
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        transition={{ delay: 1.5 }}
      >
        <div>SYS: OPERATIONAL</div>
        <div>NETWORK: SECURE</div>
      </motion.div>
    </motion.div>
  );
};

export default Observatory;
