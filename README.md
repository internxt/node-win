# Setup

This guide explains how to set up and build the `node-win` project.

## Prerequisites

Before proceeding, ensure you have the following tools installed:

- **Python 3.10**
- **Node 18**

```bash
nvm install 18
```

- **yarn**

```bash
npm install -g yarn
```

- **node-gyp**

```bash
npm install -g node-gyp
```

- **Visual Studio** (not VS Code) for building native dependencies.

## Build Steps

Run the following command to build the project:

```bash
npm run build
```

This step compiles the necessary native bindings.
