#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <set>

using namespace std;

enum FuntionUnit : int{
    LoadBuffer0, LoadBuffer1,
    StoreBuffer0, StoreBuffer1,
    Adder0, Adder1, Adder2,
    Multiplier0, Multiplier1
};
/*
enum Opcode{
    LD, SD, ADD, SUB, MUL, DIV
};
*/
typedef struct{
    string opcode;
    string rs;
    string rt;
    string rd;
    int offset;
    int Issue; //紀錄完成該步驟的迴圈
    int Execution;
    int Write;
} Instruction_t;

typedef struct{
    bool busy;
    string opcode;
    //int FU; //記錄使用哪個function unit
    float Vi;
    float Vj;
    float Vk;
    int Qi; // if not used, set as -1
    int Qj; // if not used, set as -1
    int Qk; // if not used, set as -1
    int cycle;  // how many cycles needed, init as -1
    Instruction_t &ins;
} ReservationStation_t;

typedef struct{
    int fu; // if not used, set as -1
    double value;
} Register_t;

#define FU_N 9  // Funtion Unit number
#define FR_N 16 // float point register number
#define IR_N 32 // integer register number
#define LD_C 2  // ld cycle
#define SD_C 2  // sd cycle
#define ADD_C 2 // add cycle
#define SUB_C 2 // sub cycle
#define MUL_C 10 // mul cycle
#define DIV_C 40 // div cycle

ReservationStation_t ReservationStation[FU_N];
map<string, Register_t> Register;  // use to get function unit and register value from register string
vector<Instruction_t> Instruction;
map<uint64_t, int> Memory;
map<string, int> Cycle;
//set<pair<string, int>> OpTable;  // use to translate between string and int
//set<pair<string, int>> RegTable;
//map<string, int> RegValue;  // use to get register value from string
int Clock = 0;
int PC = 0;
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
void init_Register()
{
    for (int i = 0; i < FR_N; i++)
    {
        string regstr("F");
        int regnum = i * 2;
        regstr += to_string(regnum);
        Register_t reg;
        reg.fu = -1;
        reg.value = regnum;
        Register.insert(make_pair(regstr, reg));
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
        Register_t reg;
        reg.fu = -1;
        reg.value = regnum;
        Register.insert(make_pair(regstr, reg));
    }
}
/*
ld rd, offset(rt)
sd rt, offset(rd)
add rd, rs, rt
 */
void init_Instruction(string filename)
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
        iss >> opstr;
        if (opstr == "L.D"){
            int rs; rsstr = "";
            iss >> rdstr >> rs >> rtstr;

            in.offset = rs;
            rdstr.erase(rdstr.size() - 1);
            rtstr.erase(rtstr.begin());
            rtstr.erase(rtstr.size()-1);
        }
        else if(opstr == "S.D"){
            int rs; rsstr = "";
            iss >> rtstr >> rs >> rdstr;

            in.offset = rs;
            rtstr.erase(rtstr.size() - 1);
            rdstr.erase(rdstr.begin());
            rdstr.erase(rdstr.size()-1);
        }
        else{
            iss >> rdstr >> rsstr >> rtstr;
            rdstr.erase(rdstr.size() - 1);
            rsstr.erase(rsstr.size()-1);
        }
        
        in.opcode = opstr; in.rd = rdstr; in.rs= rsstr; in.rt = rtstr;
        Instruction.push_back(in);
    }
    ifile.close();
}

void print_Instruction()
{
    for(auto &it: Instruction)
    {
        cout << it.opcode << " " << it.rd << " ";
        if(!it.opcode.compare("L.D") || !it.opcode.compare("S.D")){
            cout << it.offset;
        }
        else{
            cout << it.rs;
        }
        cout << " " << it.rt << endl;
    }
}
void init_ReservationStation()
{
    for(int i = 0; i < FU_N; i++){
        ReservationStation[i].busy = false;
        ReservationStation[i].cycle = -1;
        ReservationStation[i].Qi = -1;
        ReservationStation[i].Qj = -1;
        ReservationStation[i].Qk = -1;
    }
}
void init_ReservationStation(int fu)
{
    ReservationStation[fu].busy = false;
    ReservationStation[fu].cycle = -1;
    ReservationStation[fu].Qi = -1;
    ReservationStation[fu].Qj = -1;
    ReservationStation[fu].Qk = -1;
}
void init_Cycle()
{
    Cycle.insert(make_pair("L.D", LD_C));
    Cycle.insert(make_pair("S.D", SD_C));
    Cycle.insert(make_pair("ADD.D", ADD_C));
    Cycle.insert(make_pair("SUB.D", SUB_C));
    Cycle.insert(make_pair("MUL.D", MUL_C));
    Cycle.insert(make_pair("DIV.D", DIV_C));
}
void store_ins_to_res(int fu)
{
    Instruction_t &ins = Instruction[PC];
    ReservationStation_t &res = ReservationStation[fu];
    ins.Issue = Clock;

    string op = ins.opcode;
    res.busy = true;
    res.opcode = op;
    res.ins = ins;
    if(op == "L.D" || op == "S.D"){
        res.Vj = ins.offset; // ld or sd
        res.cycle = LD_C;  // ld and sd have same cycle
        // find rd whether has WAW hazard
        if (Register[ins.rd].fu >= 0) // hazard occur
            res.Qi = Register[ins.rd].fu;
        // find rt whether has RAW hazard
        if (Register[ins.rt].fu >= 0) // hazard occur
            res.Qk = Register[ins.rt].fu;
        else
            res.Vk = Register[ins.rt].value;
    }
    else{
        /* map find method
        // find rs whether has RAW hazard
        auto rs = Register.find(ins.rs);
        if (rs == Register.end())
            cout << op << " issue rs not found" << endl;
        if(rs->second.fu > 0)  // hazard occur
            res.Qj = rs->second.fu;
        else
            res.Vj = rs->second.fu;
        // find rt whether has RAW hazard
        auto rt = Register.find(ins.rt);
        if (rt == Register.end())
            cout << op << " issue rs not found" << endl;
        if (rt->second.fu > 0) // hazard occur
            res.Qk = rt->second.fu;
        else
            res.Vk = rt->second.fu;
        */
        // find rd whether has WAW hazard
        if (Register[ins.rd].fu >= 0) // hazard occur
            res.Qi = Register[ins.rd].fu;
        // find rs whether has RAW hazard
        if (Register[ins.rs].fu >= 0) // hazard occur
            res.Qj = Register[ins.rs].fu;
        else
            res.Vj = Register[ins.rs].value;
        // find rt whether has RAW hazard
        if (Register[ins.rt].fu >= 0) // hazard occur
            res.Qk = Register[ins.rt].fu;
        else
            res.Vk = Register[ins.rt].value;
    }
    // update register function unit
    Register[ins.rd].fu = fu;
}
// if nothing to write result, return true
bool WriteResult()
{
    bool wrote = false;

}
// if nothing to execute, return true
bool Execute()
{
    bool executed = false;  // if executed set as true
    // ld or sd
    for(int i = LoadBuffer0; i <= StoreBuffer1; i++){
        ReservationStation_t &res = ReservationStation[i];
        switch(res.cycle){
            // put rt value into res
            case 2:
                res.Vk = Register[res.ins.rt].value;
                --res.cycle;
                executed = true;
                break;
            // execute
            case 1:
                res.Vi = res.Vj + res.Vk;
                --res.cycle;
                res.ins.Execution = Clock;
                executed = true;
                break;
        }
    }
    for(int i = Adder0; i <= Multiplier1; i++){
        ReservationStation_t &res = ReservationStation[i];
        switch(res.cycle){
            // rs or rt is not ready
            case -1:
                if(res.Qj >= 0 && Register[res.ins.rs].fu != i){
                    res.Vj = Register[res.ins.rs].value;
                    res.Qj = -1;
                }
                    
                if(res.Qk >= 0 && Register[res.ins.rt].fu != i){
                    res.Vk = Register[res.ins.rt].value;
                    res.Qk = -1;
                }
                if(res.Qj == -1 && res.Qk == -1){
                    res.cycle = Cycle[res.opcode];
                    executed = true;
                }
                break;
            case 0:
                res.ins.Execution = Clock;
                executed = true;
                break;
            default:
                --res.cycle;
                executed = true;
        }
    }
    return !executed;
}
// if nothing to issue, return true
bool Issue()
{
    bool issued = true;
    if(Instruction[PC].opcode == "L.D"){
        if(!ReservationStation[LoadBuffer0].busy)
            store_ins_to_res(LoadBuffer0);
        else if(!ReservationStation[LoadBuffer1].busy)
            store_ins_to_res(LoadBuffer1);
        else
            issued = false;
    }
    else if(Instruction[PC].opcode == "S.D"){
        if(!ReservationStation[StoreBuffer0].busy)
            store_ins_to_res(StoreBuffer0);
        else if (!ReservationStation[StoreBuffer1].busy)
            store_ins_to_res(StoreBuffer1);
        else
            issued = false;
    }
    else if(Instruction[PC].opcode == "ADD.D"){
        if(!ReservationStation[Adder0].busy)
            store_ins_to_res(Adder0);
        else if (!ReservationStation[Adder1].busy)
            store_ins_to_res(Adder1);
        else if (!ReservationStation[Adder2].busy)
            store_ins_to_res(Adder2);
        else
            issued = false;
    }
    else if(Instruction[PC].opcode == "MUL.D"){
        if(!ReservationStation[Multiplier0].busy)
            store_ins_to_res(Multiplier0);
        else if (!ReservationStation[Multiplier1].busy)
            store_ins_to_res(Multiplier1);
        else
            issued = false;
    }
    return !issued;
}
int main()
{
    //初始化各個結構
    //init_regtable();
    //init_optable();
    string filename = ".\\doc\\test2";
    init_Instruction(filename);
    print_Instruction();
    init_Cycle();
    init_Register();
    init_ReservationStation();
    //何時離開主迴圈? WriteResult()、Execute()、Issue()皆無推進任何一個執令，或是struct Instruction中的Issue、Execution、Write皆非原初始值。
    while (1)
    {
        bool finished = true;
        finished &= WriteResult(); //執行已完成Execution步驟的指令
        finished &= Execute();     //執行已完成Issue步驟的指令，並在維護每個正在本步驟的指令
        finished &= Issue();       //issue下一個未被issue的指令
        if(finished)
            break;
        Clock++;
    }
    
    return 0;
}
