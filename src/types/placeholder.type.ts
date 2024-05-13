// Enum para SyncState
export enum SyncState {
  Undefined = -1,
  NotInSync = 0,
  InSync = 1,
}

// Enum para PinState
export enum PinState {
  Inherited = 0,
  AlwaysLocal = 1,
  OnlineOnly = 2,
  Unspecified = 3,
  Excluded = 4,
}

// Tipo para combinar ambos estados en un objeto de estado
export type Status = {
  pinState: PinState;
  syncState: SyncState;
};
