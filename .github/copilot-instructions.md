# GitHub Copilot Instructions for balong-usbdload

## Project Overview

This is an emergency USB boot loader utility for Huawei LTE modems and routers with Balong V2R7, V7R11, and V7R22 chipsets. The utility loads external boot loader/firmware update tools via emergency serial port when firmware is corrupted or boot pin is shorted to ground.

**⚠️ CRITICAL SAFETY WARNING**: This utility can make devices unbootable. Code changes must be carefully reviewed to avoid introducing bugs that could brick devices.

## Technology Stack

- **Language**: C (ANSI C with some POSIX extensions)
- **Build System**: GNU Make
- **Platform**: Primary target is Linux; Windows builds are maintained separately
- **Compiler**: GCC with flags: `-O2 -g -Wno-unused-result`

## Project Structure

### Main Components

- `balong-usbdload.c` - Main loader utility that communicates with emergency serial port
- `exploit.c/h` - Exploit code for device initialization
- `patcher.c/h` - Automatic patching system to disable flash erasure in USB loaders
- `parts.c/h` - Partition table handling
- `loader-patch.c` - Standalone utility to patch USB loader files
- `ptable-*.c` - Partition table manipulation utilities (injector, editor, list)

### Binary Files

- `usbloader-*.bin` - Original USB loader files from Huawei
- `usblsafe-*.bin` - Patched "safe" loaders with flash erasure disabled

## Build Instructions

```bash
make clean  # Clean build artifacts
make        # Build all utilities
```

Build artifacts to be excluded from version control (already in .gitignore):
- `*.o` - Object files
- `balong-usbdload` - Main executable
- `ptable-injector`, `loader-patch`, `ptable-list`, `ptable-editor` - Utility executables

## Coding Conventions

### Style Guidelines

1. **Platform-specific code**: Use `#ifndef WIN32` / `#else` blocks for Linux vs Windows code
2. **Comments**: Russian comments are common in the codebase (as per project maintainer preference)
3. **Line endings**: Some files use CRLF, some use LF - preserve existing line endings
4. **Variable naming**: Use descriptive names; prefer lowercase with underscores (e.g., `siofd`, `fsize`)
5. **Error handling**: Always check return values from system calls and file operations
6. **Memory safety**: Be extremely careful with buffer operations given the low-level nature of the code

### Critical Code Areas

1. **Serial port communication**: Any changes to serial I/O must be thoroughly tested
2. **Patching logic**: The automatic patcher prevents flash erasure - bugs here can destroy device data
3. **Memory operations**: Buffer overflows or incorrect offsets can brick devices
4. **Signature matching**: Incorrect signatures in patcher.c can prevent patches from being applied

## Development Guidelines

### Do's

- ✅ Make minimal, surgical changes
- ✅ Test with actual hardware when possible or carefully review logic
- ✅ Preserve existing functionality
- ✅ Add error checking for all system calls
- ✅ Follow existing code structure and patterns
- ✅ Focus on Linux version (primary platform)

### Don'ts

- ❌ Don't modify Windows-specific code unless explicitly requested (maintained by @rust3028)
- ❌ Don't add dependencies on external libraries
- ❌ Don't remove or modify existing USB loader binary files without good reason
- ❌ Don't change the automatic patching logic without thorough understanding
- ❌ Don't optimize for readability at the expense of stability
- ❌ Don't add USB loader files or provide guidance on obtaining them (out of scope)

## Testing Approach

⚠️ **No automated test suite exists** - this is low-level hardware utility code.

Testing strategy:
1. Ensure code compiles without errors or warnings
2. Review code logic carefully for correctness
3. For hardware communication changes, testing with actual devices is required
4. Check that build produces all expected executables

## Common Tasks

### Adding Support for New Chipset

1. Add new patching function in `patcher.c` (e.g., `pv7r22_3`)
2. Define signature structure in `struct defpatch`
3. Update main loader to call new patching function
4. Document chipset version in comments

### Modifying Patch Logic

1. Understand the signature being searched for
2. Verify the patch offset is correct
3. Test with actual loader file to ensure patch applies correctly
4. Consider backward compatibility with existing loader files

### Adding New Utility

1. Create new `.c` file in root directory
2. Add build target to `Makefile`
3. Add executable name to `.gitignore`
4. Include appropriate headers (`parts.h`, `patcher.h` as needed)

## Important Notes

### Out of Scope

The following topics are explicitly out of scope for this repository:
- Questions about where to obtain USB loader files
- Boot pin locations for specific devices
- Windows build issues (handled separately)
- Device-specific troubleshooting

### Repository Purpose

This repository is strictly for development of the balong-usbdload utility itself. Pull requests should focus on:
- Bug fixes in the utility
- New features for the utility
- Support for additional Balong chipset variants
- Code quality improvements

## Safety Reminders

- This utility can permanently damage devices if used incorrectly
- The automatic patcher prevents NVRAM erasure (which would destroy IMEI, S/N, calibration data)
- Never disable safety checks without the `-c` flag mechanism
- Always preserve existing error handling and validation logic

## Additional Resources

- Main documentation: `README.md`
- License: `LICENSE` (GPL-based)
- Project issues and discussions should focus on the utility itself, not on device-specific usage
