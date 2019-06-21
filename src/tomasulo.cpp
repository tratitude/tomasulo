#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <set>

using namespace std;

enum FuntionUnit{
    LoadBuffer0, LoadBuffer1,
    StoreBuffer0, StoreBuffer1,
    Adder0, Adder1, Adder2,
    Multiplier0, Multiplier1
};

enum Opcode{
    LD, SD, ADD, SUB, MUL, DIV
};

typedef struct{
    string name;
    int value;
}Reg_t;

typedef struct{
    string opcode;
    Reg_t rs;
    Reg_t rt;
    Reg_t rd;
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

#define FU_N 9  // FuntionUnit number
#define FR_N 16 // float point register number
#define IR_N 32 // integer register number
#define LD_C 2  // ld cycle
#define SD_C 2  // sd cycle
#define ADD_C 2 // add cycle
#define SUB_C 2 // sub cycle
#define MUL_C 10 // mul cycle
#define DIV_C 40 // div cycle

ReservationStation_t ReservationStation[FU_N];
RegisterStatus_t FRegister[FR_N];
RegisterStatus_t IRegister[IR_N];
vector<Instruction_t> Instruction;
map<uint64_t, int> Memory;
//set<pair<string, int>> OpTable;  // use to translate between string and int
//set<pair<string, int>> RegTable;
map<string, int> RegValue;
int Clock = 0;
/*
void init_regtable()
{
    int froffset = 100;
    int iroffset = 200;
    for(int i = 0; i < FR_N; i++)
    {
        string regstr("F");
        int regnum = i*2;
        regstr += to_string(regnum);
        RegTable.insert(make_pair(regstr, froffset + regnum));
    }
    for (int i = 0; i < IR_N; i++)
    {
        string regstr("R");
        int regnum = i;
        regstr += to_string(regnum);
        RegTable.insert(make_pair(regstr, iroffset + regnum));
    }
}

int reg_encode(string regstr)
{
    for(auto it = RegTable.begin(); it != RegTable.end(); it++)
    {
        if (!regstr.compare(it->first))
            return it->second;
    }
}

string reg_decode(int reg)
{
    for (auto it = RegTable.begin(); it != RegTable.end(); it++)
    {
        if (it->second == reg)
            return it->first;
    }
}

void init_optable()
{
    OpTable.insert(make_pair("L.D", LD));
    OpTable.insert(make_pair("S.D", SD));
    OpTable.insert(make_pair("ADD.D", ADD));
    OpTable.insert(make_pair("SUB.D", SUB));
    OpTable.insert(make_pair("MUL.D", MUL));
    OpTable.insert(make_pair("DIV.D", DIV));
}
int op_encode(string opstr)
{
    for (auto it = RegTable.begin(); it != RegTable.end(); it++)
    {
        if (!opstr.compare(it->first))
            return it->second;
    }
}

string op_decode(int op)
{
    for (auto it = RegTable.begin(); it != RegTable.end(); it++)
    {
        if (it->second == op)
            return it->first;
    }
}
*/
void init_regvalue()
{
    for (int i = 0; i < FR_N; i++)
    {
        string regstr("F");
        int regnum = i * 2;
        regstr += to_string(regnum);
        RegValue.insert(make_pair(regstr, 1));
    }
    for (int i = 0; i < IR_N; i++)
    {
        string regstr("R");
        int regnum = i;
        regstr += to_string(regnum);
        if(i==1)
            regnum = 16;
        else
            regnum = 0;
        RegValue.insert(make_pair(regstr, regnum));
    }
}
void init_instruction(string filename)
{
    string istr;
    ifstream ifile(filename, ifstream::in);
    if(!ifile){
        cout << "file open failed" << endl;
    }
    while (getline(ifile, istr))
    {
        Instruction_t in;
        string opstr, rdstr, rsstr, rtstr;

        istringstream iss(istr);
        iss >> opstr >> rdstr;
        rdstr.erase(rdstr.size() - 1);
        if (!opstr.compare("L.D") || !opstr.compare("S.D"))
        {
            int rs; rsstr = "";
            iss >> rs >> rtstr;
            in.rs.value = rs;
            rtstr.erase(rtstr.begin());
            rtstr.erase(rtstr.size()-1);
        }
        else
        {
            iss >> rsstr >> rtstr;
            rsstr.erase(rsstr.size()-1);
        }
        
        in.opcode = opstr; in.rd.name = rdstr; in.rs.name = rsstr; in.rt.name = rtstr;
        Instruction.push_back(in);
    }
    ifile.close();
}
void print_instruction()
{
    for(auto it = Instruction.begin(); it != Instruction.end(); it++)
    {
        cout << it->opcode << " " << it->rd.name << " ";
        if(!it->opcode.compare("L.D") || !it->opcode.compare("S.D")){
            cout << it->rs.value;
        }
        else{
            cout << it->rs.name;
        }
        cout << " " << it->rt.name << endl;
    }
}
// if nothing to write result, return true
bool WriteResult()
{

}
// if nothing to execute, return true
bool Execute()
{

}
// if nothing to issue, return true
bool Issue()
{

}
int main()
{
    //初始化各個結構
    //init_regtable();
    //init_optable();
    string filename = ".\\doc\\test2";
    init_instruction(filename);
    init_regvalue();
    //print_instruction();
    //何時離開主迴圈? WriteResult()、Execute()、Issue()皆無推進任何一個執令，或是struct Instruction中的Issue、Execution、Write皆非原初始值。
    while (1)
    {
        bool finished = true;
        finished &= WriteResult(); //執行已完成Execution步驟的指令
        finished &= Execute();     //執行已完成Issue步驟的指令，並在維護每個正在本步驟的指令
        finished &= Issue();       //issue下一個未被issue的指令
        Clock++;
    }
    
    return 0;
}
