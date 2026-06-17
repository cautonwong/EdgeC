package main

import (
	"contract-compiler-v2/internal/backend"
	"contract-compiler-v2/internal/frontend"
	"contract-compiler-v2/internal/optimizer"
	"flag"
	"fmt"
	"os"
)

func main() {
	product := flag.String("product", "", "Path to product YAML")
	local := flag.String("local", "", "Path to local manifest YAML")
	out := flag.String("out", "", "Output directory")
	base := flag.String("base", ".", "Base directory for includes")
	flag.Parse()

	if *product == "" || *out == "" {
		fmt.Println("Usage: contract-compiler-v2 -product <path> -out <dir> [-local <path>] [-base <dir>]")
		os.Exit(1)
	}

	fmt.Printf("🚀 EdgeOS Contract Compiler V2 (IR-Based)\n")

	loader := frontend.NewLoader(*base)
	ast, err := loader.Load(*product, *local)
	if err != nil {
		fmt.Printf("❌ Frontend Error: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("  ✅ Frontend: AST built for %s\n", ast.ProductName)

	opt := &optimizer.Optimizer{}
	ir, err := opt.Optimize(ast)
	if err != nil {
		fmt.Printf("❌ Optimizer Error: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("  ✅ Optimizer: IR generated (Tags: %d, Engines: %d)\n", 
		len(ir.Tags), len(ir.LookupEngines))

	// Emit C Code
	if err := (&backend.CBackend{OutDir: *out}).Emit(ir); err != nil {
		fmt.Printf("❌ Backend Error (C): %v\n", err)
		os.Exit(1)
	}
	
	// Emit Metadata (JSON/YAML)
	if err := (&backend.MetadataBackend{OutDir: *out}).Emit(ir); err != nil {
		fmt.Printf("❌ Backend Error (Meta): %v\n", err)
		os.Exit(1)
	}

	// Emit CMake Config
	if err := (&backend.CMakeBackend{OutDir: *out}).Emit(ir); err != nil {
		fmt.Printf("❌ Backend Error (CMake): %v\n", err)
		os.Exit(1)
	}

	// Emit TypeScript
	if err := (&backend.TsBackend{OutDir: *out}).Emit(ir); err != nil {
		fmt.Printf("❌ Backend Error (TS): %v\n", err)
		os.Exit(1)
	}

	// Emit Go Bindings
	if err := (&backend.GoBackend{OutDir: *out}).Emit(ir); err != nil {
		fmt.Printf("❌ Backend Error (Go): %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("  ✨ Backend: All artifacts emitted to %s\n", *out)

}
