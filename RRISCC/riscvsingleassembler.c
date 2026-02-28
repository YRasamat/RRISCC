#define OPCODE_RTYPE 0x33
#define OPCODE_ITYPE 0x13
#define OPCODE_LOAD  0x03
#define OPCODE_STORE 0x23

#define FUNCT3_ADD   0x0
#define FUNCT3_ADDI  0x0
#define FUNCT3_LW    0x2
#define FUNCT3_SW    0x2

#define FUNCT7_ADD   0x00

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

// trim leading and trailing whitespace in place
char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

void print_error(const char *filename, int line_num, const char *line, const char *msg, int col) {
    fprintf(stderr, "%s:%d: error: %s\n", filename, line_num, msg);
    fprintf(stderr, "  %s\n", line);
    if (col >= 0) {
        fprintf(stderr, "  ");
        for (int i = 0; i < col; i++) fprintf(stderr, " ");
        fprintf(stderr, "^\n");
    }
}

int parse_reg(const char *s, const char *filename, int line_num, const char *line) {
    const char *trimmed = s;
    while (isspace((unsigned char)*trimmed)) trimmed++;
    if (trimmed[0] != 'x') {
        char msg[64];
        snprintf(msg, sizeof(msg), "expected register like x0-x31, got '%s'", s);
        print_error(filename, line_num, line, msg, (int)(trimmed - line));
        return -1;
    }
    int reg = atoi(trimmed + 1);
    if (reg < 0 || reg > 31) {
        char msg[64];
        snprintf(msg, sizeof(msg), "register out of range: '%s'", s);
        print_error(filename, line_num, line, msg, -1);
        return -1;
    }
    return reg;
}

int parse_imm_reg(const char *s, int *imm, int *reg,
                  const char *filename, int line_num, const char *line) {
    // format: imm(xN)
    if (sscanf(s, "%d(x%d)", imm, reg) != 2) {
        char msg[64];
        snprintf(msg, sizeof(msg), "expected format imm(xN), got '%s'", s);
        print_error(filename, line_num, line, msg, -1);
        return 0;
    }
    return 1;
}

uint32_t encode_addi(int rd, int rs1, int imm) {
    return ((imm & 0xFFF) << 20) |
           (rs1 << 15) |
           (FUNCT3_ADDI << 12) |
           (rd << 7) |
           OPCODE_ITYPE;
}

uint32_t encode_add(int rd, int rs1, int rs2) {
    return (FUNCT7_ADD << 25) |
           (rs2 << 20) |
           (rs1 << 15) |
           (FUNCT3_ADD << 12) |
           (rd << 7) |
           OPCODE_RTYPE;
}

uint32_t encode_lw(int rd, int rs1, int imm) {
    uint32_t imm12 = imm & 0xFFF;
    return (imm12 << 20) |
           (rs1 << 15) |
           (FUNCT3_LW << 12) |
           (rd << 7) |
           OPCODE_LOAD;
}

uint32_t encode_sw(int rs1, int rs2, int imm) {
    uint32_t imm12 = imm & 0xFFF;
    uint32_t imm_hi = (imm12 >> 5) & 0x7F;
    uint32_t imm_lo = imm12 & 0x1F;
    return (imm_hi << 25) |
           (rs2 << 20) |
           (rs1 << 15) |
           (FUNCT3_SW << 12) |
           (imm_lo << 7) |
           OPCODE_STORE;
}

// split a string by delimiter, trimming whitespace from each token
int split(char *str, char delim, char tokens[][32], int max_tokens) {
    int count = 0;
    char *p = str;
    while (*p && count < max_tokens) {
        while (isspace((unsigned char)*p)) p++;
        char *start = p;
        while (*p && *p != delim) p++;
        int len = (int)(p - start);
        // trim trailing whitespace
        while (len > 0 && isspace((unsigned char)start[len-1])) len--;
        strncpy(tokens[count], start, len);
        tokens[count][len] = '\0';
        count++;
        if (*p == delim) p++;
    }
    return count;
}

int main(int argc, char *argv[]) {
    const char *input    = (argc > 1) ? argv[1] : "riscvtest.s";
    const char *output   = (argc > 2) ? argv[2] : "riscvtest.mem";

    FILE *in  = fopen(input, "r");
    FILE *out = fopen(output, "w");

    if (!in)  { fprintf(stderr, "error: cannot open input file '%s'\n", input);  return 1; }
    if (!out) { fprintf(stderr, "error: cannot open output file '%s'\n", output); return 1; }

    char raw[256];
    int line_num = 0;
    int errors = 0;

    while (fgets(raw, sizeof(raw), in)) {
        line_num++;

        // strip newline for display
        char line[256];
        strncpy(line, raw, sizeof(line));
        line[strcspn(line, "\n")] = '\0';

        char *trimmed = trim(line);

        // skip empty lines and comments
        if (strlen(trimmed) == 0 || trimmed[0] == '#' || trimmed[0] == ';') continue;

        // split into opcode and the rest
        char op[32] = {0};
        char rest[224] = {0};
        if (sscanf(trimmed, "%31s %223[^\n]", op, rest) < 1) continue;

        // split rest by commas
        char args[4][32] = {{0}};
        int argc2 = split(rest, ',', args, 4);

        uint32_t instr = 0;
        int ok = 1;

        if (!strcmp(op, "addi")) {
            if (argc2 < 3) {
                print_error(input, line_num, trimmed, "addi requires 3 arguments: rd, rs1, imm", -1);
                errors++; continue;
            }
            int rd  = parse_reg(args[0], input, line_num, trimmed);
            int rs1 = parse_reg(args[1], input, line_num, trimmed);
            int imm = atoi(args[2]);
            if (rd < 0 || rs1 < 0) { errors++; continue; }
            instr = encode_addi(rd, rs1, imm);

        } else if (!strcmp(op, "add")) {
            if (argc2 < 3) {
                print_error(input, line_num, trimmed, "add requires 3 arguments: rd, rs1, rs2", -1);
                errors++; continue;
            }
            int rd  = parse_reg(args[0], input, line_num, trimmed);
            int rs1 = parse_reg(args[1], input, line_num, trimmed);
            int rs2 = parse_reg(args[2], input, line_num, trimmed);
            if (rd < 0 || rs1 < 0 || rs2 < 0) { errors++; continue; }
            instr = encode_add(rd, rs1, rs2);

        } else if (!strcmp(op, "lw")) {
            if (argc2 < 2) {
                print_error(input, line_num, trimmed, "lw requires 2 arguments: rd, imm(rs1)", -1);
                errors++; continue;
            }
            int rd = parse_reg(args[0], input, line_num, trimmed);
            int imm, rs1;
            if (!parse_imm_reg(args[1], &imm, &rs1, input, line_num, trimmed)) { errors++; continue; }
            if (rd < 0) { errors++; continue; }
            instr = encode_lw(rd, rs1, imm);

        } else if (!strcmp(op, "sw")) {
            if (argc2 < 2) {
                print_error(input, line_num, trimmed, "sw requires 2 arguments: rs2, imm(rs1)", -1);
                errors++; continue;
            }
            int rs2 = parse_reg(args[0], input, line_num, trimmed);
            int imm, rs1;
            if (!parse_imm_reg(args[1], &imm, &rs1, input, line_num, trimmed)) { errors++; continue; }
            if (rs2 < 0) { errors++; continue; }
            instr = encode_sw(rs1, rs2, imm);

        } else {
            char msg[64];
            snprintf(msg, sizeof(msg), "unknown instruction '%s'", op);
            print_error(input, line_num, trimmed, msg, 0);
            errors++;
            continue;
        }

        fprintf(out, "%08X\n", instr);
    }

    fclose(in);
    fclose(out);

    if (errors > 0) {
        fprintf(stderr, "%d error(s) found. output may be incomplete.\n", errors);
        return 1;
    }

    printf("Assembly successful.\n");
    return 0;
}