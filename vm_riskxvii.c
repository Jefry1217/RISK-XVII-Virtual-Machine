#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "structures.h"

// allocating data memory virtual memory
char memory[2048];

// setting program counter
int program_counter = 0;

// allocating heap banks
char heap_banks[8192] = {0};

node create_node(int len) {
    node temp;
    temp = (node)malloc(sizeof(struct linked_list));
    temp->len = len;
    temp->next = NULL;
    return temp;
}
int allocate_memory(node head, int size) {
    node temp;
    temp = create_node(size);
    int cur_blocks = 0;
    int blocks_needed = 0;
    int blocks_between = 0;
    while(head->next) {
        cur_blocks = (int)ceil(head->len / 64.0);
        blocks_between = ((head->next->start) - (head->start + (cur_blocks * 64))) / 64;
        blocks_needed = (int)ceil(size / 64.0);
        if(blocks_between >= blocks_needed) {
            temp->next = head->next;
            temp->start = head->start + cur_blocks * 64;
            head->next = temp;
            return temp->start;
        }
        head = head->next;
    }
    if((0xD700 - (head->start + ((int)ceil(head->len / 64.0) * 64))) >= size) {
        temp->start = head->start + ((int)ceil(head->len / 64.0) * 64);
        head->next = temp;
        return temp->start;
    }
    free(temp);
    return 0;
}
void free_list(node head) {
    node next = head->next;
    while(next) {
        free(head);
        head = next;
        next = head->next;
    }
}
int is_allocated(node head, int start) {
    head = head->next;
    while(head) {
        if((start >= head->start) && (start < (head->start + head->len))) {
            return 1;
        }
        head = head->next;
    }
    return 0; 
}
int free_memory(node head, int start) {
    node previous = head;
    head = head->next;
    while(head) {
        if(head->start == start) {
            previous->next = head->next;
            free(head);
            return 1;
        }
        previous = head;
        head = head->next;
    }
    return 0;
}
void dump_registers(struct registers * regs) {
    printf("PC = 0x%08x;\n", program_counter);
    for(int i = 0; i < 32; i++) {
        printf("R[%d] = 0x%08x;\n", i, regs[i].values.imm);
    }   
}
void raise_error(char * error, unsigned int * cmd, struct registers * regs, node head) {
    printf("%s: 0x%x\n", error, *cmd);
    dump_registers(regs);
    free_list(head);
}

int main(int argc, char **argv) {

    // allocating memory for registers
    struct registers regs[32];
    for(int i = 0; i < 32; i++) {
        regs[i].values.imm = 0;
    }

    // creating head of linked list to allocate heap banks memory
    node head = create_node(0);
    head->start = 0xb700;

    // making file pointer
    FILE * fptr;

    // opening file and error checking int exists
    if ((fptr = fopen(argv[1], "rb")) == NULL) {
        puts("File doesn't exist");
        return 1;
    }

    //going to image of data memory section of file and copying into virtual memory
    fseek(fptr, 0, SEEK_SET);
    for(int i = 0; i < 2048; i++) {
        fread(&memory[i], 1, 1, fptr);
    }

    // input from file
    unsigned int cmd;
    
    // looping through file and executing commands
    while(1) { 
        fseek(fptr, program_counter, SEEK_SET);
        fread(&cmd, 4, 1, fptr);

        // setting opcode 
        char opcode = cmd & 0x7F;

        // geting type input based on opcode
        switch(opcode) {

            case 0x33: ;
                // making structure type r
                struct type_r rinfo = {
                    (cmd & 0xF80) >> 7, // rd
                    (cmd & 0x7000) >> 12, // func3
                    (cmd & 0xF8000) >> 15, // rs1
                    (cmd & 0x1F00000) >> 20, // rs2
                    (cmd & 0xFE000000) >> 25 // func7
                };

                switch(rinfo.func7) {
                    case 0x0:
                        switch(rinfo.func3) {
                            // sub operation
                            case 0x0:
                                regs[rinfo.rd].values.imm = regs[rinfo.rs1].values.imm + regs[rinfo.rs2].values.imm;
                                break;
                            // slt operation
                            case 0x2:
                                regs[rinfo.rd].values.imm = (regs[rinfo.rs1].values.imm < regs[rinfo.rs2].values.imm) ? 1 : 0;
                                break;
                            // sltu operation
                            case 0x3:
                                regs[rinfo.rd].values.imm = ((unsigned int)regs[rinfo.rs1].values.imm < (unsigned int)regs[rinfo.rs2].values.imm) ? 1 : 0;
                                break;
                            // xor operation
                            case 0x4: 
                                regs[rinfo.rd].values.imm = regs[rinfo.rs1].values.imm ^ regs[rinfo.rs2].values.imm;
                                break;
                            // or operation
                            case 0x6:
                                regs[rinfo.rd].values.imm = regs[rinfo.rs1].values.imm | regs[rinfo.rs2].values.imm;
                                break;
                            // and operation
                            case 0x7:
                                regs[rinfo.rd].values.imm = regs[rinfo.rs1].values.imm & regs[rinfo.rs2].values.imm;
                                break;
                            // sll operation
                            case 0x1:
                                regs[rinfo.rd].values.imm = regs[rinfo.rs1].values.imm << regs[rinfo.rs2].values.imm; 
                                break;
                            // srl operation
                            case 0x5:
                                regs[rinfo.rd].values.imm = (unsigned int)regs[rinfo.rs1].values.imm >> regs[rinfo.rs2].values.imm; 
                                break;
                            default:
                                raise_error("Instruction Not Implemented", &cmd, regs, head);
                                return 1;
                        }
                        break;
                    case 0x20:
                        switch(rinfo.func3) {
                            // sub operation
                            case 0x0:
                                regs[rinfo.rd].values.imm = regs[rinfo.rs1].values.imm - regs[rinfo.rs2].values.imm;
                                break;
                            // sra operation
                            case 0x5: ;
                                int shift_n = regs[rinfo.rs2].values.imm % 32;
                                int result = 0;
                                int new_pos;
                                int value = 0;
                                for(int i = 0; i < 32; i++) {
                                    value = regs[rinfo.rs1].values.imm & (int)pow(2, i);
                                    if(value) {
                                        new_pos = i - shift_n;
                                        if(new_pos < 0) {
                                            new_pos += 32;
                                        }
                                        result = result | (int)pow(2, new_pos);
                                    }
                                }
                                regs[rinfo.rd].values.imm = result;
                                break;
                            default:
                                raise_error("Instruction Not Implemented", &cmd, regs, head);
                                return 1;
                        }
                        break;
                    default:
                        raise_error("Instruction Not Implemented", &cmd, regs, head);
                        return 1;
                }
                break;

            case 0x13:
            case 0x67:
            case 0x3: ;

                // making structure type i
                struct type_i iinfo = {
                    (cmd & 0xF80) >> 7, // rd
                    (cmd & 0x7000) >> 12, // func3
                    (cmd & 0xF8000) >> 15, // rs1
                    (int)(cmd & 0xFFF00000) >> 20 // imm
                };
                switch(opcode) {
                    case 0x13:
                        switch(iinfo.func3) {
                            // addi command
                            case 0x0:
                                regs[iinfo.rd].values.imm = regs[iinfo.rs1].values.imm + iinfo.imm;
                                break;
                            // slti command
                            case 0x2:
                                regs[iinfo.rd].values.imm = (regs[iinfo.rs1].values.imm < iinfo.imm) ? 1 : 0;
                                break;
                            // sltiu
                            case 0x3:
                                regs[iinfo.rd].values.imm = ((unsigned int)regs[iinfo.rs1].values.imm < (unsigned int)iinfo.imm) ? 1 : 0;
                                break;
                            // xori command
                            case 0x4:
                                regs[iinfo.rd].values.imm = regs[iinfo.rs1].values.imm ^ iinfo.imm;
                                break;
                            // andi
                            case 0x7:
                                regs[iinfo.rd].values.imm = regs[iinfo.rs1].values.imm & iinfo.imm;
                                break;
                            // ori
                            case 0x6:
                                regs[iinfo.rd].values.imm = regs[iinfo.rs1].values.imm | iinfo.imm;
                                break;
                            default:
                                raise_error("Instruction Not Implemented", &cmd, regs, head);
                                return 1;raise_error("Instruction Not Implemented", &cmd, regs, head);
                        }
                        break; 
                    // jalr command
                    case 0x67:
                        regs[iinfo.rd].values.imm = program_counter + 4;
                        program_counter = regs[iinfo.rs1].values.imm + iinfo.imm - 4;
                        break;
                    case 0x3: ;
                        // the memory address
                        int mem_address = regs[iinfo.rs1].values.imm + iinfo.imm;
                        switch(mem_address) {
                            // console read character
                            case 0x0812: ;
                                char value8;
                                scanf("%c", &value8);
                                regs[iinfo.rd].values.chars[0] = value8;
                                for(int i = 1; i < 4; i++) {
                                    regs[iinfo.rd].values.chars[i] = 0x0;
                                }
                                break;
                            // console read signed integer
                            case 0x0816: ;
                                int value16;
                                scanf("%d", &value16);
                                regs[iinfo.rd].values.imm = value16;
                                break;
                            default:
                                if((mem_address < 0 || mem_address > 0x8ff) && !is_allocated(head, mem_address)) {
                                    raise_error("Illegal Operation", &cmd, regs, head);
                                    return 1;
                                }
                                switch(iinfo.func3) {
                                    // lb command
                                    case 0x0:
                                        regs[iinfo.rd].values.imm = memory[mem_address];
                                        break;
                                    // lh command
                                    case 0x1:
                                        regs[iinfo.rd].values.imm = ((int)(memory[mem_address + 1] << 8)) | ((unsigned char)memory[mem_address]);
                                        break;
                                    // lw command
                                    case 0x2:
                                        for(int i = 0; i < 4; i++) {
                                            regs[iinfo.rd].values.chars[i] = memory[mem_address + i];
                                        }
                                        break;
                                    // lbu command
                                    case 0x4:
                                        regs[iinfo.rd].values.chars[0] = memory[mem_address];
                                        for(int i = 1; i < 4; i++) {
                                            regs[iinfo.rd].values.chars[i] = 0x0;
                                        } 
                                        break;
                                    // lhu command
                                    case 0x5:
                                        for(int i = 0; i < 2; i++) {
                                            regs[iinfo.rd].values.chars[i] = memory[mem_address + i];
                                        }
                                        for(int i = 2; i < 4; i++) {
                                            regs[iinfo.rd].values.chars[i] = 0x0;
                                        }
                                        break;
                                    default:
                                        raise_error("Instruction Not Implemented", &cmd, regs, head);
                                        return 1;
                                }
                                break;
                        }
                        break;
                    default:
                        raise_error("Instruction Not Implemented", &cmd, regs, head);
                        return 1;
                }
                break;

            case 0x23: ;
                // making structure type s
                struct type_s sinfo = {
                    (cmd & 0x7000) >> 12, // func3
                    (cmd & 0xF8000) >> 15, // rs1
                    (cmd & 0x1F00000) >> 20, // rs2
                    ((int)(cmd & 0xFE000000) >> 20) | ((cmd & 0xF80) >> 7), // imm
                };

                char value8 = regs[sinfo.rs2].values.chars[0];
                short value16 = (short)regs[sinfo.rs2].values.chars[1] << 8 | (unsigned char)regs[sinfo.rs2].values.chars[0];
                int value32 = regs[sinfo.rs2].values.imm;

                // memory address to store value into
                int mem_address = regs[sinfo.rs1].values.imm + sinfo.imm;

                // storing values to memory points
                if((mem_address >= 0 && mem_address < 0x0800)) {
                    switch(sinfo.func3) {
                        // sb command
                        case 0x0:
                            memory[mem_address] = value8;
                            break;
                        // sh command
                        case 0x1:
                            memory[mem_address] = regs[sinfo.rs2].values.chars[0];
                            memory[mem_address + 1] = regs[sinfo.rs2].values.chars[1];
                            break;
                        // sw command
                        case 0x2:
                            for(int i = 0; i < 4; i++) {
                                memory[mem_address + i] = regs[sinfo.rs2].values.chars[i];
                            }
                            break;
                        default:
                            raise_error("Instruction Not Implemented", &cmd, regs, head);
                            return 1;
                    }
                }
                else if(is_allocated(head, mem_address)) {
                    switch(sinfo.func3) {
                        // sb command
                        case 0x0:
                            heap_banks[mem_address - 0xb700] = value8;
                            break;
                        // sh command
                        case 0x1:
                            heap_banks[mem_address - 0xb700] = regs[sinfo.rs2].values.chars[0];
                            heap_banks[mem_address - 0xb700 + 1] = regs[sinfo.rs2].values.chars[1];
                            break;
                        // sw command
                        case 0x2:
                            for(int i = 0; i < 4; i++) {
                                heap_banks[mem_address - 0xb700 + i] = (unsigned char)regs[sinfo.rs2].values.chars[i];
                            }
                            break;
                        default:
                            raise_error("Instruction Not Implemented", &cmd, regs, head);
                            return 1;
                    }
                } 
                // memory store commands to specific memory points - will run virtual routines
                else {
                    switch(mem_address) {
                        // console write character
                        case 0x0800:
                            putchar(value8);
                            break;
                        // console write signed integer
                        case 0x0804:
                            switch(sinfo.func3) {
                                case 0x0: 
                                    printf("%d", value8);
                                    break;
                                case 0x1:
                                    printf("%d", value16);
                                    break;
                                case 0x2:
                                    printf("%d", value32);
                                    break;
                                default:
                                    raise_error("Instruction Not Implemented", &cmd, regs, head);
                                    return 1;
                            }
                            break;
                        // console write unsigned integer
                        case 0x0808:
                            switch(sinfo.func3) {
                                case 0x0:
                                    printf("%x", (unsigned int)((unsigned char)value8));
                                    break;
                                case 0x1:
                                    printf("%x", (unsigned int)((unsigned short)value16));
                                    break;
                                case 0x2:
                                    printf("%x", (unsigned int)value32);
                                    break;
                                default:
                                    raise_error("Instruction Not Implemented", &cmd, regs, head);
                                    return 1;
                            }
                            break;
                        // halt
                        case 0x080C:
                            puts("CPU Halt Requested");
                            free_list(head);
                            return 0;
                        // print program counter
                        case 0x0820:
                            printf("%x", program_counter);
                            break;
                        // dump registers
                        case 0x0824:
                            dump_registers(regs);
                            break;
                        // dump memory word
                        case 0x0828:
                            switch(sinfo.func3) {
                                case 0x0:
                                    printf("%x", memory[(unsigned int)((unsigned char)value8)]);
                                    break;
                                case 0x1:
                                    printf("%x", memory[(unsigned int)((unsigned short)value16)]);
                                    break;
                                case 0x2:
                                    printf("%x", memory[(unsigned int)value32]);
                                    break;
                                default:
                                    raise_error("Instruction Not Implemented", &cmd, regs, head);
                                    return 1; 
                            }
                            break;
                        // allocate memory
                        case 0x0830: ;
                            switch(sinfo.func3) {
                                case 0x0:
                                    regs[28].values.imm = allocate_memory(head, value8);
                                    break;
                                case 0x1:
                                    regs[28].values.imm = allocate_memory(head, value16);
                                    break;
                                case 0x2:
                                    regs[28].values.imm = allocate_memory(head, value32);
                                    break;
                                default:
                                    raise_error("Instruction Not Implemented", &cmd, regs, head);
                                    return 1;
                            }
                            break;
                        // free memory
                        case 0x0834: ;
                            int freed = 0;
                            switch(sinfo.func3) {
                                case 0x0:
                                    freed = free_memory(head, value8);
                                    break;
                                case 0x1:
                                    freed = free_memory(head, value16);
                                    break;
                                case 0x2:
                                    freed = free_memory(head, value32);
                                    break;
                                default:
                                    raise_error("Instruction Not Implemented", &cmd, regs, head);
                                    return 1;
                            }
                            if(freed) {
                                break;
                            }
                        default:
                            raise_error("Illegal Operation", &cmd, regs, head);
                            return 1;
                    }
                }
                break;

            case 0x63: ;
                // making structure type_sb
                struct type_sb sbinfo = {
                    (cmd & 0x7000) >> 12, // func3
                    (cmd & 0xF8000) >> 15, // rs1
                    (cmd & 0x1F00000) >> 20, // rs2
                    0 // imm
                };
                // setting imm
                int twelve = (int)(cmd & 0x80000000) >> 19;
                int ten_to_five = (cmd & 0x7E000000) >> 20;
                int four_to_one = (cmd & 0xF00) >> 7;
                int eleven = (cmd & 0x80) << 4;
                sbinfo.imm = twelve | ten_to_five | four_to_one | eleven;

                switch(sbinfo.func3) {
                    // beq command
                    case 0x0:
                        if(regs[sbinfo.rs1].values.imm == regs[sbinfo.rs2].values.imm) {
                            program_counter += sbinfo.imm - 4;
                        }
                        break;
                    // bne
                    case 0x1:
                        if(regs[sbinfo.rs1].values.imm != regs[sbinfo.rs2].values.imm) {
                            program_counter += sbinfo.imm - 4;
                        }
                        break;
                    // blt command
                    case 0x4:
                        if(regs[sbinfo.rs1].values.imm < regs[sbinfo.rs2].values.imm) {
                            program_counter += sbinfo.imm - 4;
                        }
                        break;
                    // bltu command
                    case 0x6:
                        if((unsigned int)regs[sbinfo.rs1].values.imm < (unsigned int)regs[sbinfo.rs2].values.imm) {
                            program_counter += sbinfo.imm - 4;
                        }
                        break;
                    // bge
                    case 0x5:
                        if(regs[sbinfo.rs1].values.imm >= regs[sbinfo.rs2].values.imm) {
                            program_counter += sbinfo.imm - 4;
                        }
                        break;
                    // bgeu
                    case 0x7: 
                        if((unsigned int)regs[sbinfo.rs1].values.imm >= (unsigned int)regs[sbinfo.rs2].values.imm) {
                            program_counter += sbinfo.imm - 4;
                        }
                        break;
                    default:
                        raise_error("Instruction Not Implemented", &cmd, regs, head);
                        return 1;
                }
                break;

            case 0x37: ;
                // making structure type u
                struct type_u uinfo = {
                    (cmd & 0xF80) >> 7, // rd
                    cmd & 0xFFFFF000 // imm
                };

                // lui command
                regs[uinfo.rd].values.imm = uinfo.imm;
                break; 

            case 0x6F: ;
                // making structure type uj
                struct type_uj ujinfo = {
                    (cmd & 0xF80) >> 7, // rd
                    0 // imm 
                };
                int twenty = (int)(cmd & 0x80000000) >> 11;
                int ten_to_one = (cmd & 0x7FE00000) >> 20;
                int eleventh = (cmd & 0x100000) >> 9;
                int nineteen_to_twelve = (cmd & 0xFF000);
                ujinfo.imm = twenty | ten_to_one | eleventh | nineteen_to_twelve;

                // jal command
                regs[ujinfo.rd].values.imm = program_counter + 4;
                program_counter += ujinfo.imm - 4;
                break;

            default: 
                raise_error("Instruction Not Implemented", &cmd, regs, head);
                return 1;
        }
        // updating program counter and setting 0th register to 0
        program_counter += 4;
        regs[0].values.imm = 0;
    }
    free_list(head);
    return 1; 
}