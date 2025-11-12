# USB Loader Packer/Unpacker

A tool to unpack and repack Balong USB loader images (`usbloader.bin` files).

## Features

- **Unpack USB loaders**: Extracts individual components (raminit, usbldr) from USB loader files
- **Repack USB loaders**: Reconstructs USB loader files from extracted components
- **Metadata preservation**: Saves block descriptors and file structure for accurate repacking
- **Verification tested**: All 34+ USB loaders in this repository can be unpacked and repacked to identical files

## USB Loader Format

A Balong USB loader consists of:
- **Header** (84 bytes): Magic signature and block descriptors
  - Magic: `0x00020000` at offset 0
  - Block descriptors at offset 36+, each 16 bytes:
    - `lmode` (4 bytes): Boot mode (1=direct start, 2=via A-core restart)
    - `size` (4 bytes): Block size in bytes
    - `address` (4 bytes): Load address in memory
    - `offset` (4 bytes): Offset from file start
- **Blocks**: Typically 2 blocks:
  - Block 0: `raminit` - RAM initialization code
  - Block 1: `usbldr` - Main USB loader code (contains partition table)

## Usage

### Unpacking a USB Loader

```bash
# Unpack to default directory (usbloader.bin.unpacked/)
./usbloader-packer -u usbloader.bin

# Unpack to specific directory
./usbloader-packer -u usbloader.bin -d my-output-dir
```

This creates:
- `header.bin` - Original header (84 bytes)
- `block0_raminit.bin` - RAM initialization code
- `block1_usbldr.bin` - USB loader main code
- `metadata.txt` - Block descriptors for repacking

### Repacking a USB Loader

```bash
# Repack from unpacked directory
./usbloader-packer -p my-output-dir -o usbloader-new.bin
```

### Example: Modifying a USB Loader

```bash
# 1. Unpack the loader
./usbloader-packer -u usbloader-3372h.bin -d work

# 2. Modify extracted files (e.g., patch the usbldr)
# ... your modifications to work/block1_usbldr.bin ...

# 3. Repack the modified loader
./usbloader-packer -p work -o usbloader-3372h-modified.bin
```

## Metadata Format

The `metadata.txt` file contains block information in INI format:

```ini
[Block0]
name=raminit
lmode=1
address=0x00000000
file=block0_raminit.bin

[Block1]
name=usbldr
lmode=2
address=0x57700000
file=block1_usbldr.bin
```

Block sizes are automatically determined from the actual file sizes during repacking, so there's no need to store them in the metadata. This ensures the metadata stays in sync with the actual files.

## Use Cases

1. **Extracting partitions**: Unpack the loader to access and modify individual blocks
2. **Patching**: Apply patches to specific blocks and repack
3. **Analysis**: Study the structure and contents of USB loaders
4. **Backup**: Create modular backups of USB loader components

## Verification

All USB loaders in this repository have been tested:
- Unpacked successfully
- Repacked to byte-for-byte identical files
- No data loss or corruption

```bash
# Verify unpack/repack integrity
./usbloader-packer -u original.bin -d test
./usbloader-packer -p test -o repacked.bin
diff original.bin repacked.bin  # Should show no differences
```

## Building

```bash
make usbloader-packer
```

## Command-Line Options

```
Options:
  -u <file>    Unpack USB loader file
  -p <dir>     Pack USB loader from directory
  -o <file>    Output file (for pack mode)
  -d <dir>     Output directory (for unpack mode, default: <input>.unpacked)
  -h           Show help
```

## Notes

- The tool preserves all header information including padding
- Block descriptors must be consecutive (no gaps in block indices)
- Block sizes are automatically determined from actual file sizes during repacking
- Offsets are automatically calculated during repacking
- The header is preserved from the original file when available
