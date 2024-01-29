async function onDeleteCallback(fileId: string, callback: (response: boolean) => void) {
    console.log("[EXAMPLE] On delete File ID: " + fileId);
    const a = await (new Promise<boolean>((resolve, reject) => {
        try {
            setTimeout(() => {
                resolve(true);
            }, 10)
        } catch (err) {
            reject(err);
        }
    }));

    return a;
}

function onDeleteCallbackWithCallback(fileId: string, callback: (response: boolean) => void) {
    onDeleteCallback(fileId, callback).then((response) => {
        callback(response);
    }).catch((err) => {
        callback(false);
    });
}

export default onDeleteCallbackWithCallback;