import { useState } from 'react';
import { AnimatePresence } from 'framer-motion';
import Observatory from './components/Observatory';
import OrbitalView from './components/OrbitalView';
import Starfield from './components/Starfield';
import './App.css';

function App() {
  const [view, setView] = useState('observatory');
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedArticle, setSelectedArticle] = useState(null);

  const handleSearch = (query) => {
    setSearchQuery(query);
    setSelectedArticle(null);
    setView('results');
  };

  const handleBack = () => {
    setView('observatory');
    setSelectedArticle(null);
  };

  const handleArticleClick = (article) => {
    setSelectedArticle(article);
  };

  const handleCloseArticle = () => {
    setSelectedArticle(null);
  };

  return (
    <div className="w-full h-full bg-black relative">
      <Starfield />
      <AnimatePresence mode="wait">
        {view === 'observatory' ? (
          <Observatory key="observatory" onSearch={handleSearch} />
        ) : (
          <OrbitalView 
            key="results" 
            query={searchQuery} 
            onBack={handleBack}
            selectedArticle={selectedArticle}
            onArticleClick={handleArticleClick}
            onCloseArticle={handleCloseArticle}
            onSearch={handleSearch}
          />
        )}
      </AnimatePresence>
    </div>
  );
}

export default App;
