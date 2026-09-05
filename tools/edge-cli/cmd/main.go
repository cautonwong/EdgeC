package main

import (
	"flag"
	"fmt"
	"os"

	"edge-cli/internal/dds"
	"edge-cli/internal/linter"
	"edge-cli/internal/migrate"
	"edge-cli/internal/scaffold"
)

const Version = "1.0.0"

func printUsage() {
	fmt.Printf(`EdgeOS Developer Experience Tool (edge-cli) v%s
Industrial Scaffold, Driver Generation, Contract Linter & Migration Wizard

Usage:
  edge-cli <command> [arguments]

Commands:
  create-product <name>       Create an isolated EdgeOS product scaffold
  create-driver <name>        Create an EdgeOS peripheral driver & binding template
  lint-contract <file.yaml>   Lint & validate an EdgeOS contract YAML file
  migrate [--dir=<path>]      Scan and auto-migrate legacy SDK headers and contract schemas
  dds <subcommand>            DDS discovery, RTPS traffic telemetry, dump & Wireshark PCAP
  version                     Display edge-cli version
  help                        Show help message

Flags:
  --base <dir>                Base project root directory (default: ".")
  --dir <dir>                 Target directory to migrate (default: ".")
  --type <type>               Driver type (serial, tcp, can, custom; default: "serial")
  --protocol <protocol>       Driver protocol (modbus, dlt645, iec104, custom; default: "custom")
  --strict                    Treat linter warnings as errors
  --fix                       Automatically apply migration fixes to files

Examples:
  edge-cli create-product microgrid_gateway
  edge-cli create-driver inverter_sunspec --type=tcp --protocol=modbus
  edge-cli lint-contract contracts/products/iems_v5.yaml --strict
  edge-cli migrate --dir=examples/custom_driver --fix
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
			fmt.Fprintf(os.Stderr, "Error: missing driver name\nUsage: edge-cli create-driver <name> [--type=<type>] [--protocol=<protocol>]\n")
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

	case "migrate":
		fs := flag.NewFlagSet("migrate", flag.ExitOnError)
		dir := fs.String("dir", ".", "Target directory to scan and migrate")
		fix := fs.Bool("fix", false, "Automatically apply migration changes")
		verbose := fs.Bool("verbose", false, "Verbose output")

		var flagArgs []string
		for _, arg := range os.Args[2:] {
			if len(arg) > 0 && arg[0] == '-' {
				flagArgs = append(flagArgs, arg)
			}
		}
		_ = fs.Parse(flagArgs)

		res, err := migrate.RunMigration(migrate.MigrateOptions{
			TargetDir: *dir,
			Fix:       *fix,
			Verbose:   *verbose,
		})
		if err != nil {
			fmt.Fprintf(os.Stderr, "❌ Migration error: %v\n", err)
			os.Exit(1)
		}
		migrate.PrintReport(res, *fix)

	case "dds":
		if len(os.Args) < 3 {
			fmt.Printf(`Usage: edge-cli dds <subcommand> [flags]

Subcommands:
  discovery [--format=table|json]   Discover active DDS participants & partitions
  stats                             Show RTPS packet traffic, latency & loss telemetry
  dump --topic=<name> [--count=N]   Snoop real-time RTPS CDR packets on specified topic
  pcap --output=<file.pcap>         Generate Wireshark-compatible RTPS packet dump
`)
			os.Exit(1)
		}
		subCmd := os.Args[2]
		switch subCmd {
		case "discovery":
			fs := flag.NewFlagSet("dds discovery", flag.ExitOnError)
			format := fs.String("format", "table", "Output format (table or json)")
			_ = fs.Parse(os.Args[3:])
			out, err := dds.RunDiscovery(*format)
			if err != nil {
				fmt.Fprintf(os.Stderr, "❌ DDS Discovery error: %v\n", err)
				os.Exit(1)
			}
			fmt.Print(out)

		case "stats":
			out, err := dds.RunStats()
			if err != nil {
				fmt.Fprintf(os.Stderr, "❌ DDS Stats error: %v\n", err)
				os.Exit(1)
			}
			fmt.Print(out)

		case "dump":
			fs := flag.NewFlagSet("dds dump", flag.ExitOnError)
			topic := fs.String("topic", "edgeos/telemetry/yc", "Topic to snoop")
			count := fs.Int("count", 5, "Number of packets to dump")
			_ = fs.Parse(os.Args[3:])
			out, err := dds.RunDump(*topic, *count)
			if err != nil {
				fmt.Fprintf(os.Stderr, "❌ DDS Dump error: %v\n", err)
				os.Exit(1)
			}
			fmt.Print(out)

		case "pcap":
			fs := flag.NewFlagSet("dds pcap", flag.ExitOnError)
			outFile := fs.String("output", "rtps_trace.pcap", "Output PCAP filename")
			count := fs.Int("count", 10, "Number of packets to capture")
			_ = fs.Parse(os.Args[3:])
			err := dds.GeneratePCAP(*outFile, *count)
			if err != nil {
				fmt.Fprintf(os.Stderr, "❌ DDS PCAP capture error: %v\n", err)
				os.Exit(1)
			}
			fmt.Printf("✅ RTPS Wireshark PCAP trace captured successfully: %s (%d packets)\n", *outFile, *count)

		default:
			fmt.Fprintf(os.Stderr, "Unknown DDS subcommand: '%s'\n", subCmd)
			os.Exit(1)
		}

	default:
		fmt.Fprintf(os.Stderr, "Unknown command: '%s'\n", cmd)
		printUsage()
		os.Exit(1)
	}
}
