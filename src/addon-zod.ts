import { z } from "zod";

import { PinState, SyncState } from "@/types/placeholder.type";

export const addonZod = {
  addLoggerPath: z.boolean(),
  connectSyncRoot: z.object({ hr: z.literal(0), connectionKey: z.string() }),
  convertToPlaceholder: z.boolean(),
  dehydrateFile: z.boolean(),
  getFileIdentity: z.string(),
  getPlaceholderAttribute: z.object({ attribute: z.union([z.literal(0), z.literal(1), z.literal(2)]) }).transform(({ attribute }) => {
    if (attribute === 1) return "NOT_PINNED";
    if (attribute === 2) return "PINNED";
    return "OTHER";
  }),
  getPlaceholderState: z.object({ pinState: z.nativeEnum(PinState), syncState: z.nativeEnum(SyncState) }),
  getPlaceholderWithStatePending: z.array(z.string()),
  hydrateFile: z.undefined(),
  registerSyncRoot: z.literal(0),
  updateSyncStatus: z.boolean(),
  unregisterSyncRoot: z.number(),
};
