package model

type TagDef struct {
	ID         uint32                 `json:"id" yaml:"id"`
	Name       string                 `json:"name" yaml:"name"`
	Type       string                 `json:"type" yaml:"type"`
	Unit       string                 `json:"unit" yaml:"unit"`
	Desc       string                 `json:"desc" yaml:"desc"`
	Traits     []string               `json:"traits" yaml:"traits"`
	Attributes map[string]interface{} `json:"attributes" yaml:"attributes"`
	Bindings   []Binding              `json:"bindings" yaml:"bindings"`
}

type Binding struct {
	TagName      string                 `json:"tag" yaml:"tag"`
	ProtocolType string                 `json:"protocol" yaml:"protocol"`
	Attributes   map[string]interface{} `json:"attributes" yaml:"attributes"`
}

type GlobalSchema struct {
	Tags []TagDef `yaml:"tags"`
}

type ConfigDef struct {
	Name  string      `json:"name" yaml:"name"`
	Value interface{} `json:"value" yaml:"value"`
}

type ParamDef struct {
	Name             string    `json:"name" yaml:"name"`
	Type             string    `json:"type" yaml:"type"`
	Default          interface{} `json:"default" yaml:"default"`
	Value            interface{} `json:"value" yaml:"value"`
	Unit             string    `json:"unit" yaml:"unit"`
	Min              float64   `json:"min" yaml:"min"`
	Max              float64   `json:"max" yaml:"max"`
	Desc             string    `json:"desc" yaml:"desc"`
	IsRebootRequired bool      `json:"is_reboot_required" yaml:"is_reboot_required"`
	ID               uint32    `json:"id" yaml:"id"`
	Bindings         []Binding `json:"bindings" yaml:"bindings"`
}

// ConnectivityDef 定义逻辑总线资源
type ConnectivityDef struct {
	ID         int                    `json:"id" yaml:"id"`
	Label      string                 `json:"label" yaml:"label"`
	Category   string                 `json:"category" yaml:"category"`
	Type       string                 `json:"type" yaml:"type"`
	Config     map[string]interface{} `json:"config" yaml:"config"`
	Simulation map[string]interface{} `json:"simulation" yaml:"simulation"`
}

// DeviceAttachment 定义业务资产挂载
type DeviceAttachment struct {
	ID         string                 `json:"id" yaml:"id"`
	BusID      int                    `json:"bus_id" yaml:"bus_id"`
	Protocol   string                 `json:"protocol" yaml:"protocol"`
	Address    interface{}            `json:"address" yaml:"address"`
	Model      string                 `json:"model" yaml:"model"`
	Config     map[string]interface{} `json:"config" yaml:"config"`
	Simulation map[string]interface{} `json:"simulation" yaml:"simulation"`
}

// [Legacy] 为了向后兼容保留原有定义
type HardwareDef struct {
	Type       string                 `json:"type" yaml:"type"`
	ID         int                    `json:"id" yaml:"id"`
	Label      string                 `json:"label" yaml:"label"`
	Profiles   map[string]HardwareProfile `json:"profiles" yaml:"profiles"`
	Topology   []DeviceInstance       `json:"topology" yaml:"topology"`
	Attributes map[string]interface{} `json:"attributes,omitempty" yaml:",inline"`
}

type HardwareProfile struct {
	OSDev  string `json:"os_dev" yaml:"os_dev"`
	Params string `json:"params" yaml:"params"`
}

type DeviceInstance struct {
	DeviceID   string                 `json:"dev_id" yaml:"dev_id"`
	Address    uint32                 `json:"address" yaml:"address"`
	Protocol   string                 `json:"protocol" yaml:"protocol"`
	Simulation map[string]interface{} `json:"simulation" yaml:"simulation"`
}

type PipelineDef struct {
	ID       string                   `json:"id" yaml:"id"`
	Trigger  string                   `json:"trigger" yaml:"trigger"`
	Triggers []map[string]interface{} `json:"triggers" yaml:"triggers"`
	Stages   []map[string]interface{} `json:"stages" yaml:"stages"`
}

type PolicyDef struct {
	Name       string                 `json:"name" yaml:"name"`
	Type       string                 `json:"type" yaml:"type"`
	Attributes map[string]interface{} `yaml:",inline"`
}

type UIDef struct {
	Theme string `json:"theme" yaml:"theme"`
}

type DefaultRule struct {
	Match      string                 `yaml:"match"`
	Attributes map[string]interface{} `yaml:"attributes"`
}

type ProductConfig struct {
	ProductName  string             `yaml:"product"`
	Protocol     string             `yaml:"protocol"`
	Includes     []string           `yaml:"includes"`
	Include      []string           `yaml:"include"`
	Defaults     []DefaultRule      `yaml:"defaults"`
	Tags         []interface{}      `yaml:"tags"` // For tag overrides
	EnabledTags  []interface{}      `yaml:"enabled_tags"`
	DisabledTags []string           `yaml:"disabled_tags"`
	Configs      []ConfigDef        `yaml:"configs"`
	Params       []ParamDef         `yaml:"parameters"`
	Connectivity []ConnectivityDef  `yaml:"connectivity"`
	Devices      []DeviceAttachment `yaml:"devices"`
	Hardware     []HardwareDef      `yaml:"hardware"`
	Pipelines    []PipelineDef      `yaml:"pipelines"`
	Policies     []PolicyDef        `yaml:"policies"`
	UI           UIDef              `yaml:"ui"`
	Mappings     []interface{}      `yaml:"mappings"`
}

type EffectiveContract struct {
	Product      string             `json:"product_name" yaml:"product_name"`
	ActiveTags   []TagDef           `json:"active_tags" yaml:"active_tags"`
	AllTags      []TagDef           `json:"all_tags" yaml:"all_tags"`
	Configs      []ConfigDef        `json:"configs" yaml:"configs"`
	Params       []ParamDef         `json:"parameters" yaml:"parameters"`
	Connectivity []ConnectivityDef  `json:"connectivity" yaml:"connectivity"`
	Devices      []DeviceAttachment `json:"devices" yaml:"devices"`
	Hardware     []HardwareDef      `json:"hardware" yaml:"hardware"`
	Pipelines    []PipelineDef      `json:"pipelines" yaml:"pipelines"`
	Policies     []PolicyDef        `json:"policies" yaml:"policies"`
	UI           UIDef              `json:"ui" yaml:"ui"`
	Scenarios    []string           `json:"scenarios" yaml:"scenarios"`
}
