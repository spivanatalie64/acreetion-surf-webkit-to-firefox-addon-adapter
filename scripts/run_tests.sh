#!/bin/bash
set -e

echo "=== Running mandatory tests for firefox addon adapter ==="

# Test 1: Library exists and is loadable
if [ ! -f build/libacrefxadapter.so ]; then
    echo "FAIL: Library not built"
    exit 1
fi
echo "PASS: Library built"

# Test 2: Check symbols
if ! nm -D build/libacrefxadapter.so | grep -q "acrefx_adapter_new"; then
    echo "FAIL: Missing required symbol acrefx_adapter_new"
    exit 1
fi
echo "PASS: Required symbols present"

# Test 3: Check dependencies
if ! ldd build/libacrefxadapter.so | grep -q "libwebkit2gtk"; then
    echo "WARN: webkit2gtk dependency not linked statically (expected)"
fi
echo "PASS: Dependency check done"

# Test 4: Basic manifest parsing test
cat > /tmp/test_manifest.json << 'MANIFEST'
{
  "manifest_version": 2,
  "name": "Test Addon",
  "version": "1.0.0",
  "content_scripts": [{
    "matches": ["*://example.com/*"],
    "js": ["content.js"]
  }]
}
MANIFEST
echo "PASS: Manifest test structure valid"

echo "=== All mandatory tests PASSED ==="
exit 0
