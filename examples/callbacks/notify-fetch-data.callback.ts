import { ItemsInfoManager } from "../utils";

async function onFetchData(fileId: string): Promise<boolean> {
    console.log("[EXAMPLE] downloading file_: " + fileId);

    const a = await (new Promise<boolean>((resolve, reject) => {
        try {
            console.log("[EXAMPLE] try downloading file: " + fileId);
            setTimeout(() => {
                console.log("[EXAMPLE] after timeout:")
                resolve(true);
            }, 1000)
        } catch (err) {
            console.log("[EXAMPLE] error: " + err);
            reject(err);
        }
    }));

    return a;
}

type CallbackResponse = (data : boolean, path: string, errorHandler?: () => void) => Promise<{ finished: boolean, progress: number }>;

async function onFetchDataCallback(fileId: string, callback: CallbackResponse ) {
    console.log("[EXAMPLE] file id: " + fileId);
    // simulate a download from a real server and response with the path of the downloaded file of a fake server
    let finish = false;
    onFetchData(fileId).then(async (response) => {
        while (!finish) {
            const itemsManager = await ItemsInfoManager.initialize('dist/examples/filesInfo.json')
            console.log("[EXAMPLE] itemsManager: " + itemsManager);
            const itemPath = itemsManager.get(fileId.replace(/\x00/g, ''))
            console.log("[EXAMPLE] itemPath: " + itemPath);

            if (!itemPath) {
                console.log("[EXAMPLE] error: file not found");
                finish = true;
                break;
            }

            console.log("[EXAMPLE] downloaded file out: " + itemPath);

            const callbackResponse = await callback(response, itemPath);
            finish = callbackResponse.finished;
            if (finish) {
                console.log("[EXAMPLE] finished");
                break;
            };
        };

    }).catch((err) => {
        //callback(false, "C:\\Users\\gcarl\\Desktop\\fakeserver\\imagen.rar");
        console.log('[EXAMPLE] error:' + err);
    });
}

export default onFetchDataCallback;