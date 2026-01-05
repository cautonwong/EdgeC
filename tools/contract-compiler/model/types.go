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



type ConfigDef struct {

	Name  string      `yaml:"name"`

	Value interface{} `yaml:"value"`

	Desc  string      `yaml:"desc"`

}



type Binding struct {

	TagName      string                 `yaml:"tag"`

	ProtocolType string                 `yaml:"protocol"`

	Attributes   map[string]interface{} `yaml:"attributes"`

}



type GlobalSchema struct {

	Tags []TagDef `yaml:"tags"`

}



type ProductConfig struct {



	ProductName string                 `yaml:"product"`



	Includes    []string               `yaml:"includes"`



	EnabledTags []interface{}          `yaml:"enabled_tags"`



	Overrides   map[string]TagDef      `yaml:"overrides"`



	Configs     []ConfigDef            `yaml:"configs"`



}







type BindingFile struct {

	Protocol string                   `yaml:"protocol"`

	Mappings []map[string]interface{} `yaml:"mappings"`

	Map      []map[string]interface{} `yaml:"map"`

}



type EffectiveContract struct {

	Product    string      `yaml:"product_name"`

	ActiveTags []TagDef    `yaml:"active_tags"`

	AllTags    []TagDef    `yaml:"all_tags"`

	Configs    []ConfigDef `yaml:"configs"`

}
