ninja -C build

echo ""
echo "Running..."
echo ""

exec $(dirname "$0")/build/chess-ai "$@"