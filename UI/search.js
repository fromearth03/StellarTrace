/**
 * StellarTrace Semantic Search & Autocomplete
 * Handles search functionality with autocomplete suggestions and semantic search capabilities
 */

class StellarTraceSearch {
    constructor(baseUrl = 'http://localhost:8080') {
        this.baseUrl = baseUrl.endsWith('/') ? baseUrl.slice(0, -1) : baseUrl;
        this.suggestionsList = [];
        this.searchHistory = JSON.parse(localStorage.getItem('searchHistory')) || [];
        this.maxHistoryItems = 20;
        this.debounceTimer = null;
        this.debounceDelay = 300;
        this.cache = new Map();
        this.cacheExpiry = 5 * 60 * 1000; // 5 minutes
        this.init();
    }

    /**
     * Initialize event listeners and setup
     */
    init() {
        const queryInput = document.getElementById('query');
        const searchBtn = document.getElementById('btn');
        const suggestionsList = document.getElementById('suggestionsContainer');

        if (queryInput) {
            queryInput.addEventListener('input', (e) => this.handleInput(e));
            queryInput.addEventListener('keypress', (e) => this.handleKeyPress(e));
            queryInput.addEventListener('focus', () => this.showSuggestions());
            queryInput.addEventListener('blur', () => setTimeout(() => this.hideSuggestions(), 200));
        }

        if (searchBtn) {
            searchBtn.addEventListener('click', () => this.performSearch());
        }

        document.addEventListener('click', (e) => {
            if (!e.target.closest('#suggestionsContainer') && !e.target.closest('#query')) {
                this.hideSuggestions();
            }
        });
    }

    /**
     * Handle input event with debouncing
     */
    handleInput(event) {
        const query = event.target.value.trim();

        clearTimeout(this.debounceTimer);

        if (query.length === 0) {
            this.showSearchHistory();
            return;
        }

        this.debounceTimer = setTimeout(() => {
            this.fetchAutocompleteSuggestions(query);
        }, this.debounceDelay);
    }

    /**
     * Handle Enter key press
     */
    handleKeyPress(event) {
        if (event.key === 'Enter') {
            event.preventDefault();
            this.performSearch();
        }
    }

    /**
     * Fetch autocomplete suggestions from backend
     */
    async fetchAutocompleteSuggestions(query) {
        try {
            const cacheKey = `autocomplete_${query}`;
            const cached = this.cache.get(cacheKey);

            if (cached && Date.now() - cached.timestamp < this.cacheExpiry) {
                this.displaySuggestions(cached.data);
                return;
            }

            // Use AbortController for timeout (fetch doesn't natively support timeout)
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 5000);

            const response = await fetch(
                `${this.baseUrl}/search?q=${encodeURIComponent(query)}`,
                {
                    signal: controller.signal,
                    headers: {
                        'Accept': 'application/json'
                    }
                }
            );

            clearTimeout(timeoutId);

            if (!response.ok) {
                console.warn('Autocomplete fetch failed:', response.statusText);
                return;
            }

            const data = await response.json();
            const suggestions = this.extractSuggestions(data, query);

            // Cache the results
            this.cache.set(cacheKey, {
                data: suggestions,
                timestamp: Date.now()
            });

            this.displaySuggestions(suggestions);
        } catch (error) {
            if (error.name === 'AbortError') {
                console.warn('Autocomplete request timed out');
            } else {
                console.error('Error fetching suggestions:', error);
            }
        }
    }

    /**
     * Extract suggestion titles from search results
     */
    extractSuggestions(data, query) {
        const suggestions = new Set();

        if (!Array.isArray(data) || data.length === 0) {
            return [];
        }

        data.forEach((item) => {
            if (!item || typeof item !== 'object') return;
            
            // Try multiple title-like fields
            const title = item.title || item.name || item.arxiv_id || '';
            
            if (title && typeof title === 'string' && title.trim()) {
                // Limit length and clean up
                const cleaned = title.trim().substring(0, 100);
                if (cleaned.length > 0) {
                    suggestions.add(cleaned);
                }
            }
        });

        // Convert to array and limit results
        return Array.from(suggestions).slice(0, 10);
    }

    /**
     * Display autocomplete suggestions
     */
    displaySuggestions(suggestions) {
        const container = document.getElementById('suggestionsContainer');

        if (!container) {
            console.warn('Suggestions container not found');
            return;
        }

        container.innerHTML = '';

        if (suggestions.length === 0) {
            container.style.display = 'none';
            return;
        }

        const list = document.createElement('ul');
        list.className = 'suggestions-list';

        suggestions.forEach((suggestion) => {
            const item = document.createElement('li');
            item.className = 'suggestion-item';
            item.textContent = suggestion;
            item.addEventListener('click', () => this.selectSuggestion(suggestion));
            list.appendChild(item);
        });

        container.appendChild(list);
        container.style.display = 'block';
    }

    /**
     * Show search history
     */
    showSearchHistory() {
        const container = document.getElementById('suggestionsContainer');
        if (!container) return;

        if (this.searchHistory.length === 0) {
            container.style.display = 'none';
            return;
        }

        container.innerHTML = '';
        const list = document.createElement('ul');
        list.className = 'suggestions-list history-list';

        const header = document.createElement('li');
        header.className = 'suggestion-header';
        header.textContent = '📜 Recent Searches';
        list.appendChild(header);

        this.searchHistory.slice(0, 5).forEach((term) => {
            const item = document.createElement('li');
            item.className = 'suggestion-item';
            item.innerHTML = `<span>🕐 ${escapeHtml(term)}</span>`;
            item.addEventListener('click', () => this.selectSuggestion(term));
            list.appendChild(item);
        });

        container.appendChild(list);
        container.style.display = 'block';
    }

    /**
     * Hide suggestions dropdown
     */
    hideSuggestions() {
        const container = document.getElementById('suggestionsContainer');
        if (container) {
            container.style.display = 'none';
        }
    }

    /**
     * Show suggestions dropdown
     */
    showSuggestions() {
        const query = document.getElementById('query').value.trim();
        if (query.length === 0) {
            this.showSearchHistory();
        } else {
            const container = document.getElementById('suggestionsContainer');
            if (container) {
                container.style.display = 'block';
            }
        }
    }

    /**
     * Select a suggestion and populate input
     */
    selectSuggestion(suggestion) {
        const queryInput = document.getElementById('query');
        queryInput.value = suggestion;
        this.hideSuggestions();
        this.performSearch();
    }

    /**
     * Perform the search
     */
    async performSearch() {
        const query = document.getElementById('query').value.trim();

        if (!query) {
            alert('Please enter a search term');
            return;
        }

        // Add to history
        this.addToHistory(query);

        // Show loading state
        const resultsBox = document.getElementById('results');
        if (!resultsBox) {
            console.error('Results container not found');
            return;
        }
        
        resultsBox.innerHTML = '<div class="loading">🔄 Searching...</div>';

        try {
            // Use AbortController for timeout
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 10000);

            const response = await fetch(
                `${this.baseUrl}/search?q=${encodeURIComponent(query)}`,
                {
                    signal: controller.signal,
                    headers: {
                        'Accept': 'application/json'
                    }
                }
            );

            clearTimeout(timeoutId);

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }

            const data = await response.json();
            this.displayResults(data, query);
        } catch (error) {
            console.error('Search error:', error);
            
            let errorMsg = '❌ Error fetching results.';
            
            if (error.name === 'AbortError') {
                errorMsg = '❌ Request timeout. Server may be slow or unavailable.';
            } else if (error instanceof TypeError) {
                errorMsg = '❌ Network error. Check if server is running.';
            }
            
            resultsBox.innerHTML = `
                <div class="error-message">
                    ${errorMsg}
                    <ul>
                        <li>Server: ${this.baseUrl}</li>
                        <li>Endpoint: /search?q=query</li>
                        <li>Status: Check server logs for details</li>
                    </ul>
                </div>
            `;
        }
    }

    /**
     * Display search results
     */
    displayResults(data, query) {
        const resultsBox = document.getElementById('results');
        resultsBox.innerHTML = '';

        if (!Array.isArray(data) || data.length === 0) {
            resultsBox.innerHTML = '<div class="no-results">No results found.</div>';
            return;
        }

        const resultCount = document.createElement('div');
        resultCount.className = 'result-count';
        resultCount.textContent = `Found ${data.length} result(s) for "${escapeHtml(query)}"`;
        resultsBox.appendChild(resultCount);

        data.forEach((item, index) => {
            const section = document.createElement('details');
            section.className = 'result-item';

            const summary = document.createElement('summary');
            summary.className = 'result-summary';
            
            const title = item.title || item.name || `Result ${index + 1}`;
            const snippet = item.snippet || item.abstract || '';
            
            summary.innerHTML = `
                <span class="result-index">${index + 1}</span>
                <span class="result-title">${escapeHtml(title)}</span>
                ${snippet ? `<span class="result-snippet">— ${escapeHtml(snippet.substring(0, 100))}...</span>` : ''}
            `;

            const content = document.createElement('div');
            content.className = 'result-content';

            // Format content nicely
            const formattedContent = this.formatResultContent(item);
            content.innerHTML = formattedContent;

            section.appendChild(summary);
            section.appendChild(content);
            resultsBox.appendChild(section);
        });
    }

    /**
     * Format result content for display
     */
    formatResultContent(item) {
        if (!item || typeof item !== 'object') {
            return '<div class="result-field"><span>No data available</span></div>';
        }

        let html = '';

        // Define field priority order - these are fields from arxiv-metadata.json typically
        const fields = ['title', 'authors', 'abstract', 'date', 'category', 'id', 'arxiv_id', 'content'];

        // Display priority fields first
        fields.forEach((field) => {
            if (item[field] !== undefined && item[field] !== null && item[field] !== '') {
                const value = item[field];
                let displayValue = '';
                
                if (Array.isArray(value)) {
                    displayValue = value.join(', ');
                } else if (typeof value === 'string') {
                    displayValue = value;
                } else if (typeof value === 'number') {
                    displayValue = String(value);
                } else {
                    return; // Skip non-string/number values
                }

                if (displayValue.trim()) {
                    html += `
                        <div class="result-field">
                            <strong>${this.formatFieldName(field)}:</strong>
                            <span>${escapeHtml(String(displayValue).substring(0, 500))}</span>
                        </div>
                    `;
                }
            }
        });

        // Add remaining fields that aren't displayed yet
        Object.keys(item).forEach((key) => {
            if (!fields.includes(key)) {
                const value = item[key];
                
                // Skip non-string values, relevance_score, and empty values
                if (value === undefined || value === null || value === '' || typeof value === 'object') {
                    return;
                }
                
                if (typeof value === 'string' || typeof value === 'number') {
                    html += `
                        <div class="result-field">
                            <strong>${this.formatFieldName(key)}:</strong>
                            <span>${escapeHtml(String(value).substring(0, 500))}</span>
                        </div>
                    `;
                }
            }
        });

        // Fallback to JSON if no fields were added
        return html || `<div class="result-field"><pre>${escapeHtml(JSON.stringify(item, null, 2))}</pre></div>`;
    }

    /**
     * Format field names for display
     */
    formatFieldName(field) {
        return field
            .replace(/_/g, ' ')
            .split(' ')
            .map((word) => word.charAt(0).toUpperCase() + word.slice(1))
            .join(' ');
    }

    /**
     * Add search term to history
     */
    addToHistory(query) {
        // Remove duplicates
        this.searchHistory = this.searchHistory.filter((item) => item !== query);

        // Add to front
        this.searchHistory.unshift(query);

        // Limit size
        if (this.searchHistory.length > this.maxHistoryItems) {
            this.searchHistory.pop();
        }

        // Save to localStorage
        localStorage.setItem('searchHistory', JSON.stringify(this.searchHistory));
    }

    /**
     * Clear search history
     */
    clearHistory() {
        this.searchHistory = [];
        localStorage.removeItem('searchHistory');
    }

    /**
     * Get semantic search capabilities info
     */
    getSemanticCapabilities() {
        return {
            model: 'all-MiniLM-L6-v2',
            description: 'Semantic search using transformer embeddings',
            capabilities: [
                'Understand meaning beyond keywords',
                'Find semantically similar documents',
                'Context-aware search'
            ]
        };
    }
}

/**
 * Escape HTML to prevent XSS
 */
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Initialize search when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.searchEngine = new StellarTraceSearch();
});
