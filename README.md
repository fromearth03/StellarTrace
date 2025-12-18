# StellarTrace 🌌

> **A Scalable, High-Performance Search Engine Architecture**

[![Language](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Build](https://img.shields.io/badge/Build-CMake-green.svg)]()

## 📖 Overview

**StellarTrace** is a custom-built, full-text search engine designed to index and retrieve information from large-scale datasets efficiently.

Inspired by the architecture described in the seminal paper *"The Anatomy of a Large-Scale Hypertextual Web Search Engine"* (Brin & Page), StellarTrace demonstrates a robust implementation of core information retrieval data structures without reliance on external database management systems.

The current implementation focuses on the **Backend Search Core**, capable of processing the **arXiv research corpus** to provide high-relevance search results for scientific queries.

---

## 🚀 Key Features

* **Custom Indexing Core:** Implements a Lexicon, Forward Index, and Inverted Index from scratch in C++.
* **High-Performance Retrieval:** Utilizes an optimized **In-Memory Inverted Index** to deliver sub-second query responses.
* **Robust Tokenization:** Features a custom text processing pipeline that handles UTF-8 characters, removes stop words, and normalizes scientific text.
* **Relevance Ranking:** Implements a custom ranking algorithm based on **TF-IDF** (Term Frequency-Inverse Document Frequency) and **Positional Weighting** (prioritizing hits in Titles vs. Abstracts).
* **Scalable Architecture:** Designed with a "Barrel" sharding system roadmap to support future expansion to multi-gigabyte datasets like Common Crawl.
* **Semantic Search:** Enhances keyword-based retrieval by leveraging vector-based similarity to capture contextual meaning beyond exact term matches.
* **Autocomplete Suggestions:** Provides real-time query completion using prefix-based lookup over the indexed lexicon to improve search usability.

---

## 🏗️ System Architecture

StellarTrace operates on a modular architecture designed for speed and scalability:

### 1. The Indexer (Core Logic)
The indexing pipeline transforms raw data into efficient search structures:
* **Lexicon:** An in-memory `std::unordered_map` mapping strings to unique Integer IDs. It handles collision resolution and stop-word filtering in O(1) time.
* **Forward Index:** Maps Document IDs to Word IDs, capturing the frequency and context (Title/Body) of each term.
* **Inverted Index:** The heart of the search engine. It maps Word IDs to lists of Document IDs, enabling fast retrieval.

### 2. The Search Server (API)
A lightweight C++ HTTP server (`cpp-httplib`) that exposes the search logic via a REST API.
* **Endpoint:** `GET /search?q=query`
* **Response:** Returns a JSON array of ranked document objects (Title, Abstract, Score, Metadata).

---

## 🛠️ Technology Stack

| Component | Technology | Purpose |
| :--- | :--- | :--- |
| **Core Language** | C++ 17 | High-performance indexing and search logic. |
| **Web Server** | `cpp-httplib` | Lightweight HTTP server for the API. |
| **Parsing** | `nlohmann/json` | Robust parsing of complex arXiv metadata JSON. |
| **Tokenization** | Custom / STL | Efficient string splitting, cleaning, and Unicode handling. |
| **Build System** | CMake / G++ | Standard C++ build chain. |

---

## ⚙️ Installation & Setup

### Prerequisites
* G++ Compiler (supporting C++17)
* CMake (optional, for building)

### 1. Clone the Repository
```bash
git clone [https://github.com/fromearth03/StellarTrace.git](https://github.com/fromearth03/StellarTrace.git)
cd StellarTrace
