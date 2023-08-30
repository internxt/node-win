const { parentPort, workerData } = require('worker_threads');
const pathModule = require('path');
const addonPath = pathModule.join(__dirname, '../../build/Release/addon.node');
const addon = require(addonPath);

const { path } = workerData;

try {
  addon.watchAndWait(path);
  parentPort.postMessage('Observación completada');
} catch (error) {
  //@ts-ignore
  console.log(`Error en la observación: ${error.message}`)
    //@ts-ignore
  parentPort.postMessage(`Error en la observación: ${error.message}`);
}
