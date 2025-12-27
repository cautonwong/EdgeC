#!/bin/bash
cd /workspaces/edgeos/EdgeC/build
make -j4
timeout 10s ./edge_app || true
