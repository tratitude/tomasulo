#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

enum FuntionUnit{
    LoadBuffer0, LoadBuffer1,
    StoreBuffer0, StoreBuffer1,
    Adder0, Adder1, Adder2,
    Multiplier0, Multiplier1
};

typedef struct{
    int opcode;
    int rs;
    int rt;
    int rd;
    int offset;
    int Issue; //紀錄完成該步驟的迴圈
    int Execution;
    int Write;
} Instruction_t;

typedef struct{
    int busy;
    int opcode;
    int FU; //記錄使用哪個function unit
    float Vj;
    float Vk;
    int Qi;
    int Qj;
    int Qk;
    int A;
} ReservationStation_t;

typedef struct{
    int Qi;
} RegisterStatus_t;

#define FU_N 9
#define FR_N 16
#define IR_N 32

ReservationStation_t ReservationStation[FU_N];
RegisterStatus_t FRegister[FR_N];
RegisterStatus_t IRegister[IR_N];

int Clock = 0;

int main()
{
    //初始化各個結構

    //何時離開主迴圈? WriteResult()、Execute()、Issue()皆無推進任何一個執令，或是struct Instruction中的Issue、Execution、Write皆非原初始值。
    while (1)
    {
        WriteResult(); //執行已完成Execution步驟的指令
        Execute();     //執行已完成Issue步驟的指令，並在維護每個正在本步驟的指令
        Issue();       //issue下一個未被issue的指令
        Clock++;
    }
    return 0;
}
