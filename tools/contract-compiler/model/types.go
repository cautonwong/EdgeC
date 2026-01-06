package model

type TagDef struct {
	ID         uint32                 `yaml:"id"`
	Name       string                 `yaml:"name"`
	Type       string                 `yaml:"type"`
	Unit       string                 `yaml:"unit"`
	Desc       string                 `yaml:"desc"`
	Attributes map[string]interface{} `yaml:"attributes"`
	Bindings   []Binding              `yaml:"bindings"`
}

type Binding struct {
	TagName      string                 `yaml:"tag"`
	ProtocolType string                 `yaml:"protocol"`
	Attributes   map[string]interface{} `yaml:"attributes"`
}

type GlobalSchema struct {
	Tags []TagDef `yaml:"tags"`
}

type ConfigDef struct {
	Name  string      `yaml:"name"`
	Value interface{} `yaml:"value"`
}

type ParamDef struct {
	Name    string      `yaml:"name"`
	Type    string      `yaml:"type"`
	Default interface{} `yaml:"default"`
	Value   interface{} `yaml:"value"` // [新增] 用于覆盖 default
	Unit    string      `yaml:"unit"`
	Min     float64     `yaml:"min"`
	Max     float64     `yaml:"max"`
	Desc    string      `yaml:"desc"`
}

type HardwareDef struct {
	Type       string                 `yaml:"type"`
	ID         int                    `yaml:"id"`
	Label      string                 `yaml:"label"`
	Attributes map[string]interface{} `yaml:",inline"`
}

type PipelineDef struct {

	ID      string                 `yaml:"id"`

	Trigger string                 `yaml:"trigger"`

	Stages  []map[string]interface{} `yaml:"stages"`

}



type PolicyDef struct {

	Name       string                 `yaml:"name"`

	Type       string                 `yaml:"type"`

	Attributes map[string]interface{} `yaml:",inline"`

}



type UIDef struct {

	Layout     string                 `yaml:"layout"`

	Dashboards []map[string]interface{} `yaml:"dashboards"`

}



type ProductConfig struct {

	ProductName string                 `yaml:"product"`

	Includes    []string               `yaml:"includes"`

	EnabledTags []interface{}          `yaml:"enabled_tags"`

	Configs     []ConfigDef            `yaml:"configs"`

	Params      []ParamDef             `yaml:"parameters"`

	Hardware    []HardwareDef          `yaml:"hardware"`

	Pipelines   []PipelineDef          `yaml:"pipelines"`

	Policies    []PolicyDef            `yaml:"policies"`

	UI          UIDef                  `yaml:"ui"`

	Overrides   map[string]TagDef      `yaml:"overrides"`

}



type EffectiveContract struct {

	Product    string        `yaml:"product_name"`

	ActiveTags []TagDef      `yaml:"active_tags"`

	AllTags    []TagDef    `yaml:"all_tags"`

	Configs    []ConfigDef   `yaml:"configs"`

	Params     []ParamDef    `yaml:"parameters"`

	Hardware   []HardwareDef `yaml:"hardware"`

	Pipelines  []PipelineDef `yaml:"pipelines"`

	Policies   []PolicyDef   `yaml:"policies"`

	UI         UIDef         `yaml:"ui"`

}


