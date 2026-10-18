/**
 * @file tags_type.ts
 * @brief Common Web UI Tag Metadata Types (No-Contract Autonomous Architecture)
 */

export type TagID = number | string;

export interface TagMetadata {
  id: TagID;
  name: string;
  type?: string;
  unit?: string;
  category?: string;
  min?: number;
  max?: number;
  attributes?: Record<string, any>;
}

export const TAG_METADATA: Record<string | number, TagMetadata> = {
  1001: {
    id: 1001,
    name: 'GRID_VOLTAGE_A',
    type: 'float',
    unit: 'V',
    category: 'Grid Metrology',
    min: 180,
    max: 260,
    attributes: {
      ui: {
        group: 'Grid Metrology',
        label: 'Grid Voltage Phase A',
        widget: 'gauge',
        icon: '⚡',
        min: 180,
        max: 260,
        precision: 1
      }
    }
  },
  1002: {
    id: 1002,
    name: 'GRID_CURRENT_A',
    type: 'float',
    unit: 'A',
    category: 'Grid Metrology',
    min: 0,
    max: 100,
    attributes: {
      ui: {
        group: 'Grid Metrology',
        label: 'Grid Current Phase A',
        widget: 'text',
        icon: '🔌',
        min: 0,
        max: 100,
        precision: 2
      }
    }
  },
  1003: {
    id: 1003,
    name: 'GRID_POWER',
    type: 'float',
    unit: 'kW',
    category: 'Grid Metrology',
    min: -50,
    max: 50,
    attributes: {
      ui: {
        group: 'Grid Metrology',
        label: 'Active Grid Power',
        widget: 'chart',
        icon: '📊',
        min: -50,
        max: 50,
        precision: 2
      }
    }
  },
  2001: {
    id: 2001,
    name: 'BATTERY_SOC',
    type: 'float',
    unit: '%',
    category: 'Storage BMS',
    min: 0,
    max: 100,
    attributes: {
      ui: {
        group: 'Storage BMS',
        label: 'Battery State of Charge',
        widget: 'gauge',
        icon: '🔋',
        min: 0,
        max: 100,
        precision: 1
      }
    }
  },
  2002: {
    id: 2002,
    name: 'BATTERY_VOLTAGE',
    type: 'float',
    unit: 'V',
    category: 'Storage BMS',
    min: 600,
    max: 900,
    attributes: {
      ui: {
        group: 'Storage BMS',
        label: 'Battery Pack Voltage',
        widget: 'text',
        icon: '⚡',
        min: 600,
        max: 900,
        precision: 1
      }
    }
  }
};

