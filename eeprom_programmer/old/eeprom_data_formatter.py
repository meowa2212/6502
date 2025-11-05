'''
Wojciech Gorzynski
14-08-2025 v1

Transforms a binary file into a C-style hex array for use in an Arduino EEPROM programmer.
'''
import sys

def readfile(path):
    """Reads a binary file and returns its contents as a bytes object."""
    try:
        with open(path, "rb") as file:
            return file.read()
    except FileNotFoundError:
        print(f"Error: The file {path} does not exist.")
        sys.exit(1)

def format_c_array(bytearr, var_name="data", bytes_per_line=16):
    """Formats a bytes object as a C-style PROGMEM byte array for Arduino."""
    lines = []
    for i in range(0, len(bytearr), bytes_per_line):
        chunk = bytearr[i:i+bytes_per_line]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append("    " + line)
    return f"const byte {var_name}[] PROGMEM = {{\n" + ",\n".join(lines) + "\n};\n"

def main():
    if len(sys.argv) != 3:
        print("Usage: python formatter.py <path_to_binary_file> <target_file>")
        sys.exit(1)
    
    path = sys.argv[1]
    target_file = sys.argv[2]
    
    bytearr = readfile(path)
    print(f"{len(bytearr)} bytes read from {path}")
    
    formatted = format_c_array(bytearr)
    
    with open(target_file, "w") as file:
        file.write(formatted)
    
    print(f"Formatted C array written to {target_file}")

if __name__ == "__main__":
    main()