package backend

import (
	"contract-compiler-v2/internal/ir"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
)

type MCPBackend struct {
	OutDir string
}

type MCPToolSchema struct {
	Name        string                 `json:"name"`
	Description string                 `json:"description"`
	InputSchema map[string]interface{} `json:"inputSchema"`
}

type MCPManifest struct {
	ProductName string          `json:"product"`
	Version     string          `json:"version"`
	Tools       []MCPToolSchema `json:"tools"`
}

func (b *MCPBackend) Emit(irModel *ir.ContractIR) error {
	if err := os.MkdirAll(b.OutDir, 0755); err != nil {
		return fmt.Errorf("failed to create output directory: %w", err)
	}

	manifest := MCPManifest{
		ProductName: irModel.ProductName,
		Version:     "2.1.0",
		Tools: []MCPToolSchema{
			{
				Name:        "datahub_read_tags",
				Description: fmt.Sprintf("Read real-time physical telemetry tags from %s DataHub shared memory matrix", irModel.ProductName),
				InputSchema: map[string]interface{}{
					"type": "object",
					"properties": map[string]interface{}{
						"tag_ids": map[string]interface{}{
							"type":        "array",
							"items":       map[string]interface{}{"type": "integer"},
							"description": "Array of numerical Tag IDs to query",
						},
					},
					"required": []string{"tag_ids"},
				},
			},
			{
				Name:        "datahub_write_tag",
				Description: fmt.Sprintf("Write setpoint or telecontrol command into %s DataHub", irModel.ProductName),
				InputSchema: map[string]interface{}{
					"type": "object",
					"properties": map[string]interface{}{
						"tag_id":  map[string]interface{}{"type": "integer"},
						"value":   map[string]interface{}{"type": "number"},
						"quality": map[string]interface{}{"type": "integer", "default": 0},
					},
					"required": []string{"tag_id", "value"},
				},
			},
			{
				Name:        "datahub_get_tag_meta",
				Description: fmt.Sprintf("Get contract metadata for %s tags", irModel.ProductName),
				InputSchema: map[string]interface{}{
					"type": "object",
					"properties": map[string]interface{}{
						"tag_id": map[string]interface{}{"type": "integer"},
					},
					"required": []string{"tag_id"},
				},
			},
			{
				Name:        "datahub_scan_abnormal",
				Description: fmt.Sprintf("Scan %s DataHub for abnormal tags with non-zero quality", irModel.ProductName),
				InputSchema: map[string]interface{}{
					"type":       "object",
					"properties": map[string]interface{}{},
				},
			},
			{
				Name:        "io_list_channels",
				Description: fmt.Sprintf("List all active IO communication channels on %s", irModel.ProductName),
				InputSchema: map[string]interface{}{
					"type":       "object",
					"properties": map[string]interface{}{},
				},
			},
			{
				Name:        "io_get_channel_health",
				Description: "Get communication statistics and health grade for a channel",
				InputSchema: map[string]interface{}{
					"type": "object",
					"properties": map[string]interface{}{
						"channel_id": map[string]interface{}{"type": "integer"},
					},
					"required": []string{"channel_id"},
				},
			},
			{
				Name:        "io_trace_raw_packets",
				Description: "Inspect recent raw frame hex-dumps on communication channels",
				InputSchema: map[string]interface{}{
					"type": "object",
					"properties": map[string]interface{}{
						"limit": map[string]interface{}{"type": "integer", "default": 5},
					},
				},
			},
			{
				Name:        "pipeline_get_dag_status",
				Description: fmt.Sprintf("Inspect execution state across Stream Engine DAG nodes on %s", irModel.ProductName),
				InputSchema: map[string]interface{}{
					"type":       "object",
					"properties": map[string]interface{}{},
				},
			},
			{
				Name:        "pipeline_query_metrics",
				Description: "Get DSP pipeline processing latency distribution and throughput",
				InputSchema: map[string]interface{}{
					"type":       "object",
					"properties": map[string]interface{}{},
				},
			},
			{
				Name:        "dds_list_discovered_nodes",
				Description: "List distributed mesh nodes discovered via Cyclone DDS RTPS liveliness",
				InputSchema: map[string]interface{}{
					"type":       "object",
					"properties": map[string]interface{}{},
				},
			},
			{
				Name:        "dds_inspect_topics",
				Description: "Inspect published/subscribed DDS topics, types, and QoS profiles",
				InputSchema: map[string]interface{}{
					"type":       "object",
					"properties": map[string]interface{}{},
				},
			},
			{
				Name:        "storage_query_soe_events",
				Description: fmt.Sprintf("Query Sequence of Events (SOE) fault/alarm history on %s", irModel.ProductName),
				InputSchema: map[string]interface{}{
					"type": "object",
					"properties": map[string]interface{}{
						"limit": map[string]interface{}{"type": "integer", "default": 10},
					},
				},
			},
			{
				Name:        "storage_query_waveform",
				Description: "Query transient COMTRADE waveform samples for electrical fault diagnosis",
				InputSchema: map[string]interface{}{
					"type": "object",
					"properties": map[string]interface{}{
						"channel": map[string]interface{}{"type": "string", "default": "U_A"},
					},
				},
			},
		},
	}

	data, err := json.MarshalIndent(manifest, "", "  ")
	if err != nil {
		return fmt.Errorf("failed to marshal MCP tools schema: %w", err)
	}

	filePath := filepath.Join(b.OutDir, "mcp_tools_schema.json")
	if err := os.WriteFile(filePath, data, 0644); err != nil {
		return fmt.Errorf("failed to write mcp_tools_schema.json: %w", err)
	}

	fmt.Printf("  ✨ Backend: Emitted %d MCP tool schemas to %s\n", len(manifest.Tools), filePath)
	return nil
}
