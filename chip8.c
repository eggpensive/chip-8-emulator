#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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


//initialize the machine
void initChip8(struct Chip8 *chip8)
{
    memset(chip8, 0, sizeof(struct Chip8)); //sets to zero to clean garbage value

    chip8->pc = START_ADDRESS;              
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    

    return 0;
}
