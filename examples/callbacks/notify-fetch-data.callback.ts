async function onFetchData(fileId: string): Promise<boolean> {
    console.log("[EXAMPLE] downloading file: " + fileId);
    // simulating a download from a real server
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

type CallbackResponse = (data : boolean, path: string, errorHandler?: () => void) => Promise<{ finished: boolean, progress: number }>;
async function onFetchDataCallback(fileId: string, callback: CallbackResponse ) {
    console.log("[EXAMPLE] file id: " + fileId);
    // simulate a download from a real server and response with the path of the downloaded file of a fake server
    let finish = false;
    onFetchData(fileId).then(async (response) => {
        while (!finish) {
            const callbackResponse = await callback(response, "C:\\Users\\gcarl\\Desktop\\fakeserver\\imagen.rar");
            finish = callbackResponse.finished;
            if (finish) {
                console.log("[EXAMPLE] finished");
                break;
            };
        };

    }).catch((err) => { // THIS CATCH IS REALLY IMPORTANT
        //callback(false, "C:\\Users\\gcarl\\Desktop\\fakeserver\\imagen.rar");
        console.log(err);
    });
}

export default onFetchDataCallback;