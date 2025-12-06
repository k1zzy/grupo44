import subprocess
import time
import os
import signal
import sys
import re

# Configuration
ZK_HOST = "127.0.0.1:2181"
SERVER_BIN = "binary/list_server"
CLIENT_BIN = "binary/list_client"
START_PORT = 5000

# Helper to start a server
def start_server(port):
    print(f"Starting server on port {port}...")
    proc = subprocess.Popen(
        [SERVER_BIN, ZK_HOST, str(port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    time.sleep(1) # Wait for startup
    if proc.poll() is not None:
        print(f"Server on port {port} failed to start.")
        print(proc.stdout.read())
        print(proc.stderr.read())
        sys.exit(1)
    return proc

# Helper to start a client and run commands
def run_client_commands(commands, check_output=None):
    print(f"Running client commands: {commands}")
    proc = subprocess.Popen(
        [CLIENT_BIN, ZK_HOST],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    input_str = "\n".join(commands) + "\nquit\n"
    stdout, stderr = proc.communicate(input=input_str)
    
    if check_output:
        if check_output not in stdout:
            print(f"Assertion Failed! Expected '{check_output}' in output.")
            print("Client Output:\n", stdout)
            return False
        else:
            print(f"Assertion Passed: found '{check_output}'")
    
    return True

# Helper to kill a process
def kill_process(proc):
    if proc.poll() is None:
        os.kill(proc.pid, signal.SIGTERM)
        proc.wait()

def test_single_server():
    print("\n=== Test 1: Single Server ===")
    s1 = start_server(START_PORT)
    try:
        if not run_client_commands(["add m1 2020 1000 1 1"], "Carro adicionado"):
             sys.exit(1)
        if not run_client_commands(["size"], "Tamanho da lista: 1"):
             sys.exit(1)
    finally:
        kill_process(s1)
    print("Test 1 Passed.")

def test_chain_replication():
    print("\n=== Test 2: Chain Replication (3 Servers) ===")
    servers = []
    try:
        # Start 3 servers
        for i in range(3):
            servers.append(start_server(START_PORT + i))
            time.sleep(1) # Stagger start to ensure order

        # Write to Head (handled by client logic automatically via ZK)
        # Verify read at Tail
        if not run_client_commands(["add m1 2020 1000 1 1"], "Carro adicionado"):
            sys.exit(1)
        
        # Give time for propagation
        time.sleep(1)

        if not run_client_commands(["size"], "Tamanho da lista: 1"):
            sys.exit(1)
            
        if not run_client_commands(["get_by_year 2020"], "Modelo: m1"):
            sys.exit(1)

    finally:
        for s in servers:
            kill_process(s)
    print("Test 2 Passed.")

def test_fault_tolerance_middle():
    print("\n=== Test 3: Fault Tolerance (Kill Middle) ===")
    servers = []
    try:
        # Start 3 servers
        for i in range(3):
            servers.append(start_server(START_PORT + 10 + i))
            time.sleep(1)

        # Add initial data
        run_client_commands(["add m1 2020 1000 1 1"])
        time.sleep(1)

        # Kill middle server (index 1)
        print("Killing middle server...")
        kill_process(servers[1])
        time.sleep(3) # Wait for ZK detection and reconfiguration

        # Write new data
        if not run_client_commands(["add m2 2021 2000 2 2"], "Carro adicionado"):
            sys.exit(1)
        
        time.sleep(1)

        # Verify both items exist (tail should be servers[2])
        if not run_client_commands(["size"], "Tamanho da lista: 2"):
            sys.exit(1)

    finally:
        kill_process(servers[0])
        kill_process(servers[2]) # servers[1] already killed
    print("Test 3 Passed.")

def test_fault_tolerance_tail():
    print("\n=== Test 4: Fault Tolerance (Kill Tail) ===")
    servers = []
    try:
        for i in range(3):
            servers.append(start_server(START_PORT + 20 + i))
            time.sleep(1)

        run_client_commands(["add m1 2020 1000 1 1"])
        time.sleep(1)

        # Kill tail (index 2)
        print("Killing tail server...")
        kill_process(servers[2])
        time.sleep(3)

        # Verify read from new tail (servers[1])
        if not run_client_commands(["size"], "Tamanho da lista: 1"):
            sys.exit(1)

        # Add more data
        run_client_commands(["add m2 2021 2000 2 2"])
        time.sleep(1)

        if not run_client_commands(["size"], "Tamanho da lista: 2"):
            sys.exit(1)
            
    finally:
        kill_process(servers[0])
        kill_process(servers[1])
    print("Test 4 Passed.")

def test_new_server_joins():
    print("\n=== Test 5: New Server Joins (State Sync) ===")
    servers = []
    try:
        # Start 1 server
        s1 = start_server(START_PORT + 30)
        servers.append(s1)
        
        # Add data
        run_client_commands(["add m1 2020 1000 1 1"])
        
        # Start new server
        print("Starting new server...")
        s2 = start_server(START_PORT + 31)
        servers.append(s2)
        time.sleep(2) # Wait for sync

        # New server should be tail. Verify it has data.
        # We can test this by killing s1, making s2 the head and tail, then querying.
        print("Killing head server to force s2 to be head/tail...")
        kill_process(s1)
        time.sleep(3)
        
        if not run_client_commands(["size"], "Tamanho da lista: 1"):
            # If s2 didn't sync, size would be 0
            sys.exit(1)

    finally:
        if len(servers) > 1:
            kill_process(servers[1])
    print("Test 5 Passed.")


import threading

def test_concurrent_operations():
    print("\n=== Test 6: Concurrent Operations ===")
    servers = []
    try:
        # Start 3 servers
        for i in range(3):
            servers.append(start_server(START_PORT + 40 + i))
            time.sleep(1)

        # Function for writer threads
        def writer_func(thread_id, n_ops):
            for k in range(n_ops):
                # Unique model per thread/op to avoid collision in verify
                cmd = f"add m_{thread_id}_{k} 2022 3000 3 3"
                if not run_client_commands([cmd], "Carro adicionado"):
                    print(f"Writer {thread_id} failed at op {k}")
                    return

        # Function for reader threads
        def reader_func(thread_id, n_ops):
            for k in range(n_ops):
                # Just check size or list, don't strict assert as it changes
                run_client_commands(["size"])
        
        threads = []
        n_writers = 3
        n_readers = 3
        ops_per_thread = 5

        print(f"Starting {n_writers} writers and {n_readers} readers, {ops_per_thread} ops each...")

        # Start writers
        for i in range(n_writers):
            t = threading.Thread(target=writer_func, args=(i, ops_per_thread))
            threads.append(t)
            t.start()

        # Start readers
        for i in range(n_readers):
            t = threading.Thread(target=reader_func, args=(i, ops_per_thread))
            threads.append(t)
            t.start()

        # Wait for all
        for t in threads:
            t.join()
            
        print("All threads finished.")

        # Verify final state
        # Total writes = n_writers * ops_per_thread
        expected_size = n_writers * ops_per_thread
        if not run_client_commands(["size"], f"Tamanho da lista: {expected_size}"):
            sys.exit(1)

    finally:
        for s in servers:
            kill_process(s)
    print("Test 6 Passed.")

if __name__ == "__main__":
    # Ensure binaries exist
    if not os.path.exists(SERVER_BIN):
        print(f"Error: {SERVER_BIN} not found. Run make first.")
        sys.exit(1)
        
    try:
        test_single_server()
        test_chain_replication()
        test_fault_tolerance_middle()
        test_fault_tolerance_tail()
        test_new_server_joins()
        test_concurrent_operations()
        print("\nALL TESTS PASSED!")
    except KeyboardInterrupt:
        print("\nInterrupted.")
    except Exception as e:
        print(f"\nTest Failed: {e}")
