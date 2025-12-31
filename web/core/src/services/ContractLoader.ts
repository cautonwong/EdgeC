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
  ui_config: {
    branding: {
      title: string;
      theme: string;
      logo: string;
    };
    navigation: Record<string, boolean>;
  };
  generated_at: string;
  tags: TagMeta[];
}

export const useContractStore = defineStore('contract', {
  state: () => ({
    contract: null as Contract | null,
    loading: false,
    error: null as string | null,
    tagMap: new Map<number, TagMeta>(),
    groups: new Map<string, TagMeta[]>(),
    nav: {} as Record<string, boolean> 
  }),

  actions: {
    async loadContract() {
      this.loading = true;
      try {
        // 从后端统一获取“数据宪法”
        const response = await axios.get('/api/v1/contracts', { responseType: 'text' });
        const data = yaml.load(response.data) as Contract;
        this.contract = data;
        
        // 提取 UI 描述符
        this.nav = data.ui_config?.navigation || {};
        
        // 建立测点索引与分组
        this.tagMap.clear();
        this.groups.clear();
        data.tags.forEach(tag => {
          this.tagMap.set(tag.id, tag);
          const groupName = tag.attributes?.ui?.group || 'Uncategorized';
          if (!this.groups.has(groupName)) this.groups.set(groupName, []);
          this.groups.get(groupName)?.push(tag);
        });
        
      } catch (err) {
        this.error = 'Failed to load contract: ' + err;
        console.error(err);
      } finally {
        this.loading = false;
      }
    },

    isPageVisible(pageId: string): boolean {
      // 默认显示，除非契约明确关闭
      return this.nav[pageId] !== false;
    }
  }
});