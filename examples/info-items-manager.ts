import { existsSync } from "fs";
import { copyFile, mkdir, readFile, writeFile } from "fs/promises";
import { basename, join } from "path";
import { v4 } from "uuid";
import { TMP_PATH } from "./settings";

const infoItemsPath = join(TMP_PATH, "info-items.json");
const serverPath = join(TMP_PATH, "fake-server");

export const initInfoItems = async () => {
  if (!existsSync(infoItemsPath)) {
    await writeFile(infoItemsPath, JSON.stringify({}));
  }

  if (!existsSync(serverPath)) {
    await mkdir(serverPath);
  }
};

export const getInfoItems = async () => {
  return JSON.parse(await readFile(infoItemsPath, "utf8"));
};

export const deleteInfoItems = async () => {
  await writeFile(infoItemsPath, JSON.stringify({}));
};

export const addInfoItem = async (itemPath: string) => {
  const fileName = basename(itemPath);
  const serverItemPath = join(serverPath, fileName);
  await copyFile(itemPath, serverItemPath);
  
  const id = v4();
  const infoItems = await getInfoItems();
  infoItems[id] = serverItemPath;
  
  await writeFile(infoItemsPath, JSON.stringify(infoItems, null, 2));
  return id;
};

export const getInfoItem = async (id: string) => {
  const infoItems = await getInfoItems();
  return infoItems[id];
};
