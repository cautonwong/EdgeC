package backend

import (
	"contract-compiler-v2/internal/ir"
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

// IDLBackend emits OMG IDL 4.2 contract definitions from ContractIR (Option A)
type IDLBackend struct {
	OutDir string
}

func (b *IDLBackend) Emit(contract *ir.ContractIR) error {
	f, err := os.Create(filepath.Join(b.OutDir, "edgeos_contract.idl"))
	if err != nil {
		return err
	}
	defer f.Close()

	fmt.Fprintln(f, "// AUTO-GENERATED OMG IDL 4.2 BY EDGEOS CONTRACT COMPILER V2")
	fmt.Fprintln(f, "// DO NOT EDIT MANUALLY - SOURCE OF TRUTH: contracts/schema/*.yaml")
	fmt.Fprintln(f, "")
	fmt.Fprintf(f, "module %s {\n", strings.ToLower(contract.ProductName))
	fmt.Fprintln(f, "    module contract {")

	// Emit Tag ID Constants
	fmt.Fprintln(f, "        // Tag ID Constants")
	for _, t := range contract.Tags {
		fmt.Fprintf(f, "        const unsigned long TAG_%s = %d;\n", strings.ToUpper(t.Name), t.ID)
	}
	fmt.Fprintln(f, "")

	// Emit Standard DataHub Tag Payload IDL Struct (OMG IDL 4.2)
	fmt.Fprintln(f, "        // Standard DataHub Topic Data Payload")
	fmt.Fprintln(f, "        struct TagData {")
	fmt.Fprintln(f, "            @key unsigned long tag_id;")
	fmt.Fprintln(f, "            unsigned long device_id;")
	fmt.Fprintln(f, "            double value;")
	fmt.Fprintln(f, "            unsigned long long mtt_timestamp;")
	fmt.Fprintln(f, "            unsigned long status;")
	fmt.Fprintln(f, "        };")
	fmt.Fprintln(f, "")

	// Emit Tag Metadata Entry Struct
	fmt.Fprintln(f, "        struct TagMetadata {")
	fmt.Fprintln(f, "            unsigned long id;")
	fmt.Fprintln(f, "            string name;")
	fmt.Fprintln(f, "            string data_type;")
	fmt.Fprintln(f, "            string unit;")
	fmt.Fprintln(f, "        };")

	fmt.Fprintln(f, "    };")
	fmt.Fprintln(f, "};")

	return nil
}
