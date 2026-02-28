# run_asm.tcl
# Usage: source run_asm.tcl                    <- just assemble
#        set recompile 1; source run_asm.tcl   <- recompile assembler first

set assembler_src "C:/Users/yonik/Documents/picoRV32/RiscV_Basys3/rv_core/RRISCC/riscvsingleassembler.c"
set assembler_exe "C:/Users/yonik/Documents/picoRV32/RiscV_Basys3/rv_core/RRISCC/assembler.exe"
set asm_file      "C:/Users/yonik/Documents/picoRV32/RiscV_Basys3/rv_core/RRISCC/riscvtest.s"
set dest "C:/Users/yonik/Documents/picoRV32/RiscV_Basys3/rv_core/rv_core.srcs/sim_1/imports/RISC_compiler/riscvtest.mem"

if {[info exists recompile] && $recompile == 1} {
    puts "Recompiling assembler..."
    exec gcc $assembler_src -o $assembler_exe
    puts "Assembler recompiled."
    set recompile 0
}

puts "Assembling riscvtest.s..."
exec $assembler_exe $asm_file $dest
puts "Done! riscvtest.mem written to sim directory."