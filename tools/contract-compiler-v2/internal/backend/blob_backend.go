package backend

import (
	"contract-compiler-v2/internal/ir"
	"encoding/binary"
	"fmt"
	"os"
	"path/filepath"
)

// 💡 [ADR-0002] Binary Contract Blob Backend
// Outputs a .edgec file that can be loaded at runtime by contract_instance_load()
//
// Format (binary, little-endian):
//   [4]uint32: version
//   [64]byte:  name (null-terminated)
//   [64]byte:  schema_id (null-terminated)
//   [4]uint32: tag_count
//   [tag_count * sizeof(edge_tag_meta_t)]: tag_table
//   [4]uint32: pipeline_count
//   [pipeline_count * sizeof(edge_pipeline_def_t)]: pipeline_desc

type BlobBackend struct {
	OutDir string
}

func (b *BlobBackend) Emit(contract *ir.ContractIR) error {
	path := filepath.Join(b.OutDir, contract.ProductName+".edgec")
	f, err := os.Create(path)
	if err != nil {
		return fmt.Errorf("failed to create blob: %v", err)
	}
	defer f.Close()

	// Version
	if err := binary.Write(f, binary.LittleEndian, uint32(1)); err != nil {
		return err
	}

	// Name (64 bytes, null-padded)
	name := make([]byte, 64)
	copy(name, contract.ProductName)
	if _, err := f.Write(name); err != nil {
		return err
	}

	// Schema ID (64 bytes, null-padded)
	schemaID := make([]byte, 64)
	copy(schemaID, contract.SchemaHash)
	if _, err := f.Write(schemaID); err != nil {
		return err
	}

	// Tag count
	if err := binary.Write(f, binary.LittleEndian, uint32(len(contract.Tags))); err != nil {
		return err
	}

	// Tags
	for _, tag := range contract.Tags {
		if err := binary.Write(f, binary.LittleEndian, uint32(tag.ID)); err != nil {
			return err
		}
		// name (64 bytes)
		tagName := make([]byte, 64)
		copy(tagName, tag.Name)
		if _, err := f.Write(tagName); err != nil {
			return err
		}
		// type (64 bytes)
		tagType := make([]byte, 64)
		copy(tagType, tag.DataType)
		if _, err := f.Write(tagType); err != nil {
			return err
		}
		// unit (64 bytes)
		tagUnit := make([]byte, 64)
		copy(tagUnit, tag.Unit)
		if _, err := f.Write(tagUnit); err != nil {
			return err
		}
	}

	fmt.Printf("  ✅ BlobBackend: Wrote %d tags to %s\n", len(contract.Tags), path)
	return nil
}
