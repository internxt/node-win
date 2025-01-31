import { PinState, SyncState } from "./types/placeholder.type";

type TProps = {
  syncState: SyncState;
  pinState: PinState;
};

export const isFileInDevice = ({ syncState, pinState }: TProps) => {
  const inSync = syncState === SyncState.InSync;
  const isHydrated = pinState === PinState.AlwaysLocal || pinState === PinState.Unspecified;
  return inSync && isHydrated;
};
