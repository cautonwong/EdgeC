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
  };
}

export interface Contract {
  product_name: string;
  active_tags: TagMeta[];
  all_tags: TagMeta[];
  generated_at: string;
  ui_config: {
    branding: { title: string; theme: string; logo: string };
    navigation: Record<string, boolean>;
  };
}

export const useContractStore = defineStore('contract', {
  state: () => ({
    contract: null as any | null,
    loading: false,
    error: null as string | null,
    tagMap: new Map<number, TagMeta>(),
    groups: new Map<string, TagMeta[]>(),
    nav: {} as Record<string, boolean> 
  }),

  actions: {
    async loadContract() {
      this.loading = true;
      console.log('[ContractLoader] Syncing with EdgeOS backend...');
      try {
        const response = await axios.get('/api/v1/contracts', { responseType: 'text' });
        const rawData = yaml.load(response.data) as any;
        console.log('[ContractLoader] Received payload:', rawData);
        
        this.contract = rawData;
        this.nav = rawData.ui_config?.navigation || {};
        
        this.tagMap.clear();
        this.groups.clear();

        // 兼容性字段匹配：支持 active_tags 或 tags
        const tags = rawData.active_tags || rawData.tags || [];
        console.log('[ContractLoader] Found', tags.length, 'points to visualize');

        tags.forEach((tag: TagMeta) => {
          this.tagMap.set(tag.id, tag);
          const groupName = tag.attributes?.ui?.group || 'Metrology';
          if (!this.groups.has(groupName)) this.groups.set(groupName, []);
          this.groups.get(groupName)?.push(tag);
        });
      } catch (err) {
        console.error('[ContractLoader] FAILED:', err);
        this.error = 'Contract sync failed';
      } finally {
        this.loading = false;
      }
    }
  }
});