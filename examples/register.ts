import VirtualDrive from "../src/virtual-drive";
import * as config from "./config.json";

const drive = new VirtualDrive(
  config.syncRootPath,
  "C:\\Users\\Jonat\\AppData\\Roaming\\internxt-drive\\logs\\node-win.txt"
);

function generateRandomNumber(min: number, max: number) {
  return Math.floor(Math.random() * (max - min + 1)) + min;
}

async function onDeleteCallback(
  fileId: string,
  callback: (response: boolean) => void
) {
  console.log("On delete File ID: " + fileId);
  const a = await new Promise<boolean>((resolve, reject) => {
    try {
      setTimeout(() => {
        resolve(true);
      }, 10);
    } catch (err) {
      reject(err);
    }
  });

  return a;
}

function onDeleteCallbackWithCallback(
  fileId: string,
  callback: (response: boolean) => void
) {
  onDeleteCallback(fileId, callback)
    .then((response) => {
      callback(response);
    })
    .catch((err) => {
      callback(false);
    });
}

async function onRenameCallback(
  newName: string,
  fileId: string
): Promise<boolean> {
  console.log("File ID: " + fileId);
  console.log("New name: " + newName);

  const a = await new Promise<boolean>((resolve, reject) => {
    try {
      setTimeout(() => {
        resolve(true);
      }, 1000);
    } catch (err) {
      reject(err);
    }
  });

  return a;
}

function onRenameCallbackWithCallback(
  newName: string,
  fileId: string,
  responseCallback: (response: boolean) => void
) {
  onRenameCallback(newName, fileId)
    .then((response) => {
      responseCallback(response);
    })
    .catch((err) => {
      responseCallback(false);
    });
}

async function onFetchData(fileId: string): Promise<boolean> {
  console.log("downloading file: " + fileId);
  // simulating a download from a real server
  const a = await new Promise<boolean>((resolve, reject) => {
    try {
      setTimeout(() => {
        resolve(true);
      }, 1000);
    } catch (err) {
      reject(err);
    }
  });

  return a;
}

async function onFileAddedCallback(
  filePath: string,
  callback: (aknowledge: boolean, id: string) => void
) {
  try {
    let randomNumber = generateRandomNumber(10000, 60000);
    console.log(
      "========================= File added in callback: " + filePath
    );
    const newFilePath = filePath
      .replace(config.syncRootPath, "")
      .replace(/\\/g, "/"); //IMPORTANTE CAMBIAR LOS SLASHES
    await new Promise((resolve) =>
      setTimeout(() => {
        resolve(undefined);
      }, 1000)
    );

    // primer argumento es el boolean que indica si se pudo crear el archivo o no en el cloud
    // segundo argumento es el id del archivo creado en el cloud
    const result = Math.random().toString(36).substring(2, 7);
    console.log("=============================== with id" + result);
    callback(true, result);
  } catch (error) {
    callback(false, "");
    console.error(error);
  }
}

type CallbackResponse = (
  data: boolean,
  path: string,
  errorHandler?: () => void
) => Promise<{ finished: boolean; progress: number }>;
async function onFetchDataCallback(fileId: string, callback: CallbackResponse) {
  console.log("file id: " + fileId);
  // simulate a download from a real server and response with the path of the downloaded file of a fake server
  let finish = false;
  onFetchData(fileId)
    .then(async (response) => {
      while (!finish) {
        const callbackResponse = await callback(
          response,
          "C:\\Users\\Jonat\\Desktop\\fakeserver\\imagen.rar"
        );
        finish = callbackResponse.finished;
        if (finish) {
          console.log("finished");
          break;
        }
      }
    })
    .catch((err) => {
      // THIS CATCH IS REALLY IMPORTANT
      //callback(false, "C:\\Users\\gcarl\\Desktop\\fakeserver\\imagen.rar");
      console.log(err);
    });
}

async function onCancelFetchDataCallback(fileId: string) {
  console.log("cancel fetch data: ", fileId);
}

async function onMessageCallback(
  message: string,
  action: string,
  errorName: string,
  callback: (response: boolean) => void
) {
  try {
    console.log("[Example] Message received: ", message);
    console.log("[Example] Action: ", action);
    console.log("[Example] Error name: ", errorName);
    await callback(true);
  } catch (error) {
    callback(false);
  }
}

const iconPath = "C:\\Users\\User\\Downloads\\sicon.ico";
drive.registerSyncRoot(
  config.driveName,
  config.driveVersion,
  "{12345678-1234-1234-1234-123456789012}",
  {
    notifyDeleteCallback: onDeleteCallbackWithCallback,
    notifyRenameCallback: onRenameCallbackWithCallback,
    notifyFileAddedCallback: onFileAddedCallback,
    fetchDataCallback: onFetchDataCallback,
    cancelFetchDataCallback: onCancelFetchDataCallback,
    notifyMessageCallback: onMessageCallback,
  },
  iconPath
);

drive.connectSyncRoot();

/**  EXAMPLES OF HOW TO CREATE FILES AND FOLDERS
 * arguments:
 * 1. path of the file or folder
 * 2. id of the file or folder
 * 3. size of the file or folder
 * 4. creation date of the file or folder
 * 5. update date of the file or folder
 *
 * keep in mind that the file size must be the same as the original file
 */
const fileCreatedAt = Date.now() - 172800000; // two days ago
const fileUpdatedAt = Date.now() - 86400000; // yesterday
const folderCreatedAt = Date.now() - 259200000; // three days ago
const folderUpdatedAt = Date.now() - 345600000; // four days ago

// creating files
drive.createFileByPath(
  `/A (5th copy).pdfs`,
  "280ab650-acef-4438-8bbc-29863810b24a",
  1000,
  fileCreatedAt,
  fileUpdatedAt
);
drive.createFileByPath(
  `/file1.txt`,
  "fa8217c9-2dd6-4641-9180-8206e60368a6",
  1000,
  fileCreatedAt,
  fileUpdatedAt
);
drive.createFileByPath(
  `/folderWithFile/file2.txt`,
  "fa8217c9-2dd6-4641-9180-8206e6036216",
  1000,
  fileCreatedAt,
  fileUpdatedAt
);
drive.createFileByPath(
  `/fakefile.txt`,
  "fa8217c9-2dd6-4641-9180-8206e6036843",
  57,
  fileCreatedAt,
  fileUpdatedAt
);
drive.createFileByPath(
  `/imagen.rar`,
  "fa8217c9-2dd6-4641-9180-8206e60368f1",
  80582195,
  fileCreatedAt,
  fileUpdatedAt
);
drive.createFileByPath(
  `/noExtensionFile`,
  "fa8217c9-2dd6-4641-9180-8206e5039843",
  33020,
  fileCreatedAt,
  fileUpdatedAt
);

// creating folders
drive.createFolderByPath(
  `/only-folder`,
  "fa8217c9-2dd6-4641-9180-8206e60368123",
  1000,
  folderCreatedAt,
  folderUpdatedAt
);
drive.createFolderByPath(
  `/F.O.L.D.E.R`,
  "fa8217c9-2dd6-4641-9180-8206e80000123",
  1000,
  folderCreatedAt,
  folderUpdatedAt
);
drive.createFolderByPath(
  `/folderWithFolder/folder2`,
  "fa8217c9-2dd6-4641-9180-8206e6036845",
  1000,
  folderCreatedAt,
  folderUpdatedAt
);
drive.createFolderByPath(
  `/folderWithFolder/F.O.L.D.E.R`,
  "fa8217c9-2dd6-4641-9180-8206e60400123",
  1000,
  folderCreatedAt,
  folderUpdatedAt
);

// create items
drive.createItemByPath(
  `/item-folder/`,
  "fa8217c9-2dd6-4641-9189-8206e60368123",
  1000,
  folderCreatedAt,
  folderUpdatedAt
);
drive.createItemByPath(
  `/imagen-item.rar`,
  "fa8217c9-2dd6-4641-9180-053fe60368f1",
  33020,
  fileCreatedAt,
  fileUpdatedAt
);

const fileStatus = drive.getPlaceholderState("\\example.txt");

console.log("fileStatus:" + fileStatus);

// get items --------------
console.log("\n==============    GET ITEMS IDS    ==============");
// drive.getItemsIds();
//---------------

// using the watch and wait method
drive.watchAndWait(config.syncRootPath);

export default drive;
