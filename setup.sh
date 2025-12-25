cd "$(dirname "$0")"
cmake -S . -B build -G Ninja
cmake --build ./build