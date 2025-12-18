import { motion, AnimatePresence } from 'framer-motion';
import { Database, ArrowLeft, Search, Sparkles, Clock, X } from 'lucide-react';
import { useState, useEffect } from 'react';
import Earth3D from './Earth3D';
import Mars3D from './Mars3D';
import BlackHole3D from './BlackHole3D';
import { useAutocomplete } from '../utils/useAutocomplete';

const OrbitalView = ({ query, onBack, selectedArticle, onArticleClick, onCloseArticle, onSearch }) => {
  const [results, setResults] = useState([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [editableQuery, setEditableQuery] = useState(query);
  const [isEditing, setIsEditing] = useState(false);
  const [aiResponse, setAiResponse] = useState('');
  const [aiLoading, setAiLoading] = useState(false);
  const [aiError, setAiError] = useState(null);
  const [aiExpanded, setAiExpanded] = useState(false);
  const [recentSearches, setRecentSearches] = useState([]);
  const [showSuggestions, setShowSuggestions] = useState(false);
  const [visibleSuggestions, setVisibleSuggestions] = useState(10);
  
  // Autocomplete hook
  const { suggestions: autocompleteSuggestions, loading: autocompleteLoading } = useAutocomplete(editableQuery);
  
  // Reset visible count when query changes
  useEffect(() => {
    setVisibleSuggestions(10);
  }, [editableQuery]);

  useEffect(() => {
    setEditableQuery(query);
  }, [query]);

  // Load recent searches from localStorage
  useEffect(() => {
    const stored = localStorage.getItem('stellarTraceRecentSearches');
    if (stored) {
      setRecentSearches(JSON.parse(stored));
    }
  }, []);

  // Save search to recent searches
  const saveSearch = (searchQuery) => {
    const trimmed = searchQuery.trim();
    if (!trimmed) return;

    const updated = [trimmed, ...recentSearches.filter(q => q !== trimmed)].slice(0, 5);
    setRecentSearches(updated);
    localStorage.setItem('stellarTraceRecentSearches', JSON.stringify(updated));
  };

  const deleteSearch = (searchQuery, e) => {
    e.stopPropagation();
    const updated = recentSearches.filter(q => q !== searchQuery);
    setRecentSearches(updated);
    localStorage.setItem('stellarTraceRecentSearches', JSON.stringify(updated));
  };

  useEffect(() => {
    const fetchResults = async () => {
      setLoading(true);
      setError(null);
      try {
        const response = await fetch(`http://localhost:8080/search?q=${encodeURIComponent(query)}`);
        if (!response.ok) {
          throw new Error('Search failed');
        }
        const data = await response.json();
        setResults(data);
      } catch (err) {
        setError(err.message);
        console.error('Search error:', err);
      } finally {
        setLoading(false);
      }
    };

    if (query) {
      fetchResults();
    }
  }, [query]);

  useEffect(() => {
    const fetchAiResponse = async () => {
      setAiLoading(true);
      setAiError(null);
      setAiResponse('');
      
      try {
        // Check if puter is available
        if (typeof window.puter === 'undefined') {
          throw new Error('Puter.js not loaded');
        }

        // Use Puter.js for free unlimited Gemini API access with streaming
        const stream = await window.puter.ai.chat(
          `Provide a comprehensive scientific summary about: ${query}. Focus on key concepts, recent developments, and important research areas. Keep it informative and academic.`,
          {
            model: 'gemini-2.5-flash',
            stream: true
          }
        );

        let fullResponse = '';
        for await (const part of stream) {
          if (part?.text) {
            fullResponse += part.text;
            setAiResponse(fullResponse);
          }
        }
      } catch (err) {
        setAiError(err.message);
        console.error('AI error:', err);
      } finally {
        setAiLoading(false);
      }
    };

    if (query) {
      fetchAiResponse();
    }
  }, [query]);

  const handleSearchSubmit = (e) => {
    e.preventDefault();
    if (editableQuery.trim() && editableQuery !== query) {
      saveSearch(editableQuery.trim());
      onSearch(editableQuery.trim());
    }
    setIsEditing(false);
    setShowSuggestions(false);
  };

  const getFirstWords = (text, count = 20) => {
    if (!text) return '';
    const words = text.trim().split(/\s+/);
    return words.slice(0, count).join(' ') + (words.length > count ? '...' : '');
  };

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      transition={{ duration: 0.4 }}
      className="relative w-full h-full flex flex-col lg:flex-row overflow-hidden"
    >
      {/* Left Panel - 3D Planet */}
      <motion.div
        className="relative w-full lg:w-2/5 h-64 lg:h-full flex items-center justify-center"
        initial={{ x: -100, opacity: 0 }}
        animate={{ x: 0, opacity: 1 }}
        transition={{ duration: 0.4, delay: 0.1 }}
      >
        <div className="w-full h-full">
          {selectedArticle ? (
            <Mars3D />
          ) : !loading && results.length === 0 && !error ? (
            <BlackHole3D />
          ) : (
            <Earth3D />
          )}
        </div>

        <motion.div
          className="absolute bottom-4 lg:bottom-20 left-1/2 -translate-x-1/2 font-mono text-[10px] sm:text-xs text-gray-400 tracking-widest text-center z-10"
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ delay: 0.6 }}
        >
          <div>OBSERVATION TARGET</div>
          <div className="text-white mt-1">
            {selectedArticle ? 'VALTHOR' : (!loading && results.length === 0 && !error ? 'ZOMBIE PLANET' : 'NEXARA')}
          </div>
          <div className="mt-2 text-[8px] sm:text-[10px]">
            {selectedArticle ? 'RED WASTELAND' : (!loading && results.length === 0 && !error ? 'UNDEAD WORLD' : 'PRIME WORLD')}
          </div>
        </motion.div>
      </motion.div>

      {/* Right Panel - Results or Article */}
      <motion.div
        className="relative w-full lg:w-3/5 h-full flex flex-col p-4 sm:p-6 lg:p-8"
        initial={{ x: 100, opacity: 0 }}
        animate={{ x: 0, opacity: 1 }}
        transition={{ duration: 0.4, delay: 0.15 }}
      >
        {selectedArticle ? (
          // Article Detail View
          <>
            <div className="mb-4 sm:mb-6">
              <button
                onClick={onCloseArticle}
                className="flex items-center gap-2 text-gray-400 hover:text-white transition-colors font-mono text-[10px] sm:text-xs tracking-wider"
              >
                <ArrowLeft size={14} className="sm:w-4 sm:h-4" />
                BACK TO RESULTS
              </button>
            </div>

            <div className="flex-1 overflow-y-auto pr-2 sm:pr-4 custom-scrollbar">
              <motion.div
                initial={{ opacity: 0, y: 20 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ duration: 0.3 }}
                className="space-y-4 sm:space-y-6"
              >
                <div className="relative">
                  <div className="absolute inset-0 bg-overlay-dark border border-white/20" />
                  <div className="relative p-4 sm:p-6">
                    <div className="font-mono text-[10px] text-gray-500 tracking-widest mb-2 sm:mb-3">
                      {selectedArticle.id}
                    </div>
                    <h1 className="text-lg sm:text-xl md:text-2xl font-light text-white leading-relaxed mb-3 sm:mb-4">
                      {selectedArticle.title}
                    </h1>
                    <div className="text-xs sm:text-sm text-gray-400 mb-2">
                      {selectedArticle.authors}
                    </div>
                    {selectedArticle['journal-ref'] && (
                      <div className="text-xs text-gray-500 font-mono">
                        {selectedArticle['journal-ref']}
                      </div>
                    )}
                    {selectedArticle.doi && (
                      <div className="text-xs text-blue-400 font-mono mt-1">
                        DOI: {selectedArticle.doi}
                      </div>
                    )}
                  </div>
                  <div className="absolute top-0 left-0 w-4 h-4 border-t border-l border-white/40" />
                  <div className="absolute top-0 right-0 w-4 h-4 border-t border-r border-white/40" />
                  <div className="absolute bottom-0 left-0 w-4 h-4 border-b border-l border-white/40" />
                  <div className="absolute bottom-0 right-0 w-4 h-4 border-b border-r border-white/40" />
                </div>

                <div className="relative">
                  <div className="absolute inset-0 bg-overlay-dark border border-white/20" />
                  <div className="relative p-4 sm:p-6">
                    <h2 className="text-xs sm:text-sm font-mono text-gray-400 tracking-widest mb-3 sm:mb-4">ABSTRACT</h2>
                    <p className="text-sm sm:text-base text-gray-300 leading-relaxed whitespace-pre-wrap">
                      {selectedArticle.abstract}
                    </p>
                  </div>
                </div>

                {selectedArticle.categories && (
                  <div className="relative">
                    <div className="absolute inset-0 bg-overlay-dark border border-white/20" />
                    <div className="relative p-4 sm:p-6">
                      <h2 className="text-xs sm:text-sm font-mono text-gray-400 tracking-widest mb-3">CATEGORIES</h2>
                      <div className="flex flex-wrap gap-2">
                        {selectedArticle.categories.split(' ').map((cat, i) => (
                          <span key={i} className="px-2 sm:px-3 py-1 bg-white/5 border border-white/10 text-[10px] sm:text-xs font-mono text-gray-400">
                            {cat}
                          </span>
                        ))}
                      </div>
                    </div>
                  </div>
                )}
              </motion.div>
            </div>
          </>
        ) : (
          // Results List
          <>
            <div className="mb-4 sm:mb-6">
              <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between mb-3 sm:mb-4 gap-2">
                <button
                  onClick={onBack}
                  className="flex items-center gap-2 text-gray-400 hover:text-white transition-colors font-mono text-[10px] sm:text-xs tracking-wider"
                >
                  <ArrowLeft size={14} className="sm:w-4 sm:h-4" />
                  <span className="hidden sm:inline">RETURN TO OBSERVATORY</span>
                  <span className="sm:hidden">BACK</span>
                </button>
                <div className="font-mono text-[10px] sm:text-xs text-gray-500">
                  {loading ? 'SEARCHING...' : `${results.length} RESULTS FOUND`}
                </div>
              </div>

              <div className="relative">
                <div className="absolute inset-0 bg-overlay-light backdrop-blur-sm border border-white/20" />
                <form onSubmit={handleSearchSubmit} className="relative py-2 sm:py-3 px-3 sm:px-5 font-mono text-xs sm:text-sm text-gray-300 tracking-wide flex items-center gap-2 sm:gap-3">
                  <span className="text-gray-400 hidden sm:inline">QUERY:</span>
                  {isEditing ? (
                    <input
                      type="text"
                      value={editableQuery}
                      onChange={(e) => setEditableQuery(e.target.value)}
                      onFocus={() => setShowSuggestions(true)}
                      onBlur={() => setTimeout(() => setShowSuggestions(false), 200)}
                      autoFocus
                      className="flex-1 bg-transparent text-white outline-none border-b border-white/30 focus:border-white/60 transition-colors"
                    />
                  ) : (
                    <span 
                      className="flex-1 text-white cursor-text hover:text-blue-300 transition-colors"
                      onClick={() => {
                        setIsEditing(true);
                        setShowSuggestions(true);
                      }}
                    >
                      {query}
                    </span>
                  )}
                  <button
                    type="submit"
                    className="text-gray-400 hover:text-white transition-colors"
                    title="Search"
                  >
                    <Search size={16} />
                  </button>
                </form>
                <div className="absolute top-0 left-0 w-4 h-4 border-t border-l border-white/40" />
                <div className="absolute top-0 right-0 w-4 h-4 border-t border-r border-white/40" />
                <div className="absolute bottom-0 left-0 w-4 h-4 border-b border-l border-white/40" />
                <div className="absolute bottom-0 right-0 w-4 h-4 border-b border-r border-white/40" />
                
                {/* Autocomplete & Recent Searches Dropdown */}
                <AnimatePresence>
                  {isEditing && showSuggestions && (
                    autocompleteSuggestions.length > 0 || 
                    recentSearches.filter(s => s.toLowerCase().includes(editableQuery.toLowerCase())).length > 0 ||
                    autocompleteLoading
                  ) && (
                    <motion.div
                      initial={{ opacity: 0, y: -10 }}
                      animate={{ opacity: 1, y: 0 }}
                      exit={{ opacity: 0, y: -10 }}
                      transition={{ duration: 0.2 }}
                      className="absolute top-full left-0 right-0 mt-2 z-50 max-h-[500px] overflow-y-auto custom-scrollbar"
                    >
                      <div className="absolute inset-0 bg-overlay-light backdrop-blur-sm border border-white/20" />
                      <div className="relative py-2">
                        {autocompleteLoading && editableQuery.trim() && (
                          <div className="px-4 py-2 font-mono text-[10px] text-gray-500 tracking-widest flex items-center gap-2">
                            <div className="w-3 h-3 border-2 border-gray-500 border-t-transparent rounded-full animate-spin" />
                            SEARCHING...
                          </div>
                        )}
                        
                        {/* Autocomplete suggestions */}
                        {editableQuery.trim() && autocompleteSuggestions.length > 0 && (
                          <>
                            <div className="px-4 py-2 font-mono text-[10px] text-gray-500 tracking-widest">
                              SUGGESTIONS ({autocompleteSuggestions.length})
                            </div>
                            <motion.div
                              className="space-y-1"
                              variants={{
                                show: {
                                  transition: {
                                    staggerChildren: 0.03
                                  }
                                }
                              }}
                              initial="hidden"
                              animate="show"
                            >
                              {autocompleteSuggestions.slice(0, visibleSuggestions).map((suggestion, index) => (
                                <motion.div
                                  key={`auto-${index}`}
                                  variants={{
                                    hidden: { opacity: 0, x: -10 },
                                    show: { opacity: 1, x: 0 }
                                  }}
                                  className="px-4 py-2 font-mono text-sm text-gray-300 hover:text-white hover:bg-white/5 transition-all cursor-pointer flex items-center gap-3"
                                  onMouseDown={(e) => {
                                    e.preventDefault();
                                    setEditableQuery(suggestion);
                                    saveSearch(suggestion);
                                    onSearch(suggestion);
                                    setIsEditing(false);
                                    setShowSuggestions(false);
                                  }}
                                >
                                  <Sparkles size={14} className="text-blue-400" />
                                  <span className="flex-1">{suggestion}</span>
                                </motion.div>
                              ))}
                            </motion.div>
                            
                            {/* Show More/Less buttons */}
                            {autocompleteSuggestions.length > 10 && (
                              <div className="px-4 py-2 flex justify-center gap-3">
                                {visibleSuggestions < autocompleteSuggestions.length && (
                                  <button
                                    onMouseDown={(e) => {
                                      e.preventDefault();
                                      setVisibleSuggestions(prev => Math.min(prev + 10, autocompleteSuggestions.length));
                                    }}
                                    className="px-3 py-1.5 text-[10px] font-mono text-blue-400 border border-blue-400/30 rounded hover:bg-blue-400/10 transition-colors tracking-wider"
                                  >
                                    SHOW MORE ({autocompleteSuggestions.length - visibleSuggestions} MORE)
                                  </button>
                                )}
                                {visibleSuggestions > 10 && (
                                  <button
                                    onMouseDown={(e) => {
                                      e.preventDefault();
                                      setVisibleSuggestions(10);
                                    }}
                                    className="px-3 py-1.5 text-[10px] font-mono text-gray-400 border border-gray-400/30 rounded hover:bg-gray-400/10 transition-colors tracking-wider"
                                  >
                                    SHOW LESS
                                  </button>
                                )}
                              </div>
                            )}
                          </>
                        )}
                        
                        {/* Recent searches */}
                        {recentSearches.filter(s => s.toLowerCase().includes(editableQuery.toLowerCase())).length > 0 && (
                          <>
                            {editableQuery.trim() && autocompleteSuggestions.length > 0 && (
                              <div className="border-t border-white/10 my-2" />
                            )}
                            <div className="px-4 py-2 font-mono text-[10px] text-gray-500 tracking-widest">
                              RECENT SEARCHES
                            </div>
                            <motion.div
                              className="space-y-1"
                              variants={{
                                show: {
                                  transition: {
                                    staggerChildren: 0.03
                                  }
                                }
                              }}
                              initial="hidden"
                              animate="show"
                            >
                              {recentSearches
                                .filter(s => s.toLowerCase().includes(editableQuery.toLowerCase()))
                                .map((search, index) => (
                                  <motion.div
                                    key={`recent-${index}`}
                                    variants={{
                                      hidden: { opacity: 0, x: -10 },
                                      show: { opacity: 1, x: 0 }
                                    }}
                                    className="px-4 py-2 font-mono text-sm text-gray-300 hover:text-white hover:bg-white/5 transition-all cursor-pointer flex items-center gap-3 group"
                                  >
                                    <Clock size={14} className="text-gray-500" />
                                    <span 
                                      className="flex-1"
                                      onMouseDown={(e) => {
                                        e.preventDefault();
                                        setEditableQuery(search);
                                        saveSearch(search);
                                        onSearch(search);
                                        setIsEditing(false);
                                        setShowSuggestions(false);
                                      }}
                                    >
                                      {search}
                                    </span>
                                    <button
                                      onMouseDown={(e) => deleteSearch(search, e)}
                                      className="opacity-0 group-hover:opacity-100 transition-opacity text-gray-500 hover:text-red-400 p-1"
                                      title="Delete"
                                    >
                                      <X size={14} />
                                    </button>
                                  </motion.div>
                                ))}
                            </motion.div>
                          </>
                        )}
                      </div>
                      <div className="absolute top-0 left-0 w-4 h-4 border-t border-l border-white/40" />
                      <div className="absolute top-0 right-0 w-4 h-4 border-t border-r border-white/40" />
                      <div className="absolute bottom-0 left-0 w-4 h-4 border-b border-l border-white/40" />
                      <div className="absolute bottom-0 right-0 w-4 h-4 border-b border-r border-white/40" />
                    </motion.div>
                  )}
                </AnimatePresence>
              </div>
            </div>

            {/* AI Summary Section */}
            <div className="mb-4 sm:mb-6">
              <div 
                className="relative cursor-pointer group"
                onClick={() => setAiExpanded(!aiExpanded)}
              >
                <div className="absolute inset-0 bg-gradient-to-br from-blue-950/20 to-purple-950/20 backdrop-blur-sm border border-blue-400/30 group-hover:border-blue-400/50 transition-colors" />
                <div className="relative p-3 sm:p-4">
                  <div className="flex items-center justify-between mb-2">
                    <div className="flex items-center gap-2">
                      <Sparkles size={12} className="sm:w-3.5 sm:h-3.5 text-blue-400" />
                      <h3 className="font-mono text-[10px] sm:text-xs text-blue-400 tracking-widest">AI SUMMARY</h3>
                    </div>
                    <div className="flex items-center gap-2">
                      {aiLoading && (
                        <div className="animate-spin rounded-full h-2.5 w-2.5 sm:h-3 sm:w-3 border-2 border-blue-400 border-t-transparent" />
                      )}
                      <span className="text-[8px] sm:text-[10px] text-gray-500 font-mono hidden sm:inline">
                        {aiExpanded ? 'CLICK TO COLLAPSE' : 'CLICK TO EXPAND'}
                      </span>
                    </div>
                  </div>
                  
                  {aiError && (
                    <div className="text-red-400 font-mono text-[10px] sm:text-xs">
                      Error: {aiError}
                    </div>
                  )}
                  
                  {!aiError && aiResponse && (
                    <div className="text-gray-300 text-[11px] sm:text-xs leading-relaxed">
                      {aiExpanded ? (
                        <div className="whitespace-pre-wrap max-h-[300px] sm:max-h-[500px] overflow-y-auto pr-2 custom-scrollbar">
                          {aiResponse}
                        </div>
                      ) : (
                        <div className="line-clamp-2">
                          {aiResponse.slice(0, 150)}...
                        </div>
                      )}
                    </div>
                  )}
                  
                  {aiLoading && !aiResponse && (
                    <div className="flex items-center gap-2 text-gray-400 font-mono text-[10px] sm:text-xs">
                      <span>Generating AI response...</span>
                    </div>
                  )}
                </div>
                <div className="absolute top-0 left-0 w-3 h-3 border-t border-l border-blue-400/50 group-hover:border-blue-400/70 transition-colors" />
                <div className="absolute top-0 right-0 w-3 h-3 border-t border-r border-blue-400/50 group-hover:border-blue-400/70 transition-colors" />
                <div className="absolute bottom-0 left-0 w-3 h-3 border-b border-l border-blue-400/50 group-hover:border-blue-400/70 transition-colors" />
                <div className="absolute bottom-0 right-0 w-3 h-3 border-b border-r border-blue-400/50 group-hover:border-blue-400/70 transition-colors" />
              </div>
            </div>

            {loading && (
              <div className="flex-1 flex items-center justify-center">
                <div className="text-gray-400 font-mono text-sm">Loading results...</div>
              </div>
            )}

            {error && (
              <div className="flex-1 flex items-center justify-center">
                <div className="text-red-400 font-mono text-sm">Error: {error}</div>
              </div>
            )}

            {!loading && !error && results.length === 0 && (
              <div className="flex-1 flex items-center justify-center">
                <div className="text-center space-y-4">
                  <div className="text-gray-400 font-mono text-lg tracking-wider">NO RESULTS FOUND</div>
                  <div className="text-gray-500 text-sm">
                    We couldn't find any articles matching your search.
                  </div>
                  <div className="text-gray-600 text-xs font-mono">
                    Try different keywords or broader terms
                  </div>
                </div>
              </div>
            )}

            {!loading && !error && results.length > 0 && (
              <div className="flex-1 overflow-y-auto pr-2 sm:pr-4 space-y-3 sm:space-y-4 custom-scrollbar">
                {results.map((result, index) => (
                  <motion.div
                    key={result.id}
                    className="relative group cursor-pointer"
                    initial={{ opacity: 0, y: 20 }}
                    animate={{ opacity: 1, y: 0 }}
                    transition={{ delay: 0.3 + index * 0.05, duration: 0.3 }}
                    onClick={() => onArticleClick(result)}
                  >
                    <div className="absolute inset-0 bg-overlay-dark border border-white/10 group-hover:border-white/30 transition-colors" />
                    
                    <div className="relative p-3 sm:p-5">
                      <div className="flex flex-col sm:flex-row sm:items-start justify-between mb-2 sm:mb-3 gap-1 sm:gap-0">
                        <div className="font-mono text-[9px] sm:text-[10px] text-gray-500 tracking-widest">
                          {result.id}
                        </div>
                        <div className="flex items-center gap-2 sm:gap-3 text-[9px] sm:text-[10px] text-gray-500 font-mono">
                          {result.update_date && (
                            <div className="flex items-center gap-1">
                              <Database size={9} className="sm:w-2.5 sm:h-2.5" />
                              <span className="hidden sm:inline">{result.update_date}</span>
                            </div>
                          )}
                        </div>
                      </div>

                      <h3 className="text-base sm:text-lg font-light text-white leading-snug mb-2 group-hover:text-blue-300 transition-colors">
                        {result.title}
                      </h3>

                      <div className="text-xs sm:text-sm text-gray-400 mb-2 sm:mb-3 line-clamp-1">
                        {result.authors}
                      </div>

                      <p className="text-[11px] sm:text-xs text-gray-500 leading-relaxed line-clamp-2 sm:line-clamp-none">
                        {getFirstWords(result.abstract, 20)}
                      </p>

                      {result.categories && (
                        <div className="mt-2 sm:mt-3 flex flex-wrap gap-1.5 sm:gap-2">
                          {result.categories.split(' ').slice(0, 3).map((cat, i) => (
                            <span key={i} className="px-1.5 sm:px-2 py-0.5 sm:py-1 bg-white/5 border border-white/10 text-[9px] sm:text-[10px] font-mono text-gray-500">
                              {cat}
                            </span>
                          ))}
                        </div>
                      )}
                    </div>

                    <div className="absolute top-0 left-0 w-2 h-2 sm:w-3 sm:h-3 border-t border-l border-white/20 group-hover:border-white/40 transition-colors" />
                    <div className="absolute top-0 right-0 w-2 h-2 sm:w-3 sm:h-3 border-t border-r border-white/20 group-hover:border-white/40 transition-colors" />
                    <div className="absolute bottom-0 left-0 w-2 h-2 sm:w-3 sm:h-3 border-b border-l border-white/20 group-hover:border-white/40 transition-colors" />
                    <div className="absolute bottom-0 right-0 w-2 h-2 sm:w-3 sm:h-3 border-b border-r border-white/20 group-hover:border-white/40 transition-colors" />
                  </motion.div>
                ))}
              </div>
            )}
          </>
        )}
      </motion.div>

      <style>{`
        .custom-scrollbar::-webkit-scrollbar {
          width: 4px;
        }
        .custom-scrollbar::-webkit-scrollbar-track {
          background: rgba(255, 255, 255, 0.05);
        }
        .custom-scrollbar::-webkit-scrollbar-thumb {
          background: rgba(255, 255, 255, 0.2);
          border-radius: 2px;
        }
        .custom-scrollbar::-webkit-scrollbar-thumb:hover {
          background: rgba(255, 255, 255, 0.3);
        }
      `}</style>
    </motion.div>
  );
};

export default OrbitalView;
