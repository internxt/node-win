import { z } from "zod";

import { addonZod } from "@/addon/addon-zod";

export interface IVirtualDriveFunctions {
  CfHydrate?: () => void;
  CfDehydrate?: () => void;
  CfAddItem: () => void;
  CfNotifyMessage?: () => void;
  CfUpdateSyncStatus: (path: string, sync: boolean, isDirectory: boolean) => void;
  UpdatePinState?: () => void;
  CfUpdateItem?: () => void;
  CfGetPlaceHolderState: (path: string) => z.infer<typeof addonZod.getPlaceholderState>;
  CfGetPlaceHolderIdentity: (path: string) => z.infer<typeof addonZod.getFileIdentity>;
  CfGetPlaceHolderAttributes: (path: string) => z.infer<typeof addonZod.getPlaceholderAttribute>;
  CfConverToPlaceholder: (path: string, fileIdentity: string) => z.infer<typeof addonZod.convertToPlaceholder>;
}
