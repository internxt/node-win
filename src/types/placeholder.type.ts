export enum SyncState {
  Undefined = -1,
  NotInSync = 0,
  InSync = 1,
}

export enum PinState {
  Inherited = 0,
  AlwaysLocal = 1,
  OnlineOnly = 2,
  Unspecified = 3,
  Excluded = 4,
}
