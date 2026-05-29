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
void OP_Ex9E(Chip8 *chip8)  //SKP Vx        (skip next instruction if key with the value of Vx is pressed)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    uint8_t key = chip8->registers[Vx];

    if (chip8->keypad[key])
    {
        chip8->pc += 2;
    }
}
void OP_ExA1(Chip8 *chip8)  //SKNP Vx       (skip next instruction if key with the value of Vx is not pressed)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    uint8_t key = chip8->registers[Vx];

    if (!chip8->keypad[key])
    {
        chip8->pc += 2;
    }
}
void OP_Fx07(Chip8 *chip8)  //LD Vx, DT     (set Vx = delay timer value)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    chip8->registers[Vx] = chip8->delayTimer;
}
void OP_Fx0A(Chip8 *chip8)  //LD Vx, K      (wait for a key press, store the value of the key in Vx)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    int keyPress = 0;

    //check if any key is pressed
    for (uint8_t i = 0; i < 16; i++)
    {
        if (chip8->keypad[i])
        {
            chip8->registers[Vx] = i;
            keyPress = 1;
            break;
        }
    }

    //pull pc back to current instruction after the increment,
    //creating a loop that waits until a key is pressed
    if (!keyPress)
    {
        chip8->pc -= 2; 
    }
}
void OP_Fx15(Chip8 *chip8)  //LD DT, Vx     (set delay timer = Vx)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    chip8->delayTimer = chip8->registers[Vx];
}
void OP_Fx18(Chip8 *chip8)  //LD ST, Vx     (set sound timer = Vx)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    chip8->soundTimer = chip8->registers[Vx];
}
void OP_Fx1E(Chip8 *chip8)  //ADD I, Vx     (set I = I + Vx)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    chip8->index += chip8->registers[Vx];
}
void OP_Fx29(Chip8 *chip8)  //LD F, Vx      (set I = location of sprite for digit Vx)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t digit = chip8->registers[Vx];
    
    chip8->index = FONTSET_START_ADDRESS + (5 * digit);  //5 because each font sprite is 5 bytes
}
void OP_Fx33(Chip8 *chip8)  //LD B, Vx      (store BCD representation of Vx in memory locations I, I+1, and I+2)
{
    //The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I,
    //the tens digit at location I+1, and the ones digit at location I+2.
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;
    uint8_t value = chip8->registers[Vx];
    
    chip8->memory[chip8->index + 2] = value % 10;
    value /= 10;

    chip8->memory[chip8->index + 1] = value % 10;
    value /= 10;

    chip8->memory[chip8->index] = value;
}
void OP_Fx55(Chip8 *chip8)  //LD [I], Vx    (store registers V0 through Vx in memory starting at location I)
{
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    for (uint8_t i = 0; i <= Vx; i++)
    {
        chip8->memory[chip8->index + i] = chip8->registers[i];
    }
}
void OP_Fx65(Chip8 *chip8)  //LD Vx, [I]    (read registers V0 through Vx from memory starting at location I)
{
    //The interpreter reads values from memory starting at location I into registers V0 through Vx.
    uint8_t Vx = (chip8->opcode & 0x0F00u) >> 8;

    for (uint8_t i = 0; i <= Vx; i++)
    {
        chip8->registers[i] = chip8->memory[chip8->index + i];
    }
}
void OP_NULL(Chip8 *chip8)  //DUMMY FUNC    (does nothing)
{
    //Do nothing.
    (void)chip8;    //just to avoid compile warning
}

/* function pointer tables */
void (*table[16])(Chip8 *chip8);        //master table
void (*table8[0xE + 1])(Chip8 *chip8);  //note: +1 because we need to access index 0xE
void (*table0[0xE + 1])(Chip8 *chip8);
void (*tableE[0xE + 1])(Chip8 *chip8);
void (*tableF[0x65 + 1])(Chip8 *chip8);

/* dispatch functions */
void Table8(Chip8 *chip8)
{
    uint8_t lastDigit = chip8->opcode & 0x000Fu;

    table8[lastDigit](chip8);
}
void Table0(Chip8 *chip8)
{
    uint8_t lastDigit = chip8->opcode & 0x000Fu;

    table0[lastDigit](chip8);
}
void TableE(Chip8 *chip8)
{
    uint8_t lastDigit = chip8->opcode & 0x000Fu;

    tableE[lastDigit](chip8);
}
void TableF(Chip8 *chip8)
{
    uint8_t lastTwo = chip8->opcode & 0x00FFu;

    tableF[lastTwo](chip8);
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
    
    /* initialize secondary tables */
    for (int i = 0; i <= 0xE; i++)
    {
        //OP_NULL for it does nothing. Just in case if undefined OP functions are accessed.
        table8[i] = OP_NULL;
        table0[i] = OP_NULL;
        tableE[i] = OP_NULL;   
    }
    for (int i = 0; i <= 0x65; i++)
    {
        tableF[i] = OP_NULL;
    }

    /* initialize master table */
    table[0] = Table0;
    table[1] = OP_1nnn;
    table[2] = OP_2nnn;
    table[3] = OP_3xkk;
    table[4] = OP_4xkk;
    table[5] = OP_5xy0;
    table[6] = OP_6xkk;
    table[7] = OP_7xkk;
    table[8] = Table8;
    table[9] = OP_9xy0;
    table[0xA] = OP_Annn;
    table[0xB] = OP_Bnnn;
    table[0xC] = OP_Cxkk;
    table[0xD] = OP_Dxyn;
    table[0xE] = TableE;
    table[0xF] = TableF;

    /* initialize secondary table */
    table8[0] = OP_8xy0;
    table8[1] = OP_8xy1;
    table8[2] = OP_8xy2;
    table8[3] = OP_8xy3;
    table8[4] = OP_8xy4;
    table8[5] = OP_8xy5;
    table8[6] = OP_8xy6;
    table8[7] = OP_8xy7;
    table8[0xE] = OP_8xyE;

    table0[0] = OP_00E0;
    table0[0xE] = OP_00EE;

    tableE[1] = OP_ExA1;
    tableE[0xE] = OP_Ex9E;

    tableF[0x07] = OP_Fx07;
    tableF[0x0A] = OP_Fx0A;
    tableF[0x15] = OP_Fx15;
    tableF[0x18] = OP_Fx18;
    tableF[0x1E] = OP_Fx1E;
    tableF[0x29] = OP_Fx29;
    tableF[0x33] = OP_Fx33;
    tableF[0x55] = OP_Fx55;
    tableF[0x65] = OP_Fx65;
}

/* cycle */
void Cycle(Chip8 *chip8)
{
    //fetch
    chip8->opcode = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
    chip8->pc += 2;

    //decode
    uint8_t firstDigit = (chip8->opcode & 0xF000u) >> 12;

    //execute
    table[firstDigit](chip8);

    //decrement delay timer
    if (chip8->delayTimer > 0)
    {
        chip8->delayTimer--;
    }
    
    //decrement sound timer
    if (chip8->soundTimer > 0)
    {
        chip8->soundTimer--;
    }
}


/* main  */
int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    

    return 0;
}