import axios from 'axios';
import yaml from 'js-yaml';
import { defineStore } from 'pinia';
// @ts-ignore
import { TagID, TAG_METADATA, TagMetadata as GeneratedTagMeta } from '@generated/tags_type';

export interface TagUIMeta {
  group?: string;
  label?: string;
  unit?: string;
  precision?: number;
  widget?: 'text' | 'gauge' | 'chart' | 'sld_node';
  flash_color?: string;
  chart_type?: string;
  icon?: string;
  min?: number;
  max?: number;
}

export interface TagMeta extends GeneratedTagMeta {
  attributes?: {
    ui?: TagUIMeta;
    category?: string;
    [key: string]: any;
  };
}

export interface Contract {
  product_name: string;
  active_tags: TagMeta[];
  all_tags: TagMeta[];
  generated_at: string;
  ui_config: {
    branding: { title: string; theme: string; logo: string; subtitle?: string };
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
    activeGroup: 'all' as string,
    searchQuery: '' as string,
    nav: {} as Record<string, boolean>
  }),

  getters: {
    groupList: (state): string[] => Array.from(state.groups.keys()),
    
    totalMetrics: (state): number => state.tagMap.size,

    branding: (state) => {
      return state.contract?.ui_config?.branding || {
        title: state.contract?.product_name || 'EdgeOS System',
        subtitle: 'Advanced Industrial Edge Terminal',
        theme: 'dark',
        logo: '/logo.svg'
      };
    },

    filteredTags: (state): TagMeta[] => {
      let tags: TagMeta[] = [];
      if (state.activeGroup === 'all') {
        state.groups.forEach((groupTags) => {
          tags.push(...groupTags);
        });
      } else {
        tags = state.groups.get(state.activeGroup) || [];
      }

      if (state.searchQuery.trim()) {
        const query = state.searchQuery.toLowerCase().trim();
        tags = tags.filter((t) => 
          t.name.toLowerCase().includes(query) ||
          (t.attributes?.ui?.label && t.attributes.ui.label.toLowerCase().includes(query)) ||
          (t.attributes?.ui?.group && t.attributes.ui.group.toLowerCase().includes(query))
        );
      }
      return tags;
    }
  },

  actions: {
    /**
     * Get tag metadata with static fallback
     */
    getTag(id: TagID): TagMeta | undefined {
      return this.tagMap.get(id) || (TAG_METADATA[id] as TagMeta);
    },

    getTagsByGroup(group: string): TagMeta[] {
      return this.groups.get(group) || [];
    },

    setActiveGroup(group: string) {
      this.activeGroup = group;
    },

    setSearchQuery(q: string) {
      this.searchQuery = q;
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

        const tags: TagMeta[] = rawData.active_tags || rawData.tags || [];

        tags.forEach((tag: TagMeta) => {
          this.tagMap.set(tag.id as TagID, tag);
          const rawGroup = tag.attributes?.ui?.group || tag.attributes?.category || 'Metrology';
          const groupName = this.normalizeGroupName(rawGroup);
          if (!this.groups.has(groupName)) this.groups.set(groupName, []);
          this.groups.get(groupName)?.push(tag);
        });

        console.log(`[ContractLoader] Successfully loaded ${this.tagMap.size} tags in ${this.groups.size} groups.`);
      } catch (err) {
        console.warn('[ContractLoader] Remote sync failed, falling back to static TAG_METADATA...');
        this.loadStaticFallback();
      } finally {
        this.loading = false;
      }
    },

    loadStaticFallback() {
      this.tagMap.clear();
      this.groups.clear();

      if (TAG_METADATA) {
        Object.entries(TAG_METADATA).forEach(([_, tagObj]) => {
          const tag = tagObj as TagMeta;
          this.tagMap.set(tag.id as TagID, tag);
          const rawGroup = tag.attributes?.ui?.group || tag.attributes?.category || 'Metrology';
          const groupName = this.normalizeGroupName(rawGroup);
          if (!this.groups.has(groupName)) this.groups.set(groupName, []);
          this.groups.get(groupName)?.push(tag);
        });
      }
    },

    normalizeGroupName(raw: string): string {
      // Normalize 'Grid_Params' or 'grid_power' -> 'Grid Params'
      return raw
        .replace(/_/g, ' ')
        .replace(/\b\w/g, (c) => c.toUpperCase());
    }
  }
});