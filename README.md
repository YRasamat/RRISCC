# RRISCC
**RISC-V Single Cycle Assembler**

A lightweight assembler for RV32I assembly, designed to integrate with Vivado for FPGA simulation on the Basys3. Includes a TCL script to automate assembly and copy the output `.mem` file directly into your Vivado simulation directory.

---

## Supported Instructions(for now)

| Instruction | Format | Description |
|-------------|--------|-------------|
| `add` | `add rd, rs1, rs2` | rd = rs1 + rs2 |
| `addi` | `addi rd, rs1, imm` | rd = rs1 + immediate |
| `lw` | `lw rd, imm(rs1)` | rd = Memory[rs1 + imm] |
| `sw` | `sw rs2, imm(rs1)` | Memory[rs1 + imm] = rs2 |

---

## Compiling the Assembler

```bash
gcc riscvsingleassembler.c -o assembler.exe
```

Or use the TCL script's built-in recompile flag (see below).

---

## Assembly File Format

- One instruction per line
- Registers are written as `x0`–`x31`
- Immediate values are decimal integers
- Blank lines and lines starting with `#` or `;` are ignored as comments

**Example `riscvtest.s`:**
```
# load and add example
addi x1, x0, 5
addi x2, x0, 10
add  x3, x1, x2
sw   x3, 0(x0)
lw   x4, 0(x0)
```

---

## Using the TCL Script in Vivado

The included `run_asm.tcl` script automates assembling and copying the `.mem` file to your Vivado simulation directory.

### Setup

1. Place `run_asm.tcl` in your Vivado project root
2. Update the paths at the top of the script if needed
3. In the Vivado TCL console, source the script:

```tcl
source "C:/path/to/your/project/run_asm.tcl"
```

### Normal Usage — Just Assemble
```tcl
source run_asm.tcl
```

### Recompile the Assembler First
```tcl
set recompile 1; source run_asm.tcl
```

This will recompile `riscvsingleassembler.c` using `gcc` before assembling. The flag resets to `0` automatically after recompiling.

---

## Project Structure

```
project/
├── run_asm.tcl                  # Vivado TCL automation script
└── RRISCC/
    ├── riscvsingleassembler.c   # Assembler source code
    ├── assembler.exe            # Compiled assembler
    ├── riscvtest.s              # Assembly input file
    └── riscvtest.mem            # Assembled output (hex)
```

---

## Output Format

The assembler outputs a `.mem` file with one 32-bit instruction per line in uppercase hexadecimal, suitable for use with Vivado's `$readmemh`:

```
00500093
00A00113
003080B3
00302023
00002203
```
