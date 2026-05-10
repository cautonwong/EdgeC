package ir

type ContractIR struct {
	ProductName string `json:"product"`
	Signature   uint32 `json:"signature"`

	// --- [NEW] Semantic Architecture ---
	Models    []ModelRecord    `json:"models"`
	Instances []InstanceRecord `json:"instances"`

	// --- [Legacy/Flattened] To be populated during compilation ---
	Tags []TagRecord `json:"tags"`

	LookupEngines map[string]*PerfectHashEngine `json:"-"`
	ReverseMaps   map[string][]ReverseMapEntry   `json:"-"`

	// Tag specific lookup (O(1))
	TagEngine *PerfectHashEngine `json:"-"`

	Router RouterInfo `json:"-"`

	Pipelines []PipelineRecord `json:"pipelines"`
	Policies  []PolicyRecord   `json:"-"`

	Connectivity []ConnectivityRecord `json:"connectivity"`
	Devices      []DeviceRecord       `json:"devices"`

	UsedStages []string `json:"-"`
}

// --- [Nexus] Model Definition (Blueprint) ---
type ModelRecord struct {
	Name     string                 `json:"name"`
	Protocol string                 `json:"protocol"`
	Version  string                 `json:"version"`
	Points   []ModelPointRecord      `json:"points"`
	Metadata map[string]interface{} `json:"metadata,omitempty"`
}

type ModelPointRecord struct {
	Name     string                 `json:"name"`
	Address  uint32                 `json:"address"` // Protocol specific relative address
	Type     string                 `json:"type"`
	Unit     string                 `json:"unit"`
	Category string                 `json:"category"` // TELEMETRY, STATUS, CONTROL, etc.
	Bits     string                 `json:"bits,omitempty"`
	Scale    float64                `json:"scale,omitempty"`
	Attrs    map[string]interface{} `json:"attrs,omitempty"`
}

// --- [Nexus] Instance Definition (Physical Deployment) ---
type InstanceRecord struct {
	Name      string                 `json:"name"`
	ModelName string                 `json:"model"`
	BusID     int                    `json:"bus_id"`
	Address   interface{}            `json:"address"` // SlaveID, IP, etc.
	Bindings  []InstanceTagBinding   `json:"bindings"` // Maps Model Points to Global TagIDs
	Metadata  map[string]interface{} `json:"metadata,omitempty"`
}

type InstanceTagBinding struct {
	PointName string `json:"point"`
	TagID     uint32 `json:"tag_id"`
}

// --- [Flattened IR] Used by C/Go Backends ---

type TagRecord struct {
	ID           uint32                 `json:"id"`
	Name         string                 `json:"name"`
	DataType     string                 `json:"type"`
	Unit         string                 `json:"unit"`
	Category     string                 `json:"category"`
	LogicalIndex uint32                 `json:"-"`
	Attributes   map[string]interface{} `json:"attributes"`
	Bindings     []BindingRecord        `json:"bindings"`
}

type BindingRecord struct {
	Protocol   string                 `json:"protocol"`
	Attributes map[string]interface{} `json:"attributes"`
}

type PerfectHashEngine struct {
	Protocol  string
	Seed      uint32
	TableSize uint32
	Slots     []HashSlot
}

type HashSlot struct {
	Key          uint64
	TagID        uint32
	LogicalIndex uint32
	Scale        float32
	ByteLen      uint16
	Access       uint32
}

type ReverseMapEntry struct {
	TagID        uint32
	LogicalIndex uint32
	Key          uint64
	Scale        float32
	ByteLen      uint16
	Access       uint32
}

type RouterInfo struct {
	TriggerMask []uint32
	WatcherMask []uint32
	MaxIndex    int
}

type PipelineRecord struct {
	ID         string        `json:"id"`
	TriggerTag uint32        `json:"trigger"`
	Stages     []StageRecord `json:"stages"`
}

type StageRecord struct {
	Name string `json:"name"`
	Type string `json:"type"`
	F1   float64
	F2   float64
	F3   float64
	U1   uint32
	S1   string
	NextIndices [4]int `json:"next_indices"`
}

type PolicyRecord struct {
	Name     string
	Type     string
	Interval uint32
	Tags     []uint32
}

type ConnectivityRecord struct {
	ID             int                    `json:"id"`
	Label          string                 `json:"label"`
	Category       string                 `json:"category"`
	Type           string                 `json:"type"`
	SimulationPath string                 `json:"-"`
	ProductionPath string                 `json:"-"`
	Simulation     map[string]interface{} `json:"simulation"`
	Config         map[string]interface{} `json:"config"`
}

type DeviceRecord struct {
	ID         string                 `json:"id"`
	BusID      int                    `json:"bus_id"`
	Protocol   string                 `json:"protocol"`
	Address    interface{}            `json:"address"`
	Model      string                 `json:"model"`
	Simulation map[string]interface{} `json:"simulation"`
}
