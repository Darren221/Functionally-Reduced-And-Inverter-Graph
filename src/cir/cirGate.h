/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include "cirDef.h"
#include "sat.h"
#include <utility>
#include <unordered_map>

using namespace std;
 
class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: Define your own data members and member functions, or classes
class CirGate
{
    #define NEG 0x1
    friend CirMgr;
public:
    CirGate(){}
    CirGate(unsigned id, int lineNo, IdList input = IdList()): _lineNo(lineNo), _id(id), _ref(0){
        size_t size = input.size();
        _faninList.resize(size);
        for (size_t i = 0; i < size; ++i) 
            _faninList[i] = (CirGate*)(size_t)input[i] ;       
    } // input is a list of unsigned int
    virtual ~CirGate() {}

    // Basic access methods
    virtual string getTypeStr() const = 0;
    unsigned getLineNo() const { return _lineNo; }
    virtual bool isAig() const { return false; }

    // Printing functions
    virtual void printGate() const = 0;
    void reportGate() const;
    void reportFanin(int level) const ;
    void reportFanout(int level) const ;

    // netlist function
    void pushFanout(CirGate* fanout){ _fanoutList.push_back(fanout) ;}
    void dfsTraversal() const;
    void preOrderPrint(int level, int& flag, int iter = 0) const;
    bool setSymbol(string sym){ 
        if(_symbol.empty()){ 
            _symbol = sym;
            return true;
        }
        return false;
    }
    virtual bool defNotUsed() = 0;
    virtual void buildConnect() = 0;
    virtual CirGate* getInGate(unsigned idx=0) = 0;
    static void setGlobalRef() { ++_globalRef; }
    static bool isInv(CirGate* fanin) { return size_t(fanin) & NEG ;}
    static CirGate* getGate(CirGate* fanin) { return (CirGate*)( size_t(fanin) & ~(NEG) );}
    static void setInv(CirGate* faninout, bool isInv) { //fanin is pure this line
      if(isInv)
        faninout = (CirGate*)( size_t(faninout) | (NEG) );
      else
        faninout = (CirGate*)( size_t(faninout) & ~(NEG) );
    } 
    void setFanin(CirGate* fanin, unsigned idx, bool isInv) //fanin is pure this line
    {
        if(isInv)
            fanin = (CirGate*)( size_t(fanin) | (NEG) );
        else
            fanin = (CirGate*)( size_t(fanin) & ~(NEG) );
        _faninList[idx] = fanin;
    }
    CirGate* getFanin(unsigned idx) { return (CirGate*)( size_t(_faninList[idx]) & ~(NEG) ); }
    bool getInInv(unsigned idx) {return size_t(_faninList[idx]) & NEG ; }
    void setFanout(CirGate* fanout, bool isInv)//fanout is pure this line
    {
        setInv(fanout,isInv);
        _fanoutList.push_back(fanout);
    }
    CirGate* getFanout(unsigned idx) { return (CirGate*)( size_t(_fanoutList[idx]) & ~(NEG) ); }
    bool getOutInv(unsigned idx) {return size_t(_fanoutList[idx]) & NEG ; }

    //for Sim
    void simulate();
    
private:
    static unsigned _globalRef;

protected: 
    // last bit in fanin store inverted/not
    GateList _faninList;   
    GateList _fanoutList;
    int _lineNo;
    unsigned _id;
    mutable unsigned _ref;
    string _symbol;
    uint8_t _typeID;
    //for Sim
    size_t _simVal;

    void setToGlobalRef() const { _ref = CirGate::_globalRef; }
    bool isGlobalRef() const { return _ref==CirGate::_globalRef; }
};

class AigGate: public CirGate
{
public:
    AigGate(unsigned id, int lineNo, IdList varList): CirGate( id, lineNo, varList){_typeID = AIG_GATE;}
    ~AigGate(){}
    // access methods
    string getTypeStr() const { return "AIG"; }

    // printing functions
    void buildConnect();
    void printGate() const;
    bool defNotUsed(){return _fanoutList.empty();}
    CirGate* getInGate(unsigned idx = 0){
        return getGate(_faninList[idx]);
    }

private:
};

class PiGate: public CirGate
{
public:
    PiGate(unsigned id, int lineNo): CirGate(id, lineNo) {
        if(_id!=0)_typeID = PI_GATE;
        else _typeID = CONST_GATE;
    }
    PiGate(): CirGate(){ _lineNo = 0; _id = 0;} // for const0 gate
    ~PiGate(){}
    string getTypeStr() const { 
        if(_id!=0) return "PI";
        else return "CONST";
    }
    void buildConnect(){}; // do nothing
    void printGate() const;
    bool defNotUsed(){return _fanoutList.empty();}
    CirGate* getInGate(unsigned idx=0) { return 0;}

private:
};

class PoGate:public CirGate
{
public:
    PoGate(unsigned id, int lineNo, IdList varList): CirGate(id, lineNo, varList) {_typeID = PO_GATE;}
    ~PoGate(){}
    string getTypeStr() const { return "PO"; }
    void buildConnect();
    void printGate() const;
    bool defNotUsed(){return false;}
    CirGate* getInGate(unsigned idx=0){ return getGate(_faninList[0]);}
private:
};

// serve as a dummy class
class UndefGate: public CirGate
{
public:
    UndefGate(unsigned id): CirGate(id, 0) {_typeID = UNDEF_GATE;}
    ~UndefGate(){}
    string getTypeStr() const { return "UNDEF"; }
    void buildConnect(){};
    void printGate() const{}; // do nothing
    bool defNotUsed(){return false;}
    CirGate* getInGate(unsigned idx=0){ return 0;}

private:
};


#endif // CIR_GATE_H
