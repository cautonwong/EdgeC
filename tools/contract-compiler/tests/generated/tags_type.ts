/**
 * @file tags_type.ts
 * @brief Auto-generated TypeScript definitions for EdgeOS Tags
 * Product: TEST_COMPILER_PROD
 * Signature: 0xDE136081
 */

export enum TagID {
  TEST_VOLTAGE = 1001,
  TEST_CURRENT = 1002,
  TEST_SWITCH_STATUS = 5001,
}

/**
 * [Normalization] ParamID is a subset of TagID
 */
export enum ParamID {
}

export interface TagMetadata {
  id: number;
  name: string;
  type: string;
  unit?: string;
  desc?: string;
}

export const TAG_METADATA: Record<TagID, TagMetadata> = {
  [TagID.TEST_VOLTAGE]: {
    id: 1001,
    name: "TEST_VOLTAGE",
    type: "float",
    unit: "V",
    desc: "",
  },
  [TagID.TEST_CURRENT]: {
    id: 1002,
    name: "TEST_CURRENT",
    type: "float",
    unit: "A",
    desc: "",
  },
  [TagID.TEST_SWITCH_STATUS]: {
    id: 5001,
    name: "TEST_SWITCH_STATUS",
    type: "int",
    unit: "",
    desc: "",
  },
};

/**
 * Helper to get tag name by ID
 */
export function getTagName(id: TagID): string {
  return TagID[id] || "UNKNOWN";
}
