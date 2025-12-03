# Walkthrough - Phase 4: Chain Replication with ZooKeeper

I have implemented the skeleton and core logic for Chain Replication using ZooKeeper.

## Changes Implemented

### 1. ZooKeeper Utilities (`zookeeper_utils.c/h`)
- Created a helper module to handle ZooKeeper connections.
- Implemented [zookeeper_create_server_node](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/source/zookeeper_utils.c#45-65) to create ephemeral sequential nodes (`/chain/node-XXXX`).
- Implemented [zookeeper_get_children](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/source/zookeeper_utils.c#66-89) with a watcher to detect topology changes.

### 2. Server ([list_server.c](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/source/list_server.c) & [network_server.c](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/source/network_server.c))
- **Startup**: The server now connects to ZooKeeper and registers itself in `/chain`.
- **Topology Discovery**:
    - It watches `/chain` to identify its position.
    - If it has a predecessor, it connects to it and requests the full list to **synchronize state**.
    - If it has a successor, it connects to it for **request propagation**.
- **Request Propagation**:
    - Write operations (`ADD`, `REMOVE`) are executed locally.
    - If successful, they are propagated to the successor (if one exists).
    - The response is sent to the client only after the chain propagation is handled.

### 3. Client ([list_client.c](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/source/list_client.c))
- **Startup**: Connects to ZooKeeper.
- **Routing**:
    - Watches `/chain` to identify the **Head** (first node) and **Tail** (last node).
    - Sends [add](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/source/client_stub.c#128-173)/[remove](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/source/network_server.c#72-90) commands to the **Head**.
    - Sends `get_*`/[size](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/source/client_stub.c#277-299) commands to the **Tail**.
- **Failover**: Automatically updates Head/Tail pointers if servers join or leave.

### 4. Build System ([makefile](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/makefile))
- Updated to link against `libzookeeper_mt`.
- Added [zookeeper_utils.c](file:///c:/Users/rodri/Desktop/UNI/3%C2%BA%20Ano/1%C2%BA%20Semestre/SD/Projeto/etapa%204/grupo44/source/zookeeper_utils.c) to the build targets.

## How to Run (on Linux)

1.  **Start ZooKeeper**:
    ```bash
    zkServer.sh start
    ```

2.  **Compile**:
    ```bash
    make
    ```

3.  **Run Servers**:
    ```bash
    ./binary/list_server 127.0.0.1:2181 8080
    ./binary/list_server 127.0.0.1:2181 8081
    ```

4.  **Run Client**:
    ```bash
    ./binary/list_client 127.0.0.1:2181
    ```

## Verification
- Since you are on Windows, I have not run the compilation.
- Please ensure `libzookeeper-mt-dev` is installed on your Linux environment before compiling.
