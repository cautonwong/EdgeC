package main

import (
	"flag"
	"fmt"
	"os"

	"edge-cli/internal/linter"
	"edge-cli/internal/scaffold"
)

const Version = "1.0.0"

func printUsage() {
	fmt.Printf(`EdgeOS Developer Experience Tool (edge-cli) v%s
Industrial Scaffold, Driver Generation & Contract Linter

Usage:
  edge-cli <command> [arguments]

Commands:
  create-product <name>       Create an isolated EdgeOS product scaffold
  create-driver <name>        Create an EdgeOS peripheral driver & binding template
  lint-contract <file.yaml>   Lint & validate an EdgeOS contract YAML file
  version                     Display edge-cli version
  help                        Show help message

Flags:
  --base <dir>                Base project root directory (default: ".")
  --type <type>               Driver type (serial, tcp, can, custom; default: "serial")
  --protocol <protocol>       Driver protocol (modbus, dlt645, iec104, custom; default: "custom")
  --strict                    Treat linter warnings as errors

Examples:
  edge-cli create-product microgrid_gateway
  edge-cli create-driver inverter_sunspec --type=tcp --protocol=modbus
  edge-cli lint-contract contracts/products/iems_v5.yaml --strict
`, Version)
}

func main() {
	if len(os.Args) < 2 {
		printUsage()
		os.Exit(1)
	}

	cmd := os.Args[1]

	switch cmd {
	case "version", "--version", "-v":
		fmt.Printf("edge-cli version %s (EdgeOS Clean Architecture Suite)\n", Version)
		os.Exit(0)

	case "help", "--help", "-h":
		printUsage()
		os.Exit(0)

	case "create-product":
		fs := flag.NewFlagSet("create-product", flag.ExitOnError)
		baseDir := fs.String("base", ".", "Base project root directory")
		
		// Parse all args (extracting flags and non-flags)
		var nonFlagArgs []string
		var flagArgs []string
		for _, arg := range os.Args[2:] {
			if len(arg) > 0 && arg[0] == '-' {
				flagArgs = append(flagArgs, arg)
			} else {
				nonFlagArgs = append(nonFlagArgs, arg)
			}
		}
		_ = fs.Parse(flagArgs)

		if len(nonFlagArgs) < 1 {
			fmt.Fprintf(os.Stderr, "Error: missing product name\nUsage: edge-cli create-product <name> [--base=<dir>]\n")
			os.Exit(1)
		}
		prodName := nonFlagArgs[0]
		err := scaffold.CreateProduct(scaffold.ProductOptions{
			Name:    prodName,
			BaseDir: *baseDir,
		})
		if err != nil {
			fmt.Fprintf(os.Stderr, "❌ Error creating product '%s': %v\n", prodName, err)
			os.Exit(1)
		}

	case "create-driver":
		fs := flag.NewFlagSet("create-driver", flag.ExitOnError)
		baseDir := fs.String("base", ".", "Base project root directory")
		drvType := fs.String("type", "serial", "Driver type (serial, tcp, can, custom)")
		protocol := fs.String("protocol", "custom", "Protocol (modbus, dlt645, iec104, custom)")
		
		var nonFlagArgs []string
		var flagArgs []string
		for _, arg := range os.Args[2:] {
			if len(arg) > 0 && arg[0] == '-' {
				flagArgs = append(flagArgs, arg)
			} else {
				nonFlagArgs = append(nonFlagArgs, arg)
			}
		}
		_ = fs.Parse(flagArgs)

		if len(nonFlagArgs) < 1 {
			fmt.Fprintf(os.Stderr, "Error: missing driver name\nUsage: edge-cli create-driver <name> [--type=...] [--protocol=...]\n")
			os.Exit(1)
		}
		drvName := nonFlagArgs[0]
		err := scaffold.CreateDriver(scaffold.DriverOptions{
			Name:     drvName,
			Type:     *drvType,
			Protocol: *protocol,
			BaseDir:  *baseDir,
		})
		if err != nil {
			fmt.Fprintf(os.Stderr, "❌ Error creating driver '%s': %v\n", drvName, err)
			os.Exit(1)
		}

	case "lint-contract":
		fs := flag.NewFlagSet("lint-contract", flag.ExitOnError)
		baseDir := fs.String("base", ".", "Base project root directory")
		strict := fs.Bool("strict", false, "Treat warnings as errors")

		var nonFlagArgs []string
		var flagArgs []string
		for _, arg := range os.Args[2:] {
			if len(arg) > 0 && arg[0] == '-' {
				flagArgs = append(flagArgs, arg)
			} else {
				nonFlagArgs = append(nonFlagArgs, arg)
			}
		}
		_ = fs.Parse(flagArgs)

		if len(nonFlagArgs) < 1 {
			fmt.Fprintf(os.Stderr, "Error: missing YAML file path\nUsage: edge-cli lint-contract <file.yaml> [--strict]\n")
			os.Exit(1)
		}
		yamlPath := nonFlagArgs[0]
		result, err := linter.LintContractFile(yamlPath, *baseDir)
		if err != nil {
			fmt.Fprintf(os.Stderr, "❌ Error reading '%s': %v\n", yamlPath, err)
			os.Exit(1)
		}

		fmt.Printf("🔍 Linting Contract: %s\n", yamlPath)
		for _, diag := range result.Diagnostics {
			fmt.Printf("  • %s\n", diag.String())
		}

		if result.ErrorCount > 0 || (*strict && result.WarnCount > 0) {
			fmt.Printf("❌ Lint FAILED: %d errors, %d warnings\n", result.ErrorCount, result.WarnCount)
			os.Exit(1)
		}
		fmt.Printf("✅ Lint PASSED: 0 errors, %d warnings\n", result.WarnCount)

	default:
		fmt.Fprintf(os.Stderr, "Unknown command: '%s'\n", cmd)
		printUsage()
		os.Exit(1)
	}
}
