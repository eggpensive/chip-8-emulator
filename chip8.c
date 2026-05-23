#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* macros */
#define FONTSET_SIZE 80
#define VIDEO_WIDTH 64
#define VIDEO_HEIGHT 32

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
    uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT]; //64 * 32
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
void OP_4xkk(Chip8 *chip8)  //SNE Vx, byte  (skip next instruction if Vx != kk)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t byte = chip8->opcode & 0x00FFu;

    if (chip8->registers[Vx] != byte)
    {
        chip8->pc += 2;
    }
    
}
void OP_5xy0(Chip8 *chip8)  //SE Vx, Vy     (skip next instruction if Vx = Vy)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;

    if (chip8->registers[Vx] == chip8->registers[Vy])
    {
        chip8->pc += 2;
    }
    
}
void OP_6xkk(Chip8 *chip8)  //LD Vx, byte   (set Vx = kk) 
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t byte = (chip8->opcode & 0x00FFu);

    chip8->registers[Vx] = byte;
}
void OP_7xkk(Chip8 *chip8)  //ADD Vx, byte  (set Vx = Vx + kk)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t byte = chip8->opcode & 0x00FFu;

    chip8->registers[Vx] += byte;
}
void OP_8xy0(Chip8 *chip8)  //LD Vx, Vy     (set Vx = Vy)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;

    chip8->registers[Vx] = chip8->registers[Vy];
}
void OP_8xy1(Chip8 *chip8)  //OR Vx, Vy     (set Vx = Vx OR Vy)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;

    chip8->registers[Vx] |= chip8->registers[Vy];
}
void OP_8xy2(Chip8 *chip8)  //AND Vx, Vy    (set Vx = Vx AND Vy)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;

    chip8->registers[Vx] &= chip8->registers[Vy];
}
void OP_8xy3(Chip8 *chip8)  //XOR Vx, Vy    (set Vx = Vx XOR Vy)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;

    chip8->registers[Vx] ^= chip8->registers[Vy];
}
void OP_8xy4(Chip8 *chip8)  //ADD Vx, Vy    (set Vx = Vx + Vy, set VF = carry)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;

    uint16_t sum = chip8->registers[Vx] + chip8->registers[Vy]; //16 bit to anticipate 8 bit overflow

    if (sum > 255)   //check if it is greater than 255 (overflow)
    {                                                           
        chip8->registers[0xF] = 1;  //set VF to 1, acting as an overflow flag
    }
    else
    {
        chip8->registers[0xF] = 0;
    }

    chip8->registers[Vx] = sum & 0xFFu;   //rolls back to zero (equivalent to sum % 256)
}
void OP_8xy5(Chip8 *chip8)  //SUB Vx, Vy    (set Vx = Vx - Vy, set VF = NOT borrow)
{                                           
    //if Vx > Vy, then set VF to 1, otherwise 0. then Vx = Vx - Vy

    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;

    if (chip8->registers[Vx] >= chip8->registers[Vy])
    {
        chip8->registers[0xF] = 1;
    }
    else
    {
        chip8->registers[0xF] = 0;
    }
    
    chip8->registers[Vx] -= chip8->registers[Vy];
}
void OP_8xy6(Chip8 *chip8)  //SHR Vx        (set Vx = Vx SHR 1)
{
    //SHR = shift right
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    chip8->registers[0xF] = chip8->registers[Vx] & 1;
    
    chip8->registers[Vx] >>= 1;
}
void OP_8xy7(Chip8 *chip8)  //SUBN Vx, Vy   (set Vx = Vy - Vx, set VF = NOT borrow)
{
    //If Vy > Vx, then set VF to 1, otherwise 0. Then Vx = Vy - Vx

    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;

    if (chip8->registers[Vy] >= chip8->registers[Vx])
    {
        chip8->registers[0xF] = 1;
    }
    else
    {
        chip8->registers[0xF] = 0;
    }
    
    chip8->registers[Vx] = chip8->registers[Vy] - chip8->registers[Vx];
}
void OP_8xyE(Chip8 *chip8)  //SHL Vx        (set Vx = Vx SHL 1)
{
    //SHL = shift left
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    chip8->registers[0xF] = (chip8->registers[Vx] & 128) >> 7;
    
    chip8->registers[Vx] <<= 1;
}
void OP_9xy0(Chip8 *chip8)  //SNE Vx, Vy    (skip next instruction if Vx != Vy)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;

    if (chip8->registers[Vx] != chip8->registers[Vy])
    {
        chip8->pc += 2;
    }
}
void OP_Annn(Chip8 *chip8)  //LD I, addr    (set I = nnn)
{
    uint16_t address = chip8->opcode & 0x0FFFu;

    chip8->index = address;
}
void OP_Bnnn(Chip8 *chip8)  //JP V0, addr    (jump to location nnn + V0)
{
    uint16_t address = chip8->opcode & 0x0FFFu;

    chip8->pc = address + chip8->registers[0];
}
void OP_Cxkk(Chip8 *chip8)  //RND Vx, byte  (set Vx = random byte AND kk.)
{
    //a random number from 0 to 255 is AND-ed with the value kk
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t byte = chip8->opcode & 0x00FFu;
    
    chip8->registers[Vx] = (rand() % 256) & byte;
}
void OP_Dxyn(Chip8 *chip8)  //DRW Vx, Vy, nibble (Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision)
{
    //The interpreter reads n bytes from memory, starting at the address stored in I. These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). 
    //Sprites are XORed onto the existing screen. If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. 
    //If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen.
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t Vy = (chip8->opcode & 0x00F0u) >> 4;
    uint8_t height = chip8->opcode & 0x000Fu;   //height is basically amount of bytes

    //origin coordinates
    uint8_t xOrg = chip8->registers[Vx] % VIDEO_WIDTH;  //VIDEO_WIDTH = 64      note: modulo is for wrapping around
    uint8_t yOrg = chip8->registers[Vy] % VIDEO_HEIGHT; //VIDEO_HEIGHT = 32

    chip8->registers[0xF] = 0;

    for (int row = 0; row < height; row++)
    {
        uint8_t spriteByte = chip8->memory[chip8->index + row];

        for (int col = 0; col < 8; col++)
        {
            uint8_t xPos = xOrg + col;
            uint8_t yPos = yOrg + row;

            // a safety net to ignore operations beyond screen boundary
            if (yPos > VIDEO_HEIGHT - 1 || xPos > VIDEO_WIDTH - 1)
            {
                continue;
            }
            
            uint8_t spritePixel = spriteByte & (128 >> col); //128 = 1000 0000
            uint32_t *screenPixel = &chip8->video[(yPos * VIDEO_WIDTH) + xPos];   //video is a 1D array, therefore, this calculation is mandatory

            //sprite pixel is on
            if (spritePixel)
            {
                //screen pixel is also on, set collision
                if (*screenPixel == 0xFFFFFFFF)
                {
                    chip8->registers[0xF] = 1;  //flag collision
                }
                
                //toggle screen pixel using XOR
                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}


/* main  */
int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    

    return 0;
}