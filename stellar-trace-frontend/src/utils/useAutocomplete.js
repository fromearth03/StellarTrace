import { useState, useEffect, useRef } from 'react';

/**
 * Custom hook for autocomplete functionality
 * Fetches suggestions from backend immediately on every keystroke
 */
export const useAutocomplete = (query, delay = 100) => {
  const [suggestions, setSuggestions] = useState([]);
  const [loading, setLoading] = useState(false);
  const abortControllerRef = useRef(null);

  useEffect(() => {
    // Clear suggestions if query is empty
    if (!query || query.trim().length === 0) {
      setSuggestions([]);
      return;
    }

    // Cancel previous request if exists
    if (abortControllerRef.current) {
      abortControllerRef.current.abort();
    }

    // Create new abort controller for this request
    abortControllerRef.current = new AbortController();

    // Very short debounce just to batch rapid keystrokes
    const timeoutId = setTimeout(async () => {
      setLoading(true);
      
      try {
        const trimmedQuery = query.trim();
        
        // Extract the current word being typed (last word after space)
        const words = trimmedQuery.split(/\s+/);
        const currentWord = words[words.length - 1];
        const previousWords = words.slice(0, -1).join(' ');
        
        console.log(`[Autocomplete] Full query: "${trimmedQuery}"`);
        console.log(`[Autocomplete] Current word: "${currentWord}" | Previous: "${previousWords}"`);
        
        // Send only the current word to backend
        const response = await fetch(
          `http://localhost:8080/autocomplete?q=${encodeURIComponent(currentWord)}`,
          { signal: abortControllerRef.current.signal }
        );
        
        if (!response.ok) {
          throw new Error('Autocomplete request failed');
        }
        
        const data = await response.json();
        console.log(`[Autocomplete] Received ${data.length} suggestions for "${currentWord}":`, data);
        
        // Prepend previous words to each suggestion
        const processedSuggestions = Array.isArray(data) ? data.map(suggestion => {
          if (previousWords) {
            return `${previousWords} ${suggestion}`;
          }
          return suggestion;
        }) : [];
        
        setSuggestions(processedSuggestions);
      } catch (error) {
        // Ignore abort errors
        if (error.name !== 'AbortError') {
          console.error('Autocomplete error:', error);
          setSuggestions([]);
        }
      } finally {
        setLoading(false);
      }
    }, delay);

    // Cleanup function
    return () => {
      clearTimeout(timeoutId);
      if (abortControllerRef.current) {
        abortControllerRef.current.abort();
      }
    };
  }, [query, delay]);

  return { suggestions, loading };
};
