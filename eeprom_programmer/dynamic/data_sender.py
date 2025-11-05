'''
Wojciech Gorzynski
15-08-2025 v2

This program reads a binary file and sends its contents to an Arduino over a serial connection.
It expects the Arduino to request data in a specific format:
- The Arduino sends 'S' to request the start of data transfer.
- The Arduino sends 'R' to request the next byte of data.
- The program sends bytes one by one until all data is sent.
'''
import sys
import serial
import time

def readfile(path):
    # Reads a binary file and returns its contents as a bytes object.
    try:
        with open(path, "rb") as file:
            return file.read()
    except FileNotFoundError:
        print(f"Error: The file {path} does not exist.")
        sys.exit(1)

def _print_progress(sent, total, start):
    pct = sent / total if total else 1
    bar_width = 40
    filled = int(pct * bar_width)
    bar = "#" * filled + "-" * (bar_width - filled)
    elapsed = time.perf_counter() - start
    rate = sent / elapsed if elapsed > 0 else 0
    remaining = (total - sent) / rate if rate > 0 else 0
    sys.stdout.write(f"\r[{bar}] {sent}/{total} ({pct*100:5.1f}%)  {rate:6.1f} B/s  ETA {remaining:5.1f}s")
    sys.stdout.flush()

def _monitor_post_transfer(serial_port, seconds=30):
    # Read and print raw lines (newline-terminated) from the Arduino.
    serial_port.timeout = 0.5
    end = time.perf_counter() + seconds
    buf = bytearray()
    try:
        while time.perf_counter() < end:
            if serial_port.in_waiting:
                data = serial_port.read(serial_port.in_waiting)
                buf.extend(data)
                # print any complete newline-terminated lines as text
                while True:
                    nl = buf.find(b'\n')
                    if nl == -1:
                        break
                    line = bytes(buf[:nl + 1])
                    del buf[:nl + 1]
                    try:
                        sys.stdout.write(line.decode('utf-8', errors='replace'))
                    except Exception:
                        sys.stdout.buffer.write(line)
                    sys.stdout.flush()
            else:
                time.sleep(0.05)
        # print any remaining data (not newline-terminated) as text
        if buf:
            try:
                sys.stdout.write(bytes(buf).decode('utf-8', errors='replace'))
            except Exception:
                sys.stdout.buffer.write(bytes(buf))
            sys.stdout.flush()
    except Exception as e:
        print("Monitor error:", e)

def main():
    if len(sys.argv) != 3:
        print("Usage: python data_sender.py <path_to_binary_file> <target_serial_port>")
        sys.exit(1)
    
    path = sys.argv[1]
    target_serial_port = sys.argv[2]

    bytearr = readfile(path)
    total = len(bytearr)
    print(f"{total} bytes read from {path}")
    serial_port = serial.Serial(target_serial_port, 57600, timeout=None)
    idx = 0
    start = time.perf_counter()

    try:
        while True:
            data = serial_port.read(1)  # block until Arduino sends a request
            if not data:
                continue
            if data == b"S":
                # Arduino requests start/address (high, low)
                addr = idx
                serial_port.write(bytes([(addr >> 8) & 0xFF, addr & 0xFF]))
            elif data == b"R":
                if idx < total:
                    serial_port.write(bytes([bytearr[idx]]))
                    idx += 1
                    _print_progress(idx, total, start)
                else:
                    # all bytes sent, notify end
                    serial_port.write(b"H")
                    # finalize progress display
                    _print_progress(total, total, start)
                    print("\nAll data sent")
                    # monitor what the Arduino sends for 30 seconds
                    _monitor_post_transfer(serial_port, seconds=30)
                    break
            else:
                # ignore unexpected bytes
                continue
    finally:
        serial_port.close()

if __name__ == "__main__":
    main()