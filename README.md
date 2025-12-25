# ♟️ Chess AI Engine

A modular chess engine built with a handcrafted evaluation function and classical search (alpha–beta).  
Designed to be extensible, testable, and ready for future neural network integration.

This project focuses on:
- Correct chess rules and move generation
- Engine-first design


## 📦 Features

- Handcrafted evaluation function
- Alpha–Beta pruning with quiescence search
- UCI-compatible interface (GUI & tournament ready)
- Clean internal architecture (engine ≠ API ≠ UI)
- Designed for future NN / NNUE integration


## 🧰 Tech Stack

- **Language**: C++ (engine core)
- **Build tools**: CMake, Ninja, custom script (.sh)