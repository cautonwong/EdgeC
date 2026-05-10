package backend

import (
	"contract-compiler-v2/internal/ir"
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

type CMakeBackend struct {
	OutDir string
}

func (b *CMakeBackend) Emit(contract *ir.ContractIR) error {
	f, err := os.Create(filepath.Join(b.OutDir, "config.cmake"))
	if err != nil {
		return err
	}
	defer f.Close()

	fmt.Fprintf(f, "set(EDGE_PRODUCT_NAME \"%s\")\n", contract.ProductName)
	fmt.Fprintf(f, "set(EDGE_CONTRACT_SIGNATURE \"0x%08X\")\n", contract.Signature)

	for _, s := range contract.UsedStages {
		fmt.Fprintf(f, "set(EDGE_STAGE_%s_ENABLED ON)\n", strings.ToUpper(s))
	}

	return nil
}
