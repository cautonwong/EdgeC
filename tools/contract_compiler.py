#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
EdgeOS Contract Compiler Wrapper
---------------------------------
将调用重定向到新的模块化编译器 tools/contract_compiler/main.py
"""

import sys
import os

# 将当前 tools 目录加入模块搜索路径
sys.path.append(os.path.dirname(__file__))

from contract_compiler.main import main

if __name__ == "__main__":
    main()
