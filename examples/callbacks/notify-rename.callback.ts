import { isTemporaryFile } from "../utils";

async function onRenameCallback(
  newName: string,
  fileId: string
): Promise<boolean> {
  console.log("[EXAMPLE] File ID: " + fileId);
  console.log("[EXAMPLE] New name: " + newName);

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

async function onRenameCallbackWithCallback(
  newName: string,
  fileId: string,
  responseCallback: (response: boolean) => void
) {
  const isTempFile = await isTemporaryFile(newName);

  console.debug("[isTemporaryFile]", isTempFile);

  if (isTempFile) {
    console.debug("File is temporary, skipping");
    return;
  }
  onRenameCallback(newName, fileId)
    .then((response) => {
      responseCallback(response);
    })
    .catch((err) => {
      responseCallback(false);
    });
}

export default onRenameCallbackWithCallback;
