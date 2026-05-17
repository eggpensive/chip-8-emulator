#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* macros */
#define FONTSET_SIZE 80

/* start addresses */
const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_START_ADDRESS = 0x50;

/* chip 8 machine structure */
typedef struct
{
    uint8_t registers[16];
    uint8_t memory[4096];
    uint16_t index;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delayTimer;
    uint8_t soundTimer;
    uint8_t keypad[16];
    uint32_t video[64 * 32];
    uint16_t opcode;
} Chip8;

/* fontset data */
uint8_t fontset[FONTSET_SIZE] =     //must use preprocessor because global array, VLA is not allowed.
{
    0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
	0x20, 0x60, 0x20, 0x20, 0x70,   // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
	0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
	0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
	0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
	0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
	0xF0, 0x80, 0xF0, 0x80, 0x80    // F
};


/* load ROM */
void LoadROM(Chip8 *chip8, const char *filename)
{
    FILE *romfPtr;
    
    if ((romfPtr = fopen(filename, "rb")) == NULL)
    {
        puts("Unable to open file.");
        exit(1);
    }
    else
    {
        //move file pointer to end of file and check file size
        fseek(romfPtr, 0, SEEK_END);
        long size = ftell(romfPtr);

        rewind(romfPtr);

        fread(&chip8->memory[START_ADDRESS], 1, size, romfPtr);
    }
    

    fclose(romfPtr);
}


/* initialize machine */
void initChip8(Chip8 *chip8)
{
    srand(time(NULL));                      //uses current time RNG seed

    memset(chip8, 0, sizeof(Chip8)); //sets to zero to clean garbage value

    chip8->pc = START_ADDRESS;              //sets program counter to starting address
    
    //load fonts into memory
    for (int i = 0; i < FONTSET_SIZE; i++)
    {
        chip8->memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
    
    
}


/* chip 8 instructions */
void OP_00E0(Chip8 *chip8)  //CLS (clear display)
{
    memset(chip8->video, 0, sizeof(chip8->video));
}
void OP_00EE(Chip8 *chip8)   //RET (return from subroutine)
{
    chip8->sp--;
    chip8->pc = chip8->stack[chip8->sp];
}
void OP_1nnn(Chip8 *chip8)  //JP addr       (jump to location nnn)
{
    uint16_t address = chip8->opcode & 0x0FFFu; //extract address only using bitwise operator

    chip8->pc = address;
}
void OP_2nnn(Chip8 *chip8)  //CALL addr     (call subroutine at nnn)
{
    uint16_t address = chip8->opcode & 0x0FFFu;

    chip8->stack[chip8->sp] = chip8->pc;    //assign next instruction address pc to stack[sp] before call
                                            //note: no need to increment pc+2 because the cycle has already increment before executing instructions
    chip8->sp++;

    chip8->pc = address;
}
void OP_3xkk(Chip8 *chip8)  //SE Vx, byte   (skip next instruction if Vx = kk)
{
    //(SE = skip if equal; V = register)
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;    //mask and shift 8 bit because 1 hex digit = 4 bit.
                                                    //let  opcode = 3A22; after mask = 0A00; after shift 2 hex = 0A
                                                    //(note: 0A00 != 0A, that is why bitshifting is necessary)
    uint8_t byte = chip8->opcode & 0x00FFu;

    if (chip8->registers[Vx] == byte)
    {
        chip8->pc += 2;
    }
    
}


/* main  */
int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    

    return 0;
}