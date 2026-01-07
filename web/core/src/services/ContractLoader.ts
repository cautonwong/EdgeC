import axios from 'axios';
import yaml from 'js-yaml';
import { defineStore } from 'pinia';
// @ts-ignore
import { TagID, TAG_METADATA, TagMetadata as GeneratedTagMeta } from '@generated/tags_type';

export interface TagMeta extends GeneratedTagMeta {
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
    contract: null as Contract | null,
    loading: false,
    error: null as string | null,
    tagMap: new Map<TagID, TagMeta>(),
    groups: new Map<string, TagMeta[]>(),
    nav: {} as Record<string, boolean> 
  }),

  actions: {
    /**
     * Get tag metadata with static fallback
     */
    getTag(id: TagID): TagMeta | undefined {
      return this.tagMap.get(id) || (TAG_METADATA[id] as TagMeta);
    },

    async loadContract() {
      this.loading = true;
      console.log('[ContractLoader] Syncing with EdgeOS backend using typed contracts...');
      try {
        const response = await axios.get('/api/v1/contracts', { responseType: 'text' });
        const rawData = yaml.load(response.data) as any;
        
        this.contract = rawData;
        this.nav = rawData.ui_config?.navigation || {};
        
        this.tagMap.clear();
        this.groups.clear();

        const tags = rawData.active_tags || rawData.tags || [];

        tags.forEach((tag: TagMeta) => {
          this.tagMap.set(tag.id as TagID, tag);
          const groupName = tag.attributes?.ui?.group || 'Metrology';
          if (!this.groups.has(groupName)) this.groups.set(groupName, []);
          this.groups.get(groupName)?.push(tag);
        });

        console.log(`[ContractLoader] Successfully loaded ${this.tagMap.size} tags.`);
      } catch (err) {
        console.error('[ContractLoader] FAILED:', err);
        this.error = 'Contract sync failed';
      } finally {
        this.loading = false;
      }
    }
  }
});