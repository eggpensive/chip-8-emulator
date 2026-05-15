#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FONTSET_SIZE 80

//chip 8 machine structure
struct Chip8
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
};


const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_START_ADDRESS = 0x50;


//load ROM
void LoadROM(struct Chip8 *chip8, const char *filename)
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


//fontset data
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


//initialize the machine
void initChip8(struct Chip8 *chip8)
{
    memset(chip8, 0, sizeof(struct Chip8)); //sets to zero to clean garbage value

    chip8->pc = START_ADDRESS;
    
    //load fonts into memory
    for (int i = 0; i < FONTSET_SIZE; i++)
    {
        chip8->memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
    
    
}


int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    

    return 0;
}
