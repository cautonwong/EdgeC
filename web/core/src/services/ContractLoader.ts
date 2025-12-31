import axios from 'axios';
import yaml from 'js-yaml';
import { defineStore } from 'pinia';

export interface TagMeta {
  id: number;
  name: string;
  unit: string;
  type: string;
  desc: string;
  attributes?: {
    ui?: {
      group?: string;
      min?: number;
      max?: number;
      precision?: number;
      widget?: 'text' | 'gauge' | 'chart';
    };
    northbound?: {
      push?: 'change' | 'periodic';
    };
  };
}

export interface Contract {
  product: string;
  generated_at: string;
  tags: TagMeta[];
}

export const useContractStore = defineStore('contract', {
  state: () => ({
    contract: null as Contract | null,
    loading: false,
    error: null as string | null,
    tagMap: new Map<number, TagMeta>(),
    groups: new Map<string, TagMeta[]>() // UI Groups
  }),

  actions: {
    async loadContract() {
      this.loading = true;
      try {
        const response = await axios.get('/api/v1/contracts', { responseType: 'text' });
        // 解析 YAML
        const data = yaml.load(response.data) as Contract;
        this.contract = data;
        
        // 建立索引
        this.tagMap.clear();
        this.groups.clear();
        
        data.tags.forEach(tag => {
          this.tagMap.set(tag.id, tag);
          
          // UI 分组逻辑
          const groupName = tag.attributes?.ui?.group || 'Uncategorized';
          if (!this.groups.has(groupName)) {
            this.groups.set(groupName, []);
          }
          this.groups.get(groupName)?.push(tag);
        });
        
      } catch (err) {
        this.error = 'Failed to load contract: ' + err;
        console.error(err);
      } finally {
        this.loading = false;
      }
    },

    getTag(id: number) {
      return this.tagMap.get(id);
    },
    
    getGroup(name: string) {
      return this.groups.get(name) || [];
    }
  }
});
