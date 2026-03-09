#!/bin/bash

echo "🔨 Cleaning old build..."
rm -rf build

echo "📁 Creating build directory..."
mkdir build
cd build

echo "⚙️  Running cmake..."
cmake ..

echo "🔧 Building..."
make

echo "✅ Done! Running indexer..."
cd ..
./build/indexer "$1"