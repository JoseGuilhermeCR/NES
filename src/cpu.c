#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "cpu.h"
#include "memory.h"

#define RESET_INTERRUPT_VECTOR 0xFFFC
#define NMI_INTERRUPT_VECTOR   0xFFFA
#define IRQ_INTERRUPT_VECTOR   0xFFFE
#define STACK_START            0x100
#define DEFAULT_STATUS_FLAG    0x20
#define CYCLES_AFTER_INTERRUPT 7

#define INSTR(x) static void x(Cpu *cpu, uint16_t addr)

static uint16_t Immediate(Cpu *cpu);
static uint16_t Zeropage(Cpu *cpu);
static uint16_t ZeropageIndexed(Cpu *cpu, uint8_t index);
static uint16_t Absolute(Cpu *cpu);
static uint16_t AbsoluteIndexed(Cpu *cpu, uint8_t index);
static uint16_t Relative(Cpu *cpu);
static uint16_t Indirect(Cpu *cpu);
static uint16_t IndexedIndirect(Cpu *cpu);
static uint16_t IndirectIndexed(Cpu *cpu);

INSTR(Brk);
INSTR(Ora);
INSTR(Asl);
INSTR(AslA);
INSTR(Php);
INSTR(Bpl);
INSTR(Clc);
INSTR(Jsr);
INSTR(And);
INSTR(Bit);
INSTR(Rol);
INSTR(RolA);
INSTR(Plp);
INSTR(Bmi);
INSTR(Sec);
INSTR(Rti);
INSTR(Eor);
INSTR(Lsr);
INSTR(LsrA);
INSTR(Pha);
INSTR(Jmp);
INSTR(Bvc);
INSTR(Cli);
INSTR(Rts);
INSTR(Adc);
INSTR(Ror);
INSTR(RorA);
INSTR(Pla);
INSTR(Bvs);
INSTR(Sei);
INSTR(Sta);
INSTR(Sty);
INSTR(Stx);
INSTR(Dey);
INSTR(Txa);
INSTR(Bcc);
INSTR(Tya);
INSTR(Txs);
INSTR(Ldy);
INSTR(Lda);
INSTR(Ldx);
INSTR(Tay);
INSTR(Tax);
INSTR(Bcs);
INSTR(Clv);
INSTR(Tsx);
INSTR(Cpy);
INSTR(Cmp);
INSTR(Dec);
INSTR(Iny);
INSTR(Dex);
INSTR(Bne);
INSTR(Cld);
INSTR(Cpx);
INSTR(Sbc);
INSTR(Inc);
INSTR(Inx);
INSTR(Beq);
INSTR(Sed);

static void IncDecImpl(Cpu *cpu, uint16_t addr, uint8_t change);
static void CmpImpl(Cpu *cpu, uint16_t addr, uint8_t reg);
static void AdcImpl(Cpu *cpu, uint16_t addr, uint8_t ones_complement);
static void Branch(Cpu *cpu, uint16_t addr, Status status,
           uint8_t value_needed);

typedef struct _Instruction {
    const char *mnemonic;
    uint8_t opcode;
    uint8_t cycles;
    AddressingMode adrMode;
    void (*execute)(Cpu* cpu, uint16_t addr);
} Instruction;

typedef union _InstructionOrNothing {
    const void *valid;
    Instruction instr;
} InstructionOrNothing;

typedef union _AddrDecodeInfo {
    struct {
        uint16_t addr;
    } absolute;

    struct {
        uint16_t AbsoluteAddr;
        uint16_t addr;
        uint8_t index;
    } absoluteIndexed;

    struct {
        uint16_t addr;
    } immediate;

    struct {
        uint8_t addr;
    } zeropage;

    struct {
        uint8_t zpAddr;
        uint8_t index;
        uint16_t addr;
    } zeropageIndexed;

    struct {
        uint16_t addr;
    } relative;

    struct {
        uint16_t addr;
        uint16_t iaddr;
    } indirect;

    struct {
        uint16_t addr;
        uint8_t zpAddr;
        uint8_t zpXAddr;
    } indexedIndirect;

    struct {
        uint16_t addr;
        uint8_t zpAddr;
        uint8_t y;
    } indirectIndexed;
} AddrDecodeInfo;

static const InstructionOrNothing gInstructionTable[] = {
    {.instr = {"BRK", 0x00, 7, IMPLICIT, Brk}},
    {.instr = {"ORA", 0x01, 6, INDEXED_INDIRECT, Ora}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"ORA", 0x05, 3, INDEXED_INDIRECT, Ora}},
    {.instr = {"ASL", 0x06, 5, ZEROPAGE, Asl}},
    {NULL},
    {.instr = {"PHP", 0x08, 3, IMPLICIT, Php}},
    {.instr = {"ORA", 0x09, 2, IMMEDIATE, Ora}},
    {.instr = {"ASL", 0x0A, 2, ACCUMULATOR, AslA}},
    {NULL},
    {NULL},
    {.instr = {"ORA", 0x0D, 4, ABSOLUTE, Ora}},
    {.instr = {"ASL", 0x0E, 6, ABSOLUTE, Asl}},
    {NULL},
    {.instr = {"BPL", 0x10, 2, RELATIVE, Bpl}},
    {.instr = {"ORA", 0x11, 5, INDIRECT_INDEXED, Ora}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"ORA", 0x15, 4, ZEROPAGE_X, Ora}},
    {.instr = {"ASL", 0x16, 6, ZEROPAGE_X, Asl}},
    {NULL},
    {.instr = {"CLC", 0x18, 2, IMPLICIT, Clc}},
    {.instr = {"ORA", 0x19, 4, ABSOLUTE_Y, Ora}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"ORA", 0x1D, 4, ABSOLUTE_X, Ora}},
    {.instr = {"ASL", 0x1E, 7, ABSOLUTE_X, Asl}},
    {NULL},
    {.instr = {"JSR", 0x20, 6, ABSOLUTE, Jsr}},
    {.instr = {"AND", 0x21, 6, INDEXED_INDIRECT, And}},
    {NULL},
    {NULL},
    {.instr = {"BIT", 0x24, 3, ZEROPAGE, Bit}},
    {.instr = {"AND", 0x25, 3, ZEROPAGE, And}},
    {.instr = {"ROL", 0x26, 5, ZEROPAGE, Rol}},
    {NULL},
    {.instr = {"PLP", 0x28, 4, IMPLICIT, Plp}},
    {.instr = {"AND", 0x29, 2, IMMEDIATE, And}},
    {.instr = {"ROL", 0x2A, 2, ACCUMULATOR, RolA}},
    {NULL},
    {.instr = {"BIT", 0x2C, 4, ABSOLUTE, Bit}},
    {.instr = {"AND", 0x2D, 4, ABSOLUTE, And}},
    {.instr = {"AND", 0x2E, 6, ABSOLUTE, Rol}},
    {NULL},
    {.instr = {"BMI", 0x30, 2, RELATIVE, Bmi}},
    {.instr = {"AND", 0x31, 5, INDIRECT_INDEXED, And}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"AND", 0x35, 4, ZEROPAGE_X, And}},
    {.instr = {"ROL", 0x36, 6, ZEROPAGE_X, Rol}},
    {NULL},
    {.instr = {"SEC", 0x38, 2, IMPLICIT, Sec}},
    {.instr = {"AND", 0x39, 4, ABSOLUTE_Y, And}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"AND", 0x3D, 4, ABSOLUTE_X, And}},
    {.instr = {"ROL", 0x3E, 7, ABSOLUTE_X, Rol}},
    {NULL},
    {.instr = {"RTI", 0x40, 6, IMPLICIT, Rti}},
    {.instr = {"EOR", 0x41, 6, INDEXED_INDIRECT, Eor}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"EOR", 0x45, 3, ZEROPAGE, Eor}},
    {.instr = {"LSR", 0x46, 5, ZEROPAGE, Lsr}},
    {NULL},
    {.instr = {"PHA", 0x48, 3, IMPLICIT, Pha}},
    {.instr = {"EOR", 0x49, 2, IMMEDIATE, Eor}},
    {.instr = {"LSR", 0x4A, 2, ACCUMULATOR, LsrA}},
    {NULL},
    {.instr = {"JMP", 0x4C, 3, ABSOLUTE, Jmp}},
    {.instr = {"EOR", 0x4D, 4, ABSOLUTE, Eor}},
    {.instr = {"LSR", 0x4E, 6, ABSOLUTE, Eor}},
    {NULL},
    {.instr = {"BVC", 0x50, 2, RELATIVE, Bvc}},
    {.instr = {"EOR", 0x51, 5, INDIRECT_INDEXED, Eor}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"EOR", 0x55, 4, ZEROPAGE_X, Eor}},
    {.instr = {"LSR", 0x56, 6, ZEROPAGE_X, Lsr}},
    {NULL},
    {.instr = {"CLI", 0x58, 6, IMPLICIT, Cli}},
    {.instr = {"EOR", 0x59, 4, ABSOLUTE_Y, Eor}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"EOR", 0x5D, 4, ABSOLUTE_X, Eor}},
    {.instr = {"LSR", 0x5E, 7, ABSOLUTE_X, Lsr}},
    {NULL},
    {.instr = {"RTS", 0x60, 6, IMPLICIT, Rts}},
    {.instr = {"ADC", 0x61, 6, INDEXED_INDIRECT, Adc}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"ADC", 0x65, 3, ZEROPAGE, Adc}},
    {.instr = {"ROR", 0x66, 5, ZEROPAGE, Ror}},
    {NULL},
    {.instr = {"PLA", 0x68, 4, IMPLICIT, Pla}},
    {.instr = {"ADC", 0x69, 2, IMMEDIATE, Adc}},
    {.instr = {"ROR", 0x6A, 2, ACCUMULATOR, RorA}},
    {NULL},
    {.instr = {"JMP", 0x6C, 5, INDIRECT, Jmp}},
    {.instr = {"ADC", 0x6D, 4, ABSOLUTE, Adc}},
    {.instr = {"ROR", 0x6E, 6, ABSOLUTE, Ror}},
    {NULL},
    {.instr = {"BVS", 0x70, 2, RELATIVE, Bvs}},
    {.instr = {"ADC", 0x71, 5, INDIRECT_INDEXED, Adc}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"ADC", 0x75, 4, ZEROPAGE_X, Adc}},
    {.instr = {"ROR", 0x76, 6, ZEROPAGE_X, Ror}},
    {NULL},
    {.instr = {"SEI", 0x78, 2, IMPLICIT, Sei}},
    {.instr = {"ADC", 0x79, 3, ABSOLUTE_Y, Adc}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"ADC", 0x7D, 3, ABSOLUTE_X, Adc}},
    {.instr = {"ROR", 0x7E, 7, ABSOLUTE_X, Ror}},
    {NULL},
    {NULL},
    {.instr = {"STA", 0x81, 6, INDEXED_INDIRECT, Sta}},
    {NULL},
    {NULL},
    {.instr = {"STY", 0x84, 3, ZEROPAGE, Sty}},
    {.instr = {"STA", 0x85, 3, ZEROPAGE, Sta}},
    {.instr = {"STX", 0x86, 3, ZEROPAGE, Stx}},
    {NULL},
    {.instr = {"DEY", 0x88, 2, IMPLICIT, Dey}},
    {NULL},
    {.instr = {"TXA", 0x8A, 2, IMPLICIT, Txa}},
    {NULL},
    {.instr = {"STY", 0x8C, 4, ABSOLUTE, Sty}},
    {.instr = {"STA", 0x8D, 4, ABSOLUTE, Sta}},
    {.instr = {"STX", 0x8E, 4, ABSOLUTE, Stx}},
    {NULL},
    {.instr = {"BCC", 0x90, 2, RELATIVE, Bcc}},
    {.instr = {"STA", 0x91, 6, INDIRECT_INDEXED, Sta}},
    {NULL},
    {NULL},
    {.instr = {"STY", 0x94, 4, ZEROPAGE_X, Sty}},
    {.instr = {"STA", 0x95, 4, ZEROPAGE_X, Sta}},
    {.instr = {"STX", 0x96, 4, ZEROPAGE_Y, Stx}},
    {NULL},
    {.instr = {"TYA", 0x98, 2, IMPLICIT, Tya}},
    {.instr = {"STA", 0x99, 5, ABSOLUTE_Y, Sta}},
    {.instr = {"TXS", 0x9A, 2, IMPLICIT, Txs}},
    {NULL},
    {NULL},
    {.instr = {"STA", 0x9D, 5, ABSOLUTE_X, Sta}},
    {NULL},
    {NULL},
    {.instr = {"LDY", 0xA0, 2, IMMEDIATE, Ldy}},
    {.instr = {"LDA", 0xA1, 6, INDEXED_INDIRECT, Lda}},
    {.instr = {"LDX", 0xA2, 2, IMMEDIATE, Ldx}},
    {NULL},
    {.instr = {"LDY", 0xA4, 2, ZEROPAGE, Ldy}},
    {.instr = {"LDA", 0xA5, 3, ZEROPAGE, Lda}},
    {.instr = {"LDA", 0xA6, 3, ZEROPAGE, Ldx}},
    {NULL},
    {.instr = {"TAY", 0xA8, 2, IMPLICIT, Tay}},
    {.instr = {"LDA", 0xA9, 2, IMMEDIATE, Lda}},
    {.instr = {"TAX", 0xAA, 2, IMPLICIT, Tax}},
    {NULL},
    {.instr = {"LDY", 0xAC, 4, ABSOLUTE, Ldy}},
    {.instr = {"LDA", 0xAD, 4, ABSOLUTE, Lda}},
    {.instr = {"LDX", 0xAE, 4, ABSOLUTE, Ldx}},
    {NULL},
    {.instr = {"BCS", 0xB0, 2, RELATIVE, Bcs}},
    {.instr = {"LDA", 0xB1, 5, INDIRECT_INDEXED, Lda}},
    {NULL},
    {NULL},
    {.instr = {"LDY", 0xB4, 4, ZEROPAGE_X, Ldy}},
    {.instr = {"LDA", 0xB5, 4, ZEROPAGE_X, Lda}},
    {.instr = {"LDX", 0xB6, 4, ZEROPAGE_Y, Ldx}},
    {NULL},
    {.instr = {"CLV", 0xB8, 2, IMPLICIT, Clv}},
    {.instr = {"LDA", 0xB9, 4, ABSOLUTE_Y, Lda}},
    {.instr = {"TSA", 0xBA, 2, IMPLICIT, Tsx}},
    {NULL},
    {.instr = {"LDY", 0xBC, 4, ABSOLUTE_X, Ldy}},
    {.instr = {"LDA", 0xBD, 4, ABSOLUTE_X, Lda}},
    {.instr = {"LDX", 0xBE, 4, ABSOLUTE_Y, Ldx}},
    {NULL},
    {.instr = {"CPY", 0xC0, 2, IMMEDIATE, Cpy}},
    {.instr = {"CMP", 0xC1, 6, INDEXED_INDIRECT, Cmp}},
    {NULL},
    {NULL},
    {.instr = {"CPY", 0xC4, 3, ZEROPAGE, Cpy}},
    {.instr = {"CMP", 0xC5, 3, ZEROPAGE, Cmp}},
    {.instr = {"DEC", 0xC6, 5, ZEROPAGE, Dec}},
    {NULL},
    {.instr = {"INY", 0xC8, 2, IMPLICIT, Iny}},
    {.instr = {"CMP", 0xC9, 2, IMMEDIATE, Cmp}},
    {.instr = {"DEX", 0xCA, 2, IMPLICIT, Dex}},
    {NULL},
    {.instr = {"CPY", 0xCC, 4, ABSOLUTE, Cpy}},
    {.instr = {"CMP", 0xCD, 4, ABSOLUTE, Cmp}},
    {.instr = {"DEC", 0xCE, 6, ABSOLUTE, Dec}},
    {NULL},
    {.instr = {"BNE", 0xD0, 2, RELATIVE, Bne}},
    {.instr = {"CMP", 0xD1, 5, INDIRECT_INDEXED, Cmp}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"CMP", 0xD5, 4, ZEROPAGE_X, Cmp}},
    {.instr = {"DEC", 0xD6,	6, ZEROPAGE_X, Dec}},
    {NULL},
    {.instr = {"CLD", 0xD8,	2, IMPLICIT, Cld}},
    {.instr = {"CMP", 0xD9, 4, ABSOLUTE_Y, Cmp}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"CMP", 0xDD, 4, ABSOLUTE_X, Cmp}},
    {.instr = {"DEC", 0xDE,	7, ABSOLUTE_X, Dec}},
    {NULL},
    {.instr = {"CPX", 0xE0, 2, IMMEDIATE, Cpx}},
    {.instr = {"SBC", 0xE1, 6, INDEXED_INDIRECT, Sbc}},
    {NULL},
    {NULL},
    {.instr = {"CPX", 0xE4, 3, ZEROPAGE, Cpx}},
    {.instr = {"SBC", 0xE5, 3, ZEROPAGE, Sbc}},
    {.instr = {"INC", 0xE6, 5, ZEROPAGE, Inc}},
    {NULL},
    {.instr = {"INX", 0xE8, 2, IMPLICIT, Inx}},
    {.instr = {"SBC", 0xE9, 2, IMMEDIATE, Sbc}},
    {.instr = {"NOP", 0xEA, 2, IMPLICIT, NULL}}, /* NOP */
    {NULL},
    {.instr = {"CPX", 0xEC, 4, ABSOLUTE, Cpx}},
    {.instr = {"SBC", 0xED, 4, ABSOLUTE, Sbc}},
    {.instr = {"INC", 0xEE, 6, ABSOLUTE, Inc}},
    {NULL},
    {.instr = {"BEQ", 0xF0, 2, RELATIVE, Beq}},
    {.instr = {"SBC", 0xF1, 5, INDIRECT_INDEXED, Sbc}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"SBC", 0xF5, 5, ZEROPAGE_X, Sbc}},
    {.instr = {"INC", 0xF6, 6, ZEROPAGE_X, Inc}},
    {NULL},
    {.instr = {"SED", 0xF8, 2, IMPLICIT, Sed}},
    {.instr = {"SBC", 0xF9, 4, ABSOLUTE_Y, Sbc}},
    {NULL},
    {NULL},
    {NULL},
    {.instr = {"SBC", 0xFD, 4, ABSOLUTE_X, Sbc}},
    {.instr = {"INC", 0xFE, 7, ABSOLUTE_X, Inc}},
    {NULL}
};

static AddrDecodeInfo gAddrDecodedInfo;

static void DebugInstruction(Memory *mem,
                  uint16_t oldPc,
                  uint8_t opcode,
                  uint8_t op1,
                  uint8_t op2,
                  const Instruction *instr) {
    printf("%04X %02X ", oldPc, opcode);
    
    // Save Memory Read Flags because we might change their values
    uint8_t ppustatusRead = mem->ppustatusRead;

    switch (instr->adrMode) {
    case IMMEDIATE:
        printf("%s #$%02X",
            instr->mnemonic,
             ReadCpuByte(mem,
                gAddrDecodedInfo.immediate.addr));
        break;
    case ZEROPAGE:
        printf("%02X %s $%02X = %02X",
            op1,
            instr->mnemonic,
            gAddrDecodedInfo.zeropage.addr,
             ReadCpuByte(mem,
                gAddrDecodedInfo.zeropage.addr));
        break;
    case ZEROPAGE_X:
        printf("%02X %s $%02X,X @ %02X = %02X",
            op1,
            instr->mnemonic,
            gAddrDecodedInfo.zeropageIndexed.zpAddr,
            (uint8_t)gAddrDecodedInfo.zeropageIndexed.addr,
            ReadCpuByte(mem,
                gAddrDecodedInfo.zeropageIndexed.addr));
        break;
    case ZEROPAGE_Y:
    printf("%02X %s $%02X,Y @ %02X = %02X",
            op1,
            instr->mnemonic,
            gAddrDecodedInfo.zeropageIndexed.zpAddr,
            (uint8_t)gAddrDecodedInfo.zeropageIndexed.addr,
            ReadCpuByte(mem,
                gAddrDecodedInfo.zeropageIndexed.addr));
        break;
    case RELATIVE: {
        uint8_t displacement = ReadCpuByte(mem,
                    gAddrDecodedInfo.relative.addr);
        // Probably need adjustements for "negative" displacements.
        printf("%02X %s $%04X",
            op1,
            instr->mnemonic,
            oldPc + 2 + displacement);
        }
        break;
    case ABSOLUTE:
        printf("%02X %02X %s $%04X",
            op1,
            op2,
            instr->mnemonic,
            gAddrDecodedInfo.absolute.addr);
        break;
    case ABSOLUTE_X:
    printf("%02X %02X %s $%04X,X @ %04X = %02X",
            op1,
            op2,
            instr->mnemonic,
            gAddrDecodedInfo.absoluteIndexed.AbsoluteAddr,
            gAddrDecodedInfo.absoluteIndexed.addr,
            ReadCpuByte(mem,
                gAddrDecodedInfo.absoluteIndexed.addr));
        break;
    case ABSOLUTE_Y:
        printf("%02X %02X %s $%04X,Y @ %04X = %02X",
            op1,
            op2,
            instr->mnemonic,
            gAddrDecodedInfo.absoluteIndexed.AbsoluteAddr,
            gAddrDecodedInfo.absoluteIndexed.addr,
            ReadCpuByte(mem,
                gAddrDecodedInfo.absoluteIndexed.addr));
        break;
    case INDIRECT:
        printf("%02X %02X %s ($%04X) = %04X",
            op1,
            op2,
            instr->mnemonic,
            gAddrDecodedInfo.indirect.addr,
            gAddrDecodedInfo.indirect.iaddr);
        break;
    case INDEXED_INDIRECT:
        printf("%02X %s ($%02X,X) @ %02X = %04X = %02X",
            op1,
            instr->mnemonic,
            gAddrDecodedInfo.indexedIndirect.zpAddr,
            gAddrDecodedInfo.indexedIndirect.zpXAddr,
            gAddrDecodedInfo.indexedIndirect.addr,
            ReadCpuByte(mem,
                gAddrDecodedInfo.indexedIndirect.addr));
        break;
    case INDIRECT_INDEXED:
        /*printf("%02X %s ($02X),Y = %04X @ %04X = %02X",
            op1,
            instr->mnemonic,
            g_addr_decoded_info.IndirectIndexed.zp_addr,
            g_addr_decoded_info.IndirectIndexed.);*/
        break;
    default:
        printf("%s", instr->mnemonic);
    }

    printf("\t");

    // Set original read flags values back.
    mem->ppustatusRead = ppustatusRead;
}

static void PrintStack(const Cpu *cpu) {
    printf("---STACK---");
    for (uint16_t i = 0xFF; i != cpu->regs.sp; --i) {
        if ((i + 1) % 16 == 0) {
            printf("\n%02X: ", i);
        }

        uint8_t byte = ReadCpuByte(cpu->mem, STACK_START + i);
        printf("%02X ", byte);
    }
    puts("\n-----------");
}

static void PrintRegisters(const Registers *regs) {
    printf("pc: %04X sp: %02X s: %02X "
        "x: %02X y: %02X a: %02x ",
        regs->pc, regs->sp, regs->s,
        regs->x, regs->y, regs->a);
}

static void PushStack(Cpu *cpu, uint8_t byte) {
    WriteCpuByte(cpu->mem, cpu->regs.sp-- + STACK_START, byte);
}

static uint8_t PopStack(Cpu *cpu) {
    return ReadCpuByte(cpu->mem, ++cpu->regs.sp + STACK_START);
}

static uint8_t CheckStatus(Cpu *cpu, Status s) {
    return (cpu->regs.s & s) != 0;
}

static void SetStatus(Cpu *cpu, Status s, uint8_t active) {
    if (active)
        cpu->regs.s |= s;
    else
        cpu->regs.s &= ~s;

    cpu->regs.s |= DEFAULT_STATUS_FLAG;
}

static void SetZnFlags(Cpu *cpu, uint8_t value) {
    SetStatus(cpu, NEGATIVE, value & NEGATIVE);
    SetStatus(cpu, ZERO, value == 0);	
}

void CpuRequestInterrupt(Cpu *cpu, Interrupt i) {
    if (i == NMI || !CheckStatus(cpu, INTERRUPT_DISABLE))
        cpu->interrupt |= i;
}

static void HandleInterrupt(Cpu *cpu) {
    uint16_t handler;

    if ((cpu->interrupt & RESET) != 0) {
        handler = RESET_INTERRUPT_VECTOR;
        cpu->interrupt &= ~RESET;
    } else if ((cpu->interrupt & NMI) != 0) {
        handler = NMI_INTERRUPT_VECTOR;
        cpu->interrupt &= ~NMI;
    } else if ((cpu->interrupt & IRQ) != 0) {
        handler = IRQ_INTERRUPT_VECTOR;
        cpu->interrupt &= ~IRQ;
    }

    PushStack(cpu, cpu->regs.pc >> 8);
    PushStack(cpu, cpu->regs.pc & 0xFF);
    PushStack(cpu, cpu->regs.s);

    uint8_t lo = ReadCpuByte(cpu->mem, handler);
    uint8_t hi = ReadCpuByte(cpu->mem, handler + 1);
    cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;

    SetStatus(cpu, INTERRUPT_DISABLE, 1);
    cpu->currentCycle = 0;
    cpu->cycles = CYCLES_AFTER_INTERRUPT;
}

static uint16_t Absolute(Cpu *cpu) {
    uint8_t lo = ReadCpuByte(cpu->mem, cpu->regs.pc++);
    uint8_t hi = ReadCpuByte(cpu->mem, cpu->regs.pc++);
    uint16_t addr = ((uint16_t)hi << 8) | (uint16_t)lo;

    gAddrDecodedInfo.absolute.addr = addr;

    return addr;
}

static uint16_t AbsoluteIndexed(Cpu *cpu, uint8_t index) {
    uint8_t lo = ReadCpuByte(cpu->mem, cpu->regs.pc++);
    uint8_t hi = ReadCpuByte(cpu->mem, cpu->regs.pc++);
    uint16_t absolute = ((uint16_t)hi << 8) | (uint16_t)lo;
    uint16_t addr = absolute + (uint16_t)index;
    
    if ((uint16_t)lo > (addr & 0xFF))
        printf("\npage crossed\n");

    gAddrDecodedInfo.absoluteIndexed.AbsoluteAddr = absolute;
    gAddrDecodedInfo.absoluteIndexed.index = index;
    gAddrDecodedInfo.absoluteIndexed.addr = addr;

    return addr;
}

static uint16_t Immediate(Cpu *cpu) {
    gAddrDecodedInfo.immediate.addr = cpu->regs.pc;

    return cpu->regs.pc++;
}

static uint16_t Zeropage(Cpu *cpu) {
    uint8_t addr = ReadCpuByte(cpu->mem, cpu->regs.pc++);

    gAddrDecodedInfo.zeropage.addr = addr;

    return (uint16_t)addr;
}

static uint16_t ZeropageIndexed(Cpu *cpu, uint8_t index) {
    uint8_t byte = ReadCpuByte(cpu->mem, cpu->regs.pc++);
    uint16_t addr = (byte + index) % 0xFF;
    
    gAddrDecodedInfo.zeropageIndexed.zpAddr = byte;
    gAddrDecodedInfo.zeropageIndexed.addr = addr;
    gAddrDecodedInfo.zeropageIndexed.index = index;

    return addr;
}

static uint16_t Relative(Cpu *cpu) {
    gAddrDecodedInfo.relative.addr = cpu->regs.pc;

    return cpu->regs.pc++;
}

static uint16_t Indirect(Cpu *cpu) {
    uint8_t lo = ReadCpuByte(cpu->mem, cpu->regs.pc++);
    uint8_t hi = ReadCpuByte(cpu->mem, cpu->regs.pc++);
    uint16_t addr = ((uint16_t)hi << 8) | (uint16_t)lo;

    lo = ReadCpuByte(cpu->mem, addr);
    hi = ReadCpuByte(cpu->mem, addr + 1);
    uint16_t iaddr = ((uint16_t)hi << 8) | (uint16_t)lo;

    gAddrDecodedInfo.indirect.addr = addr;
    gAddrDecodedInfo.indirect.iaddr = iaddr;

    return iaddr;
}

static uint16_t IndexedIndirect(Cpu *cpu) {
    uint8_t zp_addr = ReadCpuByte(cpu->mem, cpu->regs.pc++);
    uint8_t zp_x_addr = zp_addr + cpu->regs.x;

    uint8_t lo = ReadCpuByte(cpu->mem, zp_x_addr);
    uint8_t hi = ReadCpuByte(cpu->mem, zp_x_addr + 1);
    uint16_t addr = ((uint16_t)hi << 8) | (uint16_t)lo;

    gAddrDecodedInfo.indexedIndirect.zpAddr = zp_addr;
    gAddrDecodedInfo.indexedIndirect.zpXAddr = zp_x_addr;
    gAddrDecodedInfo.indexedIndirect.addr = addr;
    
    return addr;
}

// TODO: page cross 1+ cycle
static uint16_t IndirectIndexed(Cpu *cpu) {
    uint8_t zp_addr = ReadCpuByte(cpu->mem, cpu->regs.pc++);

    uint8_t lo = ReadCpuByte(cpu->mem, zp_addr);
    uint8_t hi = ReadCpuByte(cpu->mem, zp_addr + 1);
    uint16_t addr = (((uint16_t)hi << 8) | (uint16_t)lo) + (uint16_t)cpu->regs.y;
    
    if ((uint16_t)lo > (addr & 0xFF))
        printf("\npage crossed\n0");

    gAddrDecodedInfo.indirectIndexed.y = cpu->regs.y;
    gAddrDecodedInfo.indirectIndexed.zpAddr = zp_addr;
    gAddrDecodedInfo.indirectIndexed.addr = addr;

    return addr;
}

/* TODO: Unnoficial opcodes need to be implemented... for now let's try to work with this and try to get the ppu a start as well.
 * Work on a better way to print instructions on the screen. 
 */
uint8_t CpuEmulate(Cpu *cpu) {
    /* Is the current instruction in execution? */
    if (cpu->currentCycle < cpu->cycles) {
        ++cpu->currentCycle;
        return 0;
    }

    if (cpu->interrupt) {
        HandleInterrupt(cpu);
        return 0;
    }

    cpu->cycles = 0;
    cpu->currentCycle = 0;
    
    uint16_t oldPc = cpu->regs.pc;
    uint8_t opcode = ReadCpuByte(cpu->mem, cpu->regs.pc);
    uint8_t op1 = ReadCpuByte(cpu->mem, cpu->regs.pc + 1);
    uint8_t op2 = ReadCpuByte(cpu->mem, cpu->regs.pc + 2);

    const InstructionOrNothing *possibleInstr = &gInstructionTable[opcode];

    if (!possibleInstr->valid) {
        fprintf(stderr, "Invalid opcode %02x found\n", opcode);
        return 0;
    }

    const Instruction *instr = &possibleInstr->instr;
    assert(instr->opcode == opcode);

    ++cpu->regs.pc;

    uint16_t addr;
    switch (instr->adrMode) {
        case IMMEDIATE:
            addr = Immediate(cpu);
            break;
        case ZEROPAGE:
            addr = Zeropage(cpu);
            break;
        case ZEROPAGE_X:
            addr = ZeropageIndexed(cpu, cpu->regs.x);
            break;
        case ZEROPAGE_Y:
            addr = ZeropageIndexed(cpu, cpu->regs.y);
            break;
        case RELATIVE:
            addr = Relative(cpu);
            break;
        case ABSOLUTE:
            addr = Absolute(cpu);
            break;
        case ABSOLUTE_X:
            addr = AbsoluteIndexed(cpu, cpu->regs.x);
            break;
        case ABSOLUTE_Y:
            addr = AbsoluteIndexed(cpu, cpu->regs.y);
            break;
        case INDIRECT:
            addr = Indirect(cpu);
            break;
        case INDEXED_INDIRECT:
            addr = IndexedIndirect(cpu);
            break;
        case INDIRECT_INDEXED:
            addr = IndirectIndexed(cpu);
            break;
        default:
            addr = 0;
    }

    DebugInstruction(cpu->mem, oldPc, opcode, op1, op2, instr);
    
    cpu->cycles = instr->cycles;
    if (instr->execute)
        instr->execute(cpu, addr);

    PrintRegisters(&cpu->regs);

    /* Finished an instruction. */
    *(cpu->totalCycles) += cpu->cycles;
    return 1;
}

void CpuInit(Cpu *cpu, Memory *mem, uint64_t *totalCycles) {
    Registers regs;

    regs.pc = 0x0000;
    regs.sp = 0xFF;
    regs.a  = 0x00;
    regs.x  = 0x00;
    regs.y  = 0x00;
    regs.s  = 0x20;

    cpu->regs = regs;
    cpu->mem = mem;
    cpu->interrupt = RESET;
    cpu->cycles = 0;
    cpu->currentCycle = 0;
    cpu->totalCycles = totalCycles;
}

INSTR(Brk) {
    (void)addr;

    PushStack(cpu, cpu->regs.pc >> 8);
    PushStack(cpu, cpu->regs.pc & 0xFF);
    PushStack(cpu, cpu->regs.s | BREAK);

    uint8_t lo = ReadCpuByte(cpu->mem, IRQ_INTERRUPT_VECTOR);
    uint8_t hi = ReadCpuByte(cpu->mem, IRQ_INTERRUPT_VECTOR + 1);
    cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;

    cpu->cycles += 7;
}

INSTR(Ora) {
    cpu->regs.a |= ReadCpuByte(cpu->mem, addr);
    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Asl) {
    uint8_t byte = ReadCpuByte(cpu->mem, addr);
    
    SetStatus(cpu, CARRY, byte & NEGATIVE);

    byte <<= 1;

    SetZnFlags(cpu, byte);
    WriteCpuByte(cpu->mem, addr, byte);
}

INSTR(AslA) {
    (void)addr;

    SetStatus(cpu, CARRY, cpu->regs.a & NEGATIVE);
    cpu->regs.a <<= 1;
    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Php) {
    (void)addr;
    PushStack(cpu, cpu->regs.s);
}

INSTR(Bpl) {
    Branch(cpu, addr, NEGATIVE, 0);
}

INSTR(Clc) {
    (void)addr;
    SetStatus(cpu, CARRY, 0);
}

INSTR(Jsr) {
    uint16_t tmp = cpu->regs.pc - 1;
        
    PushStack(cpu, tmp >> 8);
    PushStack(cpu, tmp & 0xFF);
    cpu->regs.pc = addr;
}

INSTR(And) {
    cpu->regs.a &= ReadCpuByte(cpu->mem, addr);
    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Bit) {
    uint8_t byte = ReadCpuByte(cpu->mem, addr);

    SetStatus(cpu, ZERO, (byte & cpu->regs.a) == 0);
    SetStatus(cpu, OVERFLOW, byte & OVERFLOW);
    SetStatus(cpu, NEGATIVE, byte & NEGATIVE);
}

INSTR(Rol) {
    uint8_t byte, carry;

    byte = ReadCpuByte(cpu->mem, addr);
    carry = CheckStatus(cpu, CARRY);

    SetStatus(cpu, CARRY, byte & NEGATIVE);
    byte = (byte << 1) | carry;

    SetZnFlags(cpu, byte);
    WriteCpuByte(cpu->mem, addr, byte);
}

INSTR(RolA) {
    (void)addr;

    uint8_t carry = CheckStatus(cpu, CARRY);

    SetStatus(cpu, CARRY, cpu->regs.a & NEGATIVE);
    cpu->regs.a = (cpu->regs.a << 1) | carry;

    SetZnFlags(cpu, cpu->regs.a);
    cpu->cycles += 2;
}

INSTR(Plp) {
    (void)addr;
    cpu->regs.s = PopStack(cpu);
}

INSTR(Bmi) {
    Branch(cpu, addr, NEGATIVE, 1);
}

INSTR(Sec) {
    (void)addr;
    SetStatus(cpu, CARRY, 1);
}

INSTR(Rti) {
    (void)addr;

    cpu->regs.s = PopStack(cpu) & ~BREAK;
    uint8_t lo = PopStack(cpu);
    uint8_t hi = PopStack(cpu);
    cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;
}

INSTR(Eor) {
    cpu->regs.a ^= ReadCpuByte(cpu->mem, addr);
    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Lsr) {
    uint8_t byte = ReadCpuByte(cpu->mem, addr);

    SetStatus(cpu, CARRY, byte & CARRY);

    byte >>= 1;

    SetZnFlags(cpu, byte);
    WriteCpuByte(cpu->mem, addr, byte);
}

INSTR(LsrA) {
    (void)addr;

    SetStatus(cpu, CARRY, cpu->regs.a & CARRY);

    cpu->regs.a >>= 1;

    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Pha) {
    (void)addr;

    PushStack(cpu, cpu->regs.a);
}

INSTR(Jmp) {
    // TODO: Maybe wrong?
    cpu->regs.pc = addr;
}

INSTR(Bvc) {
    Branch(cpu, addr, OVERFLOW, 0);
}

INSTR(Cli) {
    (void)addr;
    SetStatus(cpu, INTERRUPT_DISABLE, 0);
}

INSTR(Rts) {
    (void)addr;

    uint8_t lo = PopStack(cpu);
    uint8_t hi = PopStack(cpu);
    cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;
    // TODO: Since we save pc - 1, we return to the wrong position. Maybe save pc? or just keep adding 1...
    cpu->regs.pc += 1;
}

INSTR(Adc) {
    AdcImpl(cpu, addr, 0);
}

INSTR(Ror) {
    uint8_t byte, carry;

    byte = ReadCpuByte(cpu->mem, addr);
    carry = CheckStatus(cpu, CARRY);

    SetStatus(cpu, CARRY, byte & CARRY);
    byte = (byte >> 1) | (carry << 7);

    SetZnFlags(cpu, byte);
    WriteCpuByte(cpu->mem, addr, byte);
}

INSTR(RorA) {
    (void)addr;

    uint8_t carry;
    
    carry = CheckStatus(cpu, CARRY);

    SetStatus(cpu, CARRY, cpu->regs.a & CARRY);
    cpu->regs.a = (cpu->regs.a >> 1) | (carry << 7);

    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Pla) {
    (void)addr;

    cpu->regs.a = PopStack(cpu);
    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Bvs) {
    Branch(cpu, addr, OVERFLOW, 1);
}

INSTR(Sei) {
    (void)addr;
    SetStatus(cpu, INTERRUPT_DISABLE, 1);
}

INSTR(Sta) {
    WriteCpuByte(cpu->mem, addr, cpu->regs.a);
}

INSTR(Sty) {
    WriteCpuByte(cpu->mem, addr, cpu->regs.y);
}

INSTR(Stx) {
    WriteCpuByte(cpu->mem, addr, cpu->regs.x);
}

INSTR(Dey) {
    (void)addr;

    --cpu->regs.y;
    SetZnFlags(cpu, cpu->regs.y);
}

INSTR(Txa) {
    (void)addr;

    cpu->regs.a = cpu->regs.x;
    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Bcc) {
    Branch(cpu, addr, CARRY, 0);	
}

INSTR(Tya) {
    (void)addr;

    cpu->regs.a = cpu->regs.y;
    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Txs) {
    (void)addr;

    cpu->regs.sp = cpu->regs.x;
}

INSTR(Ldy) {
    cpu->regs.y = ReadCpuByte(cpu->mem, addr);
    SetZnFlags(cpu, cpu->regs.y);
}

INSTR(Lda) {
    cpu->regs.a = ReadCpuByte(cpu->mem, addr);
    SetZnFlags(cpu, cpu->regs.a);
}

INSTR(Ldx) {
    cpu->regs.x = ReadCpuByte(cpu->mem, addr);
    SetZnFlags(cpu, cpu->regs.x);
}

INSTR(Tay) {
    (void)addr;

    cpu->regs.y = cpu->regs.a;
    SetZnFlags(cpu, cpu->regs.y);
}

INSTR(Tax) {
    (void)addr;

    cpu->regs.x = cpu->regs.a;
    SetZnFlags(cpu, cpu->regs.x);
}

INSTR(Bcs) {
    Branch(cpu, addr, CARRY, 1);
}

INSTR(Clv) {
    (void)addr;

    SetStatus(cpu, OVERFLOW, 0);
}

INSTR(Tsx) {
    (void)addr;

    cpu->regs.x = cpu->regs.sp;
    SetZnFlags(cpu, cpu->regs.x);
}

INSTR(Cpy) {
    CmpImpl(cpu, addr, cpu->regs.y);
}

INSTR(Cmp) {
    CmpImpl(cpu, addr, cpu->regs.a);
}

INSTR(Dec) {
    IncDecImpl(cpu, addr, 0xFF);
}

INSTR(Iny) {
    (void)addr;

    ++cpu->regs.y;
    SetZnFlags(cpu, cpu->regs.y);
}

INSTR(Dex) {
    (void)addr;

    --cpu->regs.x;
    SetZnFlags(cpu, cpu->regs.x);
}

INSTR(Bne) {
    Branch(cpu, addr, ZERO, 0);
}

INSTR(Cld) {
    (void)addr;

    SetStatus(cpu, DECIMAL, 0);
}

INSTR(Cpx) {
    CmpImpl(cpu, addr, cpu->regs.x);
}

INSTR(Sbc) {
    AdcImpl(cpu, addr, 1);
}

INSTR(Inc) {
    IncDecImpl(cpu, addr, 0x01);
}

INSTR(Inx) {
    (void)addr;

    ++cpu->regs.x;
    SetZnFlags(cpu, cpu->regs.x);
}

INSTR(Beq) {
    Branch(cpu, addr, ZERO, 1);
}

INSTR(Sed) {
    (void)addr;
    
    SetStatus(cpu, DECIMAL, 1);
}

static void IncDecImpl(Cpu *cpu, uint16_t addr, uint8_t change) {
    uint8_t byte = ReadCpuByte(cpu->mem, addr) + change;
    
    WriteCpuByte(cpu->mem, addr, byte);

    SetZnFlags(cpu, byte);
}

static void CmpImpl(Cpu *cpu, uint16_t addr, uint8_t reg) {
    uint8_t byte = ReadCpuByte(cpu->mem, addr);

    SetStatus(cpu, CARRY, reg >= byte);
    SetStatus(cpu, ZERO, reg == byte);
    SetStatus(cpu, NEGATIVE, ((reg - byte) & NEGATIVE) != 0);
}

static void AdcImpl(Cpu *cpu, uint16_t addr, uint8_t onesComplement) {	
    uint8_t byte, a, overflow, carry;

    byte = ReadCpuByte(cpu->mem, addr);

    if (onesComplement)
        byte = ~byte;

    a = cpu->regs.a + byte + CheckStatus(cpu, CARRY);
    overflow = (byte & 0x80) == (cpu->regs.a & 0x80) && (byte & 0x80) != (a & 0x80);
    carry = (((uint16_t)cpu->regs.a + (uint16_t)byte) & 0xFF00) != 0;

    SetStatus(cpu, OVERFLOW, overflow);
    SetZnFlags(cpu, a);
    SetStatus(cpu, CARRY, carry);

    cpu->regs.a = a;
}

static void Branch(Cpu *cpu, uint16_t addr, Status status,
           uint8_t valueNeeded) {
    uint8_t displacement, oldLo;

    displacement = ReadCpuByte(cpu->mem, addr);

    if (CheckStatus(cpu, status) == valueNeeded) {
        oldLo = (uint8_t)(cpu->regs.pc & 0xFF);
        /* TODO: Maybe fix? displacement seems to work for now. */
        if (displacement & NEGATIVE)
            cpu->regs.pc += (int8_t)displacement;
        else
            cpu->regs.pc += displacement;

        if (oldLo != (uint8_t)(cpu->regs.pc & 0xFF))
            cpu->cycles += 2;
        else
            cpu->cycles += 1;
    }
}
