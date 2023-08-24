const { parentPort, workerData } = require('worker_threads');
const addon = require('../../build/Release/addon.node');

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
