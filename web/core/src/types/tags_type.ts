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

export const TAG_METADATA: Record<string | number, TagMetadata> = {};
