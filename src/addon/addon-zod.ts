import { z } from "zod";

import { logger } from "@/logger";
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

export const parseAddonZod = <T>(fn: keyof typeof addonZod, data: T) => {
  const schema = addonZod[fn];
  const result = schema.safeParse(data);
  if (result.error) logger.error(result.error, fn);
  return data;
};
