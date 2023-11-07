#include "globals.hpp"
#define MEM_SIZE 4096
const int BREG_SIZE = 32; 
int32_t breg[BREG_SIZE];
int32_t mem[MEM_SIZE];
bool stop_prg;

uint32_t pc, ri;			

string reg_str[] = {
		"ZERO", "RA", "SP", "GP",
		"TP", "T0", "T1", "T2",
		"S0", "S1", "A0", "A1",
		"A2", "A3", "A4", "A5",
		"A6", "A7", "S2", "S3",
		"S4", "S5", "S6", "S7",
		"S8", "S9", "S10", "S11",
		"T3", "T4", "T5", "T6"};

uint32_t opcode, rs1, rs2, rd, shamt, funct3, funct7;			

int32_t imm12_i, imm12_s, imm13, imm20_u, imm21;	

int32_t lw(uint32_t address, int32_t kte)
{
	if ((address + kte) % 4 != 0)
	{
		printf("Inválido!\n");
		return 0;
	}
	int32_t indice = (kte + address) >> 2;
	return mem[indice];
}
int32_t lb(uint32_t address, int32_t kte)
{
	int8_t *pointer = (int8_t *)mem;
	return (int32_t) * (pointer + address + kte);
}
int32_t lbu(uint32_t address, int32_t kte)
{
	int8_t *pointer = (int8_t *)mem;
	return (0x00FF) & ((int8_t) * (pointer + address + kte));
}
void sw(uint32_t address, int32_t kte, int32_t dado)
{
	if ((address + kte) % 4 != 0)
	{
		printf("Invalido!\n");
		return;
	}
	int32_t indice = (kte + address) >> 2;
	mem[indice] = dado;
}
void sb(uint32_t address, int32_t kte, int8_t dado)
{
	int8_t *pointer = (int8_t *)mem;
	*(pointer + address + kte) = dado;
}

void decode(instruction_context_st &ic)
{
	int32_t tmp;
	opcode = ri & 0x7F;
	rs2 = (ri >> 20) & 0x1F;
	rs1 = (ri >> 15) & 0x1F;
	rd = (ri >> 7) & 0x1F;
	shamt = (ri >> 20) & 0x1F;
	funct3 = (ri >> 12) & 0x7;
	funct7 = (ri >> 25);
	imm12_i = ((int32_t)ri) >> 20;
	tmp = get_field(ri, 7, 0x1f);
	imm12_s = set_field(imm12_i, 0, 0x1f, tmp);
	imm13 = imm12_s;
	imm13 = set_bit(imm13, 11, imm12_s & 1);
	imm13 = imm13 & ~1;
	imm20_u = ri & (~0xFFF);
	
	imm21 = (int32_t)ri >> 11;							 
	tmp = get_field(ri, 12, 0xFF);					
	imm21 = set_field(imm21, 12, 0xFF, tmp); 
	tmp = get_bit(ri, 20);									 
	imm21 = set_bit(imm21, 11, tmp);				 
	tmp = get_field(ri, 21, 0x3FF);
	imm21 = set_field(imm21, 1, 0x3FF, tmp);
	imm21 = imm21 & ~1; // zero bit 0
	ic.ins_code = get_instr_code(opcode, funct3, funct7);
	ic.rs1 = (REGISTERS)rs1;
	ic.rs2 = (REGISTERS)rs2;
	ic.rd = (REGISTERS)rd;
	ic.shamt = shamt;
	ic.imm12_i = imm12_i;
	ic.imm12_s = imm12_s;
	ic.imm13 = imm13;
	ic.imm21 = imm21;
	ic.imm20_u = imm20_u;
}

void load_mem(const char *fn, int start)
{
	FILE *fp;
	int i;
	fp = fopen(fn, "r");
	for (i = start / 4; i < MEM_SIZE; i++)
	{
		if (fread(&mem[i], sizeof(int32_t), 1, fp) != 1)
		{
			break;
		};
	}
	fclose(fp);
}
void init()
{
	pc = 0x00000000;
	ri = 0x00000000;
	breg[SP] = 0x00003ffc;
	breg[GP] = 0x00001800;
	stop_prg = false;
}
void fetch(instruction_context_st &ic)
{
	ri = lw(pc, 0);
	ic.ir = ri;
	ic.pc = pc;
	pc += 4;
}
void step()
{
	instruction_context_st ic;
	fetch(ic);
	decode(ic);
	execute(ic);
}
void run()
{
	while ((pc < DATA_SEGMENT_START) && !stop_prg)
	{
		step();
	}
}
INSTRUCTIONS get_instr_code(uint32_t opcode, uint32_t funct3, uint32_t funct7)
{
    switch (opcode)
    {
    case LUI:
        return I_lui;
    case AUIPC:
        return I_auipc;
    case BType:
        switch (funct3)
        {
        case BEQ3:
            return I_beq;
        case BNE3:
            return I_bne;
        case BLT3:
            return I_blt;
        case BGE3:
            return I_bge;
        case BLTU3:
            return I_bltu;
        case BGEU3:
            return I_bgeu;
        default:
            break;
        }
        break;
    case ILType:
        switch (funct3)
        {
        case LB3:
            return I_lb;
        case LW3:
            return I_lw;
        case LBU3:
            return I_lbu;
        default:
            break;
        }
    case JAL:
        return I_jal;
    case JALR:
        return I_jalr;
    case StoreType:
        switch (funct3)
        {
        case SB3:
            return I_sb;
        case SW3:
            return I_sw;
        default:
            break;
        }
        break;
    case ILAType:
        switch (funct3)
        {
        case ADDI3:
            return I_addi;
        case ORI3:
            return I_ori;
        case ANDI3:
            return I_andi;
        case SLLI3:
            return I_slli;
        case SRI3:
            if (funct7 == SRLI7)
                return I_srli;
            else
                return I_srai;
        default:
            break;
        }
        break;
    case RegType:
        switch (funct3)
        {
        case ADDSUB3:
            if (funct7 == SUB7)
                return I_sub;
            else
                return I_add;
        case SLT3:
            return I_slt;
        case SLTU3:
            return I_sltu;
        case XOR3:
            return I_xor;
        case OR3:
            return I_or;
        case AND3:
            return I_and;
        default:
            break;
        }
        break;
    case ECALL:
        return I_ecall;
    default:
        break;
    }
    return I_nop;
}
void execute(instruction_context_st &ic)
{
    int8_t *first;
    breg[ZERO] = 0;
    switch (ic.ins_code)
    {
    case I_add:
        breg[ic.rd] = breg[ic.rs1] + breg[ic.rs2];
        break;
    case I_addi:
        breg[ic.rd] = breg[ic.rs1] + ic.imm12_i;
        break;
    case I_and:
        breg[ic.rd] = breg[ic.rs1] & breg[ic.rs2];
        break;
    case I_andi:
        breg[ic.rd] = breg[ic.rs1] & ic.imm12_i;
        break;
    case I_auipc:
        breg[ic.rd] = ic.imm20_u + ic.pc;
        break;
    case I_beq:
        if (breg[ic.rs1] == breg[ic.rs2])
        {
            pc = ic.pc + ic.imm13;
        }
        break;
    case I_bge:
        if (breg[ic.rs1] >= breg[ic.rs2])
        {
            pc = ic.pc + ic.imm13;
        }
        break;
    case I_bgeu:
        if ((uint32_t)breg[ic.rs1] >= (uint32_t)breg[ic.rs2])
        {
            pc = ic.pc + ic.imm13;
        }
        break;
    case I_blt:
        if (breg[ic.rs1] < breg[ic.rs2])
        {
            pc = ic.pc + ic.imm13;
        }
        break;
    case I_bltu:
        if ((uint32_t)breg[ic.rs1] < (uint32_t)breg[ic.rs2])
        {
            pc = ic.pc + ic.imm13;
        }
        break;
    case I_bne:
        if (breg[ic.rs1] != breg[ic.rs2])
        {
            pc = ic.pc + ic.imm13;
        }
        break;
    case I_jal:
        breg[ic.rd] = ic.pc + 4;
        pc = ic.pc + ic.imm21;
        break;
    case I_jalr:
        breg[ic.rd] = ic.pc + 4;
        pc = (breg[ic.rs1] + ic.imm12_i) & ~1;
        break;
    case I_lb:
        breg[ic.rd] = lb(breg[ic.rs1], ic.imm12_i);
        break;
    case I_lbu:
        breg[ic.rd] = lbu(breg[ic.rs1], ic.imm12_i);
        break;
    case I_lw:
        breg[ic.rd] = lw(breg[ic.rs1], ic.imm12_i);
        break;
    case I_lui:
        breg[ic.rd] = ic.imm20_u;
        break;
    case I_sb:
        sb(breg[ic.rs1], ic.imm12_s, (int8_t)breg[ic.rs2]);
        break;
    case I_sw:
        sw(breg[ic.rs1], ic.imm12_s, (int32_t)breg[ic.rs2]);
        break;
    case I_slli:
        breg[ic.rd] = breg[ic.rs1] << ic.shamt;
        break;
    case I_slt:
        breg[ic.rd] = breg[ic.rs1] < breg[ic.rs2];
        break;
    case I_sub:
        breg[ic.rd] = breg[ic.rs1] - breg[ic.rs2];
        break;
    case I_xor:
        breg[ic.rd] = breg[ic.rs1] ^ breg[ic.rs2];
        break;
    case I_or:
        breg[ic.rd] = breg[ic.rs1] | breg[ic.rs2];
        break;
    case I_srli:
        breg[ic.rd] = (uint32_t)breg[ic.rs1] >> ic.shamt;
        break;
    case I_srai:
        breg[ic.rd] = breg[ic.rs1] >> ic.shamt;
        break;
    case I_sltu:
        breg[ic.rd] = (uint32_t)breg[ic.rs1] < (uint32_t)breg[ic.rs2];
        break;
    case I_ori:
        breg[ic.rd] = breg[ic.rs1] | ic.imm12_i;
        break;
    case I_ecall:
        switch (breg[A7])
        {
        case 1:
            printf("%d", breg[A0]);
            break;
        case 4:
            first = (int8_t *)mem + breg[A0];
            printf("%s", first);
            break;
        case 10:
            stop_prg = true;
            break;
        }
    case I_nop:
        break;
    default:
        if (ic.ir == 0)
        {
            stop_prg = true;
        }
        break;
    }
}

void dump_mem(int start_byte, int end_byte, char format)
{
	int i;
	for (i = start_byte; i < end_byte; i+=4)
	{
		if (format == 'x')
			printf("%08x", lw(i, 0));
		else if (format == 'h')
		{
			printf("%08x", lw(i, 0));
		}
		else
			printf("Invalido");
		printf("\n");
	}
}
void dump_breg(char format)
{
	int i;
	for (i = 0; i < BREG_SIZE; i++)
	{
		if (format == 'h')
			printf("BREG %s: %04x", reg_str[i].c_str(), breg[i]);
		else if (format == 'd')
		{
			printf("BREG %s: %08x", reg_str[i].c_str(), breg[i]);
		}
		else
			printf("Invalido");
		printf("\n");
	}
}
