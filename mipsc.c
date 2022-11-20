// COMP1521 22T3 Assignment 2: mipsc -- a MIPS simulator
// starting point code v1.0 - 24/10/22

//******************************************************
// IN PROGRESS
//******************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h> 
#include <unistd.h>

// ADD ANY ADDITIONAL #include HERE

#define MAX_LINE_LENGTH 256
#define INSTRUCTIONS_GROW 64
#define ADD 1
#define SUB 2
#define SLT 3
#define MFHI 4 
#define MFLO 5 
#define MULT 6 
#define DIV 7 
#define MUL 8 
#define BEQ 9
#define BNE 10
#define ADDI 11
#define ORI 12 
#define LUI 13
#define SYSCALL 0 
#define LO 32
#define HI 33
void execute_instructions(uint32_t n_instructions,
                          uint32_t instructions[],
                          int trace_mode);
char *process_arguments(int argc, char *argv[], int *trace_mode);
uint32_t *read_instructions(char *filename, uint32_t *n_instructions_p);
uint32_t *instructions_realloc(uint32_t *instructions, uint32_t n_instructions);
int determine_instruction(uint32_t hex_number);
void first_group(int which_instruction, uint32_t binary_instruction, int registers[]);
void second_group(int which_instruction, uint32_t binary_instruction, int registers[]);
int third_group(int which_instruction, uint32_t binary_instructions, int registers[], int current_instruction);
void fourth_group(int which_instruction, uint32_t binary_instruction, int registers[]);
void mips_syscall(int registers[], int trace_mode);
int supress_stdout();
void resume_stdout(int fd);

// ADD ANY ADDITIONAL FUNCTION PROTOTYPES HERE

// YOU DO NOT NEED TO CHANGE MAIN
// but you can if you really want to
int main(int argc, char *argv[]) {
    int trace_mode;
    char *filename = process_arguments(argc, argv, &trace_mode); // error checking for command-line arguments
                                                                // also sets variable trace_mode to 0 or 1 depending on if -r is specified
    uint32_t n_instructions;
    uint32_t *instructions = read_instructions(filename, &n_instructions); // each line (hexadecimal string) in the file is converted
                                                                            // to a number and stored in each instructions array index
    execute_instructions(n_instructions, instructions, trace_mode);

    free(instructions);
    return 0;
}


// simulate execution of  instruction codes in  instructions array
// output from syscall instruction & any error messages are printed
//
// if trace_mode != 0:
//     information is printed about each instruction as it executed
//
// execution stops if it reaches the end of the array
void execute_instructions(uint32_t n_instructions,
                          uint32_t instructions[],
                          int trace_mode) {

    int32_t registers[35];
    for (int i = 0; i < 35; i++) { 
        registers[i] = 0;
    }
    
    if (trace_mode == 0) {
        registers[34] = supress_stdout(); 
    }

    for (int i = 0; i < n_instructions; i++) { 
        printf("%d: 0x%08X ", i, instructions[i]);
        int which_instruction = determine_instruction(instructions[i]);
        
        if (which_instruction == ADD || which_instruction == SUB || which_instruction == MUL || which_instruction == SLT) { 
            first_group(which_instruction, instructions[i], registers);
        }

        else if (which_instruction == MULT || which_instruction == DIV || which_instruction == MFHI || which_instruction == MFLO) { 
            second_group(which_instruction, instructions[i], registers);
        }

        else if (which_instruction == SYSCALL) { 
            mips_syscall(registers, trace_mode);
        }

        else if(which_instruction == ADDI || which_instruction == ORI || which_instruction == LUI) { 
            fourth_group(which_instruction, instructions[i], registers);
        }

        //keep this last 
        else if (which_instruction == BEQ || which_instruction == BNE) { 
            int pc = third_group(which_instruction, instructions[i], registers, i);
            i = pc;
           
        }
        registers[0] = 0;
    }

}



int determine_instruction(uint32_t hex_number) {  //should it be unsigned?
    int which_instruction = -1;

    if ((hex_number & 0xFC000000) == 0) { //retrieve the first 6 bytes of hex_number
                                        // if equal to 0, can only be 8 possibilities 
        uint32_t last_six_bits = hex_number & 0x3F; //setting all but last 6 bits to 0

        switch (last_six_bits) {            // determining which of the 8 possibilities it is
            case 32:
            which_instruction = ADD;
            break; 

            case 34: 
            which_instruction = SUB;
            break;

            case 42: 
            which_instruction = SLT;
            break;

            case 16: 
            which_instruction = MFHI;
            break;

            case 18: 
            which_instruction = MFLO;
            break;
            
            case 24: 
            which_instruction = MULT;
            break;

            case 26: 
            which_instruction = DIV;
            break;

            case 12: 
            which_instruction = SYSCALL;
            break;
            
        }

    }

    else { 
        uint32_t first_six_bits = (hex_number & 0xFC000000) >> 26; // setting all but the first six bits to 0
        switch (first_six_bits) { 
            case 28: 
            which_instruction = MUL;
            break;

            case 4: 
            which_instruction = BEQ;
            break;

            case 5:
            which_instruction = BNE;
            break;

            case 8: 
            which_instruction = ADDI;
            break;

            case 13: 
            which_instruction = ORI;
            break;

            case 15: 
            which_instruction = LUI; 
            break;

        }
    }

    return which_instruction;
}

void first_group(int which_instruction, uint32_t binary_instruction, int registers[]) { 
    uint32_t destination, source, transform;
    destination = (binary_instruction >> 11) & 0x1F;                //extracting destination register
    transform = (binary_instruction >> 16) & 0x1F;
    source = (binary_instruction >> 21) & 0x1F;

    if (which_instruction == ADD) { 
        printf("add  $%d, $%d, $%d\n", destination, source, transform);
        registers[destination] = registers[source] + registers[transform];
    }

    else if (which_instruction == SUB) { 
        printf("sub  $%d, $%d, $%d\n", destination, source, transform);
        registers[destination] = registers[source] - registers[transform];
    }

    else if (which_instruction == MUL) { 
        printf("mul  $%d, $%d, $%d\n", destination, source, transform);
        registers[destination] = registers[source] * registers[transform];
    }

    else if (which_instruction == SLT) { 
        printf("slt  $%d, $%d, $%d\n", destination, source, transform);
        registers[destination] = registers[source] < registers[transform];
    }
 
    printf(">>> $%d = %d\n", destination, registers[destination]);

    return;
}

void second_group(int which_instruction, uint32_t binary_instruction, int registers[]) { 
    uint32_t destination, source, transform;
    destination = (binary_instruction >> 11) & 0x1F;                //extracting destination register
    transform = (binary_instruction >> 16) & 0x1F;
    source = (binary_instruction >> 21) & 0x1F;

     
    if (which_instruction == MULT) { 
        printf("mult $%d, $%d\n", source, transform);
        int64_t temp = (int64_t) registers[source] * registers[transform];
        registers[HI] = temp >> 32;
        registers[LO] = temp & 0x00000000FFFFFFFF;
        printf(">>> HI = %d\n", registers[HI]);
        printf(">>> LO = %d\n", registers[LO]);
    }
     
    else if (which_instruction == DIV) {
        registers[HI] = registers[source] % registers[transform]; 
        registers[LO] = registers[source] / registers[transform];
        printf("div  $%d, $%d\n", source, transform);
        printf(">>> HI = %d\n", registers[HI]);
        printf(">>> LO = %d\n", registers[LO]);
    }

    else if (which_instruction == MFHI) { 
        registers[destination] = registers[HI];
        printf("mfhi $%d\n", destination);
        printf(">>> $%d = %d\n", destination, registers[destination]);
    }

    else if (which_instruction == MFLO) { 
        registers[destination] = registers[LO];
        printf("mflo $%d\n", destination);
        printf(">>> $%d = %d\n", destination, registers[destination]);
    }

    return;
}

int third_group(int which_instruction, uint32_t binary_instruction, int registers[], int current_instruction) { 
    int16_t source, transform, immediate;
    immediate = binary_instruction & 0x0000FFFF;
    transform = (binary_instruction >> 16) & 0x1F;
    source = (binary_instruction >> 21) & 0x1F;
    int pc = current_instruction; 

    if (which_instruction == BEQ) { 
        printf("beq  $%d, $%d, %d\n", source, transform, immediate);
        if (registers[transform] == registers[source]) { 
            pc += immediate;
            printf(">>> branch taken to PC = %d\n", pc);
            pc--;
        }
        else { 
            printf(">>> branch not taken\n");
        }
    }

    else if (which_instruction == BNE) { 
        printf("bne  $%d, $%d, %d\n", source, transform, immediate);
        if (registers[transform] != registers[source]) { 
            pc += immediate;
            printf(">>> branch taken to PC = %d\n", pc);
            pc--;
        }
        else { 
            printf(">>> branch not taken\n");
        }
    }

    return pc;
}

void fourth_group(int which_instruction, uint32_t binary_instruction, int registers[]) { 
    int16_t source, transform, immediate;
    immediate = binary_instruction & 0x0000FFFF;
    transform = (binary_instruction >> 16) & 0x1F;
    source = (binary_instruction >> 21) & 0x1F;
    
    if (which_instruction == ADDI) { 
        registers[transform] = registers[source] + immediate;
        printf("addi $%d, $%d, %d\n", transform, source, immediate);
        printf(">>> $%d = %d\n", transform, registers[transform]);
    }

    else if (which_instruction == ORI) { 
        registers[transform] = registers[source] | (uint16_t) immediate;
        printf("ori  $%d, $%d, %d\n", transform, source, immediate);
        printf(">>> $%d = %d\n", transform, registers[transform]);
    }

    else if (which_instruction == LUI) { 
        registers[transform] = immediate << 16;
        printf("lui  $%d, %d\n", transform, immediate);
        printf(">>> $%d = %d\n", transform, registers[transform]);
    }

}

void mips_syscall(int registers[], int trace_mode) { 
    if (trace_mode == 0) {
        printf("HELLOOOOOOOO");
        resume_stdout(registers[34]);
    }

    int sys_command = registers[2];

    if (trace_mode == 0) { 
        if (sys_command == 1) { 
            printf("%d", registers[4]);
        }

        else if (sys_command == 10) { 
            exit(0);
        }

        else if (sys_command == 11) { 
            printf("%c", registers[4] & 0x000000FF);
        }
        else { 
            fprintf(stderr, "Unknown system call: %d\n", sys_command);
            exit(0);
        }
    }
    else {
        printf("syscall\n");
        printf(">>> syscall %d\n", sys_command);
        
        if (sys_command == 1) { 
            printf("<<< %d\n", registers[4]);
        }

        else if (sys_command == 10) { 
            exit(0);
        }

        else if (sys_command == 11) { 
            printf("<<< %c\n", registers[4] & 0x000000FF);
        }
        else {
            fprintf(stderr, "Unknown system call: %d\n", sys_command);
            exit(0);
        }
    }

    if (trace_mode == 0) { 
        registers[34] = supress_stdout();
    }
}

int supress_stdout() {
  fflush(stdout);

  int ret = dup(1);
  int nullfd = open("/dev/null", O_WRONLY);
  // check nullfd for error omitted
  dup2(nullfd, 1);
  close(nullfd);

  return ret;
}

void resume_stdout(int fd) {
  fflush(stdout);
  dup2(fd, 1);
  close(fd);
}



// DO NOT CHANGE ANY CODE BELOW HERE


// check_arguments is given command-line arguments
// it sets *trace_mode to 0 if -r is specified
//         *trace_mode is set to 1 otherwise
// the filename specified in command-line arguments is returned
char *process_arguments(int argc, char *argv[], int *trace_mode) {
    if (
        argc < 2 ||
        argc > 3 ||
        (argc == 2 && strcmp(argv[1], "-r") == 0) ||
        (argc == 3 && strcmp(argv[1], "-r") != 0)
    ) {
        fprintf(stderr, "Usage: %s [-r] <file>\n", argv[0]);
        exit(1);
    }
    *trace_mode = (argc == 2);
    return argv[argc - 1];
}


// read hexadecimal numbers from filename one per line
// numbers are return in a malloc'ed array
// *n_instructions is set to size of the array
uint32_t *read_instructions(char *filename, uint32_t *n_instructions_p) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        perror(filename);
        exit(1);
    }

    uint32_t *instructions = NULL;
    uint32_t n_instructions = 0;
    char line[MAX_LINE_LENGTH + 1];
    while (fgets(line, sizeof line, f) != NULL) {

        // grow instructions array in steps of INSTRUCTIONS_GROW elements
        if (n_instructions % INSTRUCTIONS_GROW == 0) {
            instructions = instructions_realloc(instructions, n_instructions + INSTRUCTIONS_GROW);
        }

        char *endptr;
        instructions[n_instructions] = (uint32_t)strtoul(line, &endptr, 16);
        if (*endptr != '\n' && *endptr != '\r' && *endptr != '\0') {
            fprintf(stderr, "line %d: invalid hexadecimal number: %s",
                    n_instructions + 1, line);
            exit(1);
        }
        if (instructions[n_instructions] != strtoul(line, &endptr, 16)) {
            fprintf(stderr, "line %d: number too large: %s",
                    n_instructions + 1, line);
            exit(1);
        }
        n_instructions++;
    }
    fclose(f);
    *n_instructions_p = n_instructions;
    // shrink instructions array to correct size
    instructions = instructions_realloc(instructions, n_instructions);
    return instructions;
}


// instructions_realloc is wrapper for realloc
// it calls realloc to grow/shrink the instructions array
// to the specified size
// it exits if realloc fails
// otherwise it returns the new instructions array
uint32_t *instructions_realloc(uint32_t *instructions, uint32_t n_instructions) {
    instructions = realloc(instructions, n_instructions * sizeof *instructions);
    if (instructions == NULL) {
        fprintf(stderr, "out of memory");
        exit(1);
    }
    return instructions;
}
