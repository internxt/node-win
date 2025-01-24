import { Status } from "src/types/placeholder.type";

export interface IVirtualDriveFunctions {
  CfHydrate?: () => void;
  CfDehydrate?: () => void;
  CfAddItem: () => void;
  CfNotifyMessage?: () => void;
  CfUpdateSyncStatus: (
    path: string,
    sync: boolean,
    isDirectory: boolean
  ) => void;
  UpdatePinState?: () => void;
  CfUpdateItem?: () => void;
  CfGetPlaceHolderState: (path: string) => Status;
  CfGetPlaceHolderIdentity: (path: string) => string;
  CfGetPlaceHolderAttributes: (path: string) => Promise<any>;
  CfConverToPlaceholder: (path: string, fileIdentity: string) => void;
}
