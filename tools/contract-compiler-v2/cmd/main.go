package main

import (
	"contract-compiler-v2/internal/backend"
	"contract-compiler-v2/internal/frontend"
	"contract-compiler-v2/internal/optimizer"
	"contract-compiler-v2/internal/validator"
	"crypto"
	"crypto/rand"
	"crypto/rsa"
	"crypto/sha256"
	"crypto/x509"
	"encoding/pem"
	"flag"
	"fmt"
	"os"
	"path/filepath"
)

func main() {
	product := flag.String("product", "", "Path to product YAML")
	local := flag.String("local", "", "Path to local manifest YAML")
	out := flag.String("out", "", "Output directory")
	base := flag.String("base", ".", "Base directory for includes")
	outputFormat := flag.String("output-format", "all", "Output format: all, blob, c, go, ts, idl")
	signKey := flag.String("sign", "", "Path to RSA private key for signing .wasm output")
	strict := flag.Bool("strict", true, "Enable strict contract validation (default: true)")
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

	v := validator.NewValidator(*strict)
	if errs := v.Validate(ast); len(errs) > 0 {
		fmt.Printf("❌ Contract Validation Failed (%d error(s)):\n", len(errs))
		for _, e := range errs {
			fmt.Printf("   • %v\n", e)
		}
		os.Exit(1)
	}
	fmt.Printf("  ✅ Validator: Contract validation PASSED\n")

	opt := &optimizer.Optimizer{}
	ir, err := opt.Optimize(ast)
	if err != nil {
		fmt.Printf("❌ Optimizer Error: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("  ✅ Optimizer: IR generated (Tags: %d, Engines: %d)\n", 
		len(ir.Tags), len(ir.LookupEngines))

	// Emit C Code
	if *outputFormat == "all" || *outputFormat == "c" {
		if err := (&backend.CBackend{OutDir: *out}).Emit(ir); err != nil {
			fmt.Printf("❌ Backend Error (C): %v\n", err)
			os.Exit(1)
		}
	}

	// Emit Binary Blob (ADR-0002)
	if *outputFormat == "all" || *outputFormat == "blob" {
		if err := (&backend.BlobBackend{OutDir: *out}).Emit(ir); err != nil {
			fmt.Printf("❌ Backend Error (Blob): %v\n", err)
			os.Exit(1)
		}
	}

	// Emit Metadata (JSON/YAML)
	if *outputFormat == "all" || *outputFormat == "c" {
		if err := (&backend.MetadataBackend{OutDir: *out}).Emit(ir); err != nil {
			fmt.Printf("❌ Backend Error (Meta): %v\n", err)
			os.Exit(1)
		}
	}

	// Emit CMake Config
	if *outputFormat == "all" || *outputFormat == "c" {
		if err := (&backend.CMakeBackend{OutDir: *out}).Emit(ir); err != nil {
			fmt.Printf("❌ Backend Error (CMake): %v\n", err)
			os.Exit(1)
		}
	}

	// Emit TypeScript
	if *outputFormat == "all" || *outputFormat == "ts" {
		if err := (&backend.TsBackend{OutDir: *out}).Emit(ir); err != nil {
			fmt.Printf("❌ Backend Error (TS): %v\n", err)
			os.Exit(1)
		}
	}

	// Emit Go Bindings
	if *outputFormat == "all" || *outputFormat == "go" {
		if err := (&backend.GoBackend{OutDir: *out}).Emit(ir); err != nil {
			fmt.Printf("❌ Backend Error (Go): %v\n", err)
			os.Exit(1)
		}
	}

	// Emit OMG IDL 4.2 Contracts (Option A)
	if *outputFormat == "all" || *outputFormat == "idl" {
		if err := (&backend.IDLBackend{OutDir: *out}).Emit(ir); err != nil {
			fmt.Printf("❌ Backend Error (IDL): %v\n", err)
			os.Exit(1)
		}
	}

	// Emit MCP Tools Schema (AEP-506)
	if *outputFormat == "all" || *outputFormat == "c" || *outputFormat == "mcp" {
		if err := (&backend.MCPBackend{OutDir: *out}).Emit(ir); err != nil {
			fmt.Printf("❌ Backend Error (MCP): %v\n", err)
			os.Exit(1)
		}
	}

	// Sign .wasm output if requested
	if *signKey != "" {
		wasmPath := filepath.Join(*out, ast.ProductName+".wasm")
		if err := signWasmFile(wasmPath, *signKey); err != nil {
			fmt.Printf("  ⚠️  Signing skipped: %v\n", err)
		}
	}

	fmt.Printf("  ✨ Backend: All artifacts emitted to %s\n", *out)

}

// signWasmFile reads a .wasm file, computes SHA256, and signs with the given RSA private key.
// The signature is written to <wasmPath>.sig in PKCS1v15-SHA256 format.
func signWasmFile(wasmPath, keyPath string) error {
	wasmData, err := os.ReadFile(wasmPath)
	if err != nil {
		return fmt.Errorf("cannot read %s: %v", wasmPath, err)
	}

	hash := sha256.Sum256(wasmData)

	keyData, err := os.ReadFile(keyPath)
	if err != nil {
		return fmt.Errorf("cannot read key %s: %v", keyPath, err)
	}

	block, _ := pem.Decode(keyData)
	if block == nil {
		return fmt.Errorf("invalid PEM data in %s", keyPath)
	}

	priv, err := x509.ParsePKCS8PrivateKey(block.Bytes)
	if err != nil {
		priv, err = x509.ParsePKCS1PrivateKey(block.Bytes)
		if err != nil {
			return fmt.Errorf("failed to parse private key: %v", err)
		}
	}

	rsaKey, ok := priv.(*rsa.PrivateKey)
	if !ok {
		return fmt.Errorf("key is not RSA (got %T)", priv)
	}

	sig, err := rsa.SignPKCS1v15(rand.Reader, rsaKey, crypto.SHA256, hash[:])
	if err != nil {
		return fmt.Errorf("signing failed: %v", err)
	}

	sigPath := wasmPath + ".sig"
	if err := os.WriteFile(sigPath, sig, 0644); err != nil {
		return fmt.Errorf("cannot write %s: %v", sigPath, err)
	}

	fmt.Printf("  ✅ Signature written to %s\n", sigPath)
	return nil
}
