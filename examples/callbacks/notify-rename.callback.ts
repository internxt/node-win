async function onRenameCallback(newName: string, fileId: string): Promise<boolean> {
    console.log("File ID: " + fileId);
    console.log("New name: " + newName);

    const a = await (new Promise<boolean>((resolve, reject) => {
        try {

            setTimeout(() => {
                resolve(true);
            }, 1000)
        } catch (err) {
            reject(err);
        }
    }));

    return a;
}

function onRenameCallbackWithCallback(newName: string, fileId: string, responseCallback: (response: boolean) => void) {
    onRenameCallback(newName, fileId).then((response) => {
        responseCallback(response);
    }).catch((err) => {
        responseCallback(false);
    });
}

export default onRenameCallbackWithCallback;