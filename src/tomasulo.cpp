#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <set>
#include <iomanip>

using namespace std;

enum FuntionUnit : int{
    LoadBuffer0, LoadBuffer1,
    StoreBuffer0, StoreBuffer1,
    Adder0, Adder1, Adder2,
    Multiplier0, Multiplier1
};
typedef struct{
    string opcode;
    string rs;
    string rt;
    string rd;
    int offset;
    int Issue; //紀錄完成該步驟的迴圈
    int Execution;
    int Write;
    int line;
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
    Instruction_t *ins;
    bool last_writeback;
    int qi;
    int qj;
    int qk;
} ReservationStation_t;

typedef struct{
    int fu; // if not used, set as -1
    double value;
} Register_t;

#define FU_N 9  // Funtion Unit number
#define FR_N 16 // float point register number
#define IR_N 32 // integer register number
#define LD_C 2  // ld cycle
#define SD_C 1  // sd cycle
#define ADD_C 2 // add cycle
#define SUB_C 2 // sub cycle
#define MUL_C 10 // mul cycle
#define DIV_C 40 // div cycle

ReservationStation_t ReservationStation[FU_N];
map<string, Register_t> Register;  // use to get function unit and register value from register string
vector<Instruction_t> Instruction;
map<uint64_t, int> Memory;
map<string, int> Cycle{{"L.D", LD_C}, {"S.D", SD_C}, {"ADD.D", ADD_C}, 
    {"SUB.D", SUB_C}, {"MUL.D", MUL_C}, {"DIV.D", DIV_C}};
int Clock = 0;
int PC = 0;
void init_Register()
{
    for (int i = 0; i < FR_N; i++)
    {
        string regstr("F");
        int regnum = i * 2;
        regstr += to_string(regnum);
        Register_t reg;
        reg.fu = -1;
        reg.value = 1;
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
    int line = 0;
    ifstream ifile(filename, ifstream::in);
    if(!ifile){
        cout << "input file open failed" << endl;
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
        in.Issue = -1; in.Execution = -1; in.Write = -1;
        in.line = line;
        Instruction.push_back(in);
        ++line;
    }
    ifile.close();
}
void print(string filename)
{
    ofstream ofile(filename, ifstream::app);
    if(!ofile){
        cout << "output file open failed" << endl;
    }
    ofile.setf(ios::left);
    ofile << "Cycle " << Clock << endl << endl;
    ofile << setw(30) << "Instruction:" << setw(10) << "Issue" << setw(15) << "Exec_comp" << setw(15) << "Write_back" << endl;
    // print Instruction
    for(auto &it :Instruction){
        if(it.opcode == "L.D")
            ofile << setw(7) << it.opcode << setw(5) << it.rd << setw(5) << it.offset << setw(13) << it.rt
              << setw(10) << it.Issue << setw(15) << it.Execution << setw(15) << it.Write << endl;
        else if(it.opcode == "S.D")
            ofile << setw(7) << it.opcode << setw(5) << it.rt << setw(5) << it.offset << setw(13) << it.rd
              << setw(10) << it.Issue << setw(15) << it.Execution << setw(15) << it.Write << endl;
        else
            ofile << setw(7) << it.opcode << setw(5) << it.rd << setw(5) << it.rs << setw(13) << it.rt
                << setw(10) << it.Issue << setw(15) << it.Execution << setw(15) << it.Write << endl;
    }
    // print function unit
    ofile << endl;
    ofile << setw(7) << "Time" << setw(7) << "Name" << setw(7) << "Busy" << setw(7) << "Op" << setw(10) << "Vj" << setw(10) << "Vk" << setw(10) << "Qj" << setw(10) << "Qk" << endl;
    for(int i = LoadBuffer0; i <= LoadBuffer1; i++){
        ReservationStation_t &res = ReservationStation[i];
        ofile << setw(7) << res.cycle << setw(7) << "LOAD" + to_string(i-LoadBuffer0+1) << setw(7) << to_string(ReservationStation[i].busy) << setw(7) << 
            res.opcode << setw(10) << res.Vj << setw(10) << res.Vk << setw(10) << res.Qj << setw(10) << res.Qk << endl;
    }
    ofile << endl;
    for(int i = StoreBuffer0; i <= StoreBuffer1; i++){
        ReservationStation_t &res = ReservationStation[i];
        ofile << setw(7) << res.cycle << setw(7) << "STORE" + to_string(i-StoreBuffer0+1) << setw(7) << to_string(ReservationStation[i].busy) << setw(7) << 
            res.opcode << setw(10) << res.Vj << setw(10) << res.Vk << setw(10) << res.Qj << setw(10) << res.Qk << endl;
    }
    ofile << endl;
    for(int i = Adder0; i <= Adder2; i++){
        ReservationStation_t &res = ReservationStation[i];
        ofile << setw(7) << res.cycle << setw(7) << "ADD" + to_string(i-Adder0+1) << setw(7) << to_string(ReservationStation[i].busy) << setw(7) << 
            res.opcode << setw(10) << res.Vj << setw(10) << res.Vk << setw(10) << res.Qj << setw(10) << res.Qk << endl;
    }
    ofile << endl;
    for(int i = Multiplier0; i <= Multiplier1; i++){
        ReservationStation_t &res = ReservationStation[i];
        ofile << setw(7) << res.cycle << setw(7) << "MUL" + to_string(i-Multiplier0+1) << setw(7) << to_string(ReservationStation[i].busy) << setw(7) << 
            res.opcode << setw(10) << res.Vj << setw(10) << res.Vk << setw(10) << res.Qj << setw(10) << res.Qk << endl;
    }
    ofile << endl << "Register Result Status" << endl;
    for(auto &it :Register){
        string f(it.first);
        char c = f[0];
        char tmp = 'F';
        if(c == tmp)
            ofile << setw(10) << it.first;
    }
    ofile << endl;
    for(auto &it :Register){
        string f(it.first);
        char c = f[0];
        char tmp = 'F';
        if(c == tmp)
            ofile << setw(10) << it.second.fu;
    }
    ofile << endl;
    for(auto &it :Register){
        string f(it.first);
        char c = f[0];
        char tmp = 'F';
        if(c == tmp)
            ofile << setw(10) << it.second.value;
    }
    ofile << endl << endl << "Integer Register" << endl;
    for(auto &it :Register){
        string f(it.first);
        char c = f[0];
        char tmp = 'R';
        if(c == tmp)
            ofile << setw(5) << it.first;
    }
    ofile << endl;
    for(auto &it :Register){
        string f(it.first);
        char c = f[0];
        char tmp = 'R';
        if(c == tmp)
            ofile << setw(5) << it.second.value;
    }
    ofile << endl << endl << "Memory" << endl;
    for(auto &it :Memory)
        ofile << setw(10) << it.first;
    ofile << endl;
    for(auto &it :Memory)
        ofile << setw(10) << it.second;
    ofile << endl << "-----------------------------------------------------" << endl;
    ofile.close();
}
void init_ReservationStation(int fu)
{
    ReservationStation[fu] = {false, "", -1, -1, -1, -1, -1, -1, -1, NULL, true, -1, -1, -1};
}
void init_ReservationStation()
{
    for(int i = 0; i < FU_N; i++){
        ReservationStation[i] = {false, "", -1, -1, -1, -1, -1, -1, -1, NULL, false, -1, -1, -1};
    }
}
float execute_operation(string op, float rs, float rt)
{
    if(op == "L.D" || op == "S.D" || op == "ADD.D")
        return rs + rt;
    else if(op == "SUB.D")
        return rs - rt;
    else if(op == "MUL.D")
        return rs * rt;
    else
        return rs / rt;
}
// if nothing to write result, return true
bool WriteResult()
{
    bool wrote = false;
    for(int i = LoadBuffer0; i <= Multiplier1; i++){
        ReservationStation_t &res = ReservationStation[i];
        // check
        if(res.ins == NULL)
            continue;
        if(res.opcode == "S.D"){
            Register_t &rdreg = Register[res.ins->rt];
            if(res.cycle == 0 && (res.qk == -1 || Instruction[res.qk].Write > 0)){
                // write result
                Memory[(uint64_t)res.Vi] = rdreg.value;
                res.ins->Write = Clock;
                if(rdreg.fu == i)
                    rdreg.fu = -1;
                init_ReservationStation(i);
                wrote = true;
            }
        }
        else{
            Register_t &rdreg = Register[res.ins->rd];
            if(res.cycle == 0 && (res.qi == -1 || Instruction[res.qi].Write > 0)){
                // write result
                if(i == LoadBuffer0 || i == LoadBuffer1){
                    int value = 1;
                    auto it = Memory.find((uint64_t)res.Vi);
                    if (it != Memory.end())
                        value = it->second;
                    rdreg.value = value;
                }
                else
                    rdreg.value = res.Vi;
                res.ins->Write = Clock;
                if (rdreg.fu == i)
                    rdreg.fu = -1;
                init_ReservationStation(i);
                wrote = true;
            }
        }
    }
    return !wrote;
}
// if nothing to execute, return true
bool Execute()
{
    bool executed = false;  // if executed set as true
    // ld
    for(int i = LoadBuffer0; i <= LoadBuffer1; i++){
        ReservationStation_t &res = ReservationStation[i];
        if(res.ins == NULL)
            continue;
        switch(res.cycle){
            // RAW hazard
            case -1:
                if(res.Qk >=0 && Register[res.ins->rt].fu != res.Qk){
                    res.Vk = Register[res.ins->rt].value;
                    res.Qk = -1;
                    res.cycle = LD_C;
                    executed = true;
                }
                break;
            // put rt value into res
            case LD_C:
                res.Vk = Register[res.ins->rt].value;
                --res.cycle;
                executed = true;
                break;
            // execute
            case 1:
                res.Vi = execute_operation(res.opcode, res.Vj, res.Vk);
                --res.cycle;
                res.ins->Execution = Clock;
                executed = true;
                break;
        }
    }
    // sd
    for(int i = StoreBuffer0; i <= StoreBuffer1; i++){
        ReservationStation_t &res = ReservationStation[i];
        if(res.ins == NULL)
            continue;
        switch(res.cycle){
            // execute
            case SD_C:
                res.Vk = Register[res.ins->rd].value;
                res.Vi = execute_operation(res.opcode, res.Vj, res.Vk);
                --res.cycle;
                res.ins->Execution = Clock;
                executed = true;
                break;
        }
    }
    for(int i = Adder0; i <= Multiplier1; i++){
        ReservationStation_t &res = ReservationStation[i];
        if (res.ins == NULL)
            continue;
        switch(res.cycle){
            // rs or rt is not ready
            case -1:
                if(res.Qj >= 0 && Register[res.ins->rs].fu != res.Qj){
                    res.Vj = Register[res.ins->rs].value;
                    res.Qj = -1;
                }
                if(res.Qk >= 0 && Register[res.ins->rt].fu != res.Qk){
                    res.Vk = Register[res.ins->rt].value;
                    res.Qk = -1;
                }
                if(res.Qj == -1 && res.Qk == -1){
                    res.cycle = Cycle[res.opcode];
                    executed = true;
                }
                break;
            case 1:
                res.Vi = execute_operation(res.opcode, res.Vj, res.Vk);
                res.ins->Execution = Clock;
                --res.cycle;
                executed = true;
                break;
            default:
                --res.cycle;
                executed = true;
        }
    }
    return !executed;
}
void store_ins_to_res(int fu)
{
    Instruction_t &ins = Instruction[PC];
    ReservationStation_t &res = ReservationStation[fu];
    ins.Issue = Clock;

    string op = ins.opcode;
    res.busy = true;
    res.opcode = op;
    res.ins = &ins;
    if (op == "L.D"){
        res.Vj = ins.offset; // ld or sd
        res.cycle = LD_C;    // ld and sd have same cycle
        // find rd whether has WAW hazard, but not consider sd's hazard
        if (Register[ins.rd].fu >= 0){ // hazard occur
            res.Qi = Register[ins.rd].fu;
            res.qi = ReservationStation[res.Qi].ins->line;
        }
        // find rt whether has RAW hazard, but not consider sd's hazard
        if (Register[ins.rt].fu >= 0){  // hazard occur
            res.Qk = Register[ins.rt].fu;
            res.cycle = -1;
        }
    }
    else if (op == "S.D"){
        res.Vj = ins.offset; // ld or sd
        res.cycle = SD_C;    // ld and sd have same cycle
        // find rd whether has WAW hazard, but not consider sd's hazard
        /*
        if (Register[ins.rd].fu >= 0) // hazard occur
            res.Qi = Register[ins.rd].fu;
        */
        // find rt whether has RAW hazard, but not consider sd's hazard
        if (Register[ins.rt].fu >= 0){  // hazard occur
            res.Qk = Register[ins.rt].fu;
            res.qk = ReservationStation[res.Qk].ins->line;
        }
    }
    else{
       res.cycle = Cycle[res.opcode];
        // find rd whether has WAW hazard
        if (Register[ins.rd].fu >= 0){ // hazard occur
            res.Qi = Register[ins.rd].fu;
            res.qi = ReservationStation[res.Qi].ins->line;
        }
        // find rs whether has RAW hazard
        if (Register[ins.rs].fu >= 0){ // hazard occur
            res.Qj = Register[ins.rs].fu;
            res.cycle = -1;
        }
        else
            res.Vj = Register[ins.rs].value;
        // find rt whether has RAW hazard
        if (Register[ins.rt].fu >= 0){ // hazard occur
            res.Qk = Register[ins.rt].fu;
            res.cycle = -1;
        }
        else
            res.Vk = Register[ins.rt].value;
    }
    // update register function unit
    if(ins.opcode != "S.D")
        Register[ins.rd].fu = fu;
    ++PC;
}
// if nothing to issue, return true
bool Issue()
{
    if(Instruction.size() <= PC)
        return true;
    bool issued = true;
    if(Instruction[PC].opcode == "L.D"){
        if(!ReservationStation[LoadBuffer0].busy && !ReservationStation[LoadBuffer0].last_writeback)
            store_ins_to_res(LoadBuffer0);
        else if (!ReservationStation[LoadBuffer1].busy && !ReservationStation[LoadBuffer1].last_writeback)
            store_ins_to_res(LoadBuffer1);
        else
            issued = false;
    }
    else if(Instruction[PC].opcode == "S.D"){
        if (!ReservationStation[StoreBuffer0].busy && !ReservationStation[StoreBuffer0].last_writeback)
            store_ins_to_res(StoreBuffer0);
        else if (!ReservationStation[StoreBuffer1].busy && !ReservationStation[StoreBuffer1].last_writeback)
            store_ins_to_res(StoreBuffer1);
        else
            issued = false;
    }
    else if(Instruction[PC].opcode == "ADD.D" || Instruction[PC].opcode == "SUB.D"){
        if (!ReservationStation[Adder0].busy && !ReservationStation[Adder0].last_writeback)
            store_ins_to_res(Adder0);
        else if (!ReservationStation[Adder1].busy && !ReservationStation[Adder1].last_writeback)
            store_ins_to_res(Adder1);
        else if (!ReservationStation[Adder2].busy && !ReservationStation[Adder2].last_writeback)
            store_ins_to_res(Adder2);
        else
            issued = false;
    }
    else if(Instruction[PC].opcode == "MUL.D" || Instruction[PC].opcode == "DIV.D"){
        if (!ReservationStation[Multiplier0].busy && !ReservationStation[Multiplier0].last_writeback)
            store_ins_to_res(Multiplier0);
        else if (!ReservationStation[Multiplier1].busy && !ReservationStation[Multiplier1].last_writeback)
            store_ins_to_res(Multiplier1);
        else
            issued = false;
    }
    return !issued;
}
int main(int argc, char *argv[])
{
    //初始化各個結構
    string filename(argv[1]);
    string inputfile = ".\\doc\\" + filename;
    string outputfile = inputfile + "_output.txt";
    init_Instruction(inputfile);
    init_Register();
    init_ReservationStation();
    //何時離開主迴圈? WriteResult()、Execute()、Issue()皆無推進任何一個執令，或是struct Instruction中的Issue、Execution、Write皆非原初始值。
    while (1)
    {
        ++Clock;
        bool finished = true;
        finished &= WriteResult(); //執行已完成Execution步驟的指令
        finished &= Execute();     //執行已完成Issue步驟的指令，並在維護每個正在本步驟的指令
        finished &= Issue();       //issue下一個未被issue的指令
        for(int i = LoadBuffer0; i <= Multiplier1; i++)
            ReservationStation[i].last_writeback = false;
        if(finished)
            break;
        print(outputfile);
    }
    
    return 0;
}
