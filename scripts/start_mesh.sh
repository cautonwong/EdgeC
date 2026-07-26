#!/bin/bash
export EDGE_SHM_NAME=edgeos_shadow_iems

pkill -9 -f "ui-backend|edge-api-core|demo-algo|iems|edge-gateway" || true
sleep 1

./build/products/iems/bin/iems > /tmp/iems.log 2>&1 &
sleep 2

./go/cmd/ui-backend/ui-backend > /tmp/ui_backend.log 2>&1 &
./go/cmd/edge-api-core/edge-api-core > /tmp/edge_api_core.log 2>&1 &
./go/demo-algo > /tmp/demo_algo.log 2>&1 &
sleep 1
./go/cmd/edge-gateway/edge-gateway > /tmp/edge_gateway.log 2>&1 &
sleep 2

echo "EdgeOS Microservices Mesh active on ports 80/8080/8081/8082/9091."
disown -a
sleep 86400
