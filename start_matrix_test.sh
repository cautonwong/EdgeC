#!/bin/bash
# start_matrix_test.sh - 运行 IEMS 自动化集成测试

export MATRIX_PRODUCT=iems
export PYTHONPATH=$PYTHONPATH:$(pwd)

echo "🚀 Starting Matrix Integration Tests for $MATRIX_PRODUCT..."
.venv/bin/pytest cmd/iems/matrix/test_protection.py -s
