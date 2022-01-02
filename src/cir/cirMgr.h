/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <utility>

using namespace std;

#include "cirDef.h"
 
extern CirMgr *cirMgr;
class AigGate;
class PiGate;
class PoGate;
class UndefGate;


// TODO: Define your own data members and member functions
class CirMgr
{
    friend CirGate;
    friend AigGate;
    friend PiGate;
    friend PoGate;
    friend UndefGate;
public:
    CirMgr(){
        for (int i = 0; i < 5; ++i)
            _miloa[i] = 0;
    }
    ~CirMgr() { reset(); }

    // Access functions
    // return '0' if "gid" corresponds to an undefined gate.
    CirGate* getGate(unsigned gid) const;
    // Member functions about circuit construction
    bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   bool isValid(string &line);
   void writeSimLog();
   void buildFECs(bool &isSimed);
   size_t hashFECkey(size_t &simVal);
   void fecs_sort();
   static bool compare_comp(IdList &l, IdList &r);
   void fecMap_sort();

   // Member functions about fraig
   void strash();
   size_t genHashKey(CirGate *&g);
   void printFEC() const;
   void fraig();

    // Member functions about circuit reporting
    void printSummary() const;
    void printNetlist() const;
    void printPIs() const;
    void printPOs() const;
    void printFloatGates() const;
    void printFECPairs() const;
    void writeAag(ostream&) const;
    void writeGate(ostream&, CirGate*) const;

    //mine
    void replace(CirGate* const &remove, CirGate* const &remain, CirGate* &g);
    void removeGate(CirGate* &g);
    

private:
    //for Sim
    vector <IdList> _realFECgroup;
    //unordered_map <size_t, IdList> _fecMap;
    //vector<IdList*> _Keys_Idlistptr;
    //unordered_map<size_t, IdList>::iterator _fecMapIt;
    //vector< IdList* > _fecs;
    //bool _isSimed;
    void simAllGate();
    int _simCount;
    size_t _lineNum;
    unsigned nFECgroup;
    ofstream    *_simLog;
    
 
    int _miloa[5] ;
    //maintain gates' order
    GateList _gateList; // store the actual address
    GateList _dfsList;  // store the address & isNeg on last bit
    IdList _piIdList; // for print -pi (po already ordered in _gateList)
    IdList _floatIdList;
    IdList _notUsedIdList;
    static CirGate* const0;

    // private helper function
    bool readHeader(ifstream&);
    bool readIO(ifstream&, unsigned); // unsigned specified I/O
    bool readAig(ifstream&);
    bool readSymbol(ifstream&);
    bool readComment(ifstream&);
    void connect();
    void reset();    
    void genDFSList();
    void genNotUsedList();

    
};

#endif // CIR_MGR_H
