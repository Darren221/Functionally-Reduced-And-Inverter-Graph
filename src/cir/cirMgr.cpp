/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <algorithm>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;
 
// TODO: Implement memeber functions for class CirMgr
#define PHASE(var) var%2
#define ID(var) unsigned(var/2)

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr;
CirGate* CirMgr::const0 = new PiGate();

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE, 
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;
static bool parseError(CirParseError);


static bool
myIsPrint(const string& symbol){
    for (int i = 0; i < (int)symbol.length(); i++,colNo++) {
        if(!isprint(symbol[i])){
            errInt = symbol[i];
            return parseError(ILLEGAL_SYMBOL_NAME) ;
        }
    }
    return true;
}

static bool
lexOptions(const string &option, vector<string> &tokens, size_t nOpts) 
{
    string token;
    size_t n = myStrGetTok(option, token);
    while (token.size())
    {
        tokens.push_back(token);
        n = myStrGetTok(option, token, n);
    }
    if (nOpts != 0)
    {
        if (tokens.size() < nOpts)
        {
            return false;
        }
        if (tokens.size() > nOpts)
        {
            return false;
        }
    }
    return true;
} 
static void
resetStaticVar(){
    errInt = lineNo = colNo = 0;
    memset(buf, 0, sizeof(buf));
    errMsg = "";
    errGate = 0;
}

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine constant (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

static bool
checkSpace(const char* buf){
    if(buf[0]==' ') return parseError(EXTRA_SPACE);
    if(buf[0]!=0){
        for (size_t i = 0; i < strlen(buf)-1; ++i) {
            if(buf[i]=='\t'){
                colNo = i;
                errInt = '\t';
                return parseError(ILLEGAL_WSPACE);
            }
            if(buf[i]==' ' && buf[i+1]==' '){
                colNo = i+1;
                return parseError(EXTRA_SPACE);
            }
        }
    }
    return true;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
    ifstream cirFile(fileName);
    if(!cirFile){
        cout << "Cannot open design \"" << fileName << "\"!!" << endl;
        return false;
    }
    if(!readHeader(cirFile)|| !readIO(cirFile, 0) || !readIO(cirFile, 1) || 
       !readAig(cirFile)   || !readSymbol(cirFile)|| !readComment(cirFile)){
        reset();
        cirFile.close();
        resetStaticVar();
        return false;
    }
    cirFile.close();
    // building connection & check floating, unused, undef etc.
    connect(); 
    genDFSList();
    genNotUsedList();
    sort(_floatIdList.begin(), _floatIdList.end());
    sort(_notUsedIdList.begin(), _notUsedIdList.end());
    return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
    cout << '\n';
    cout << "Circuit Statistics\n" <<
            "==================\n" <<
            "  PI   " << setw(9)   << right << _miloa[1] << endl <<
            "  PO   " << setw(9)   << right << _miloa[3] << endl <<
            "  AIG  " << setw(9)   << right <<_miloa[4] << endl <<
            "------------------\n" << 
            "  Total" << setw(9)   << _miloa[1]+_miloa[3]+_miloa[4] << endl; 
}

void
CirMgr::printNetlist() const
{
    int cnt=0;
    cout << endl;
    for (auto i: _dfsList) {
        if(i->getTypeStr()!="UNDEF"){
            cout << "[" << cnt << "] "; 
            i->printGate();
            ++cnt;
        }
    }
}

void
CirMgr::printPIs() const
{
    int size = _piIdList.size();
    cout << "PIs of the circuit: ";
    for (int i=0 ; i<size ; ++i) {
        cout << _piIdList[i];
        if(i<size-1)
            cout << " " ;
    }
    cout << endl;
}

void
CirMgr::printPOs() const
{
    cout << "POs of the circuit: ";
    for (int i = 0; i < _miloa[3] ; ++i) {
        if(_gateList[i+_miloa[0]+1]->getTypeStr() == "UNDEF")
            break;
        cout << i+_miloa[0]+1;
        if(i!=_miloa[3]-1)
            cout << " " ;
    }
    cout << endl;
}

void
CirMgr::printFloatGates() const
{
    if(!_floatIdList.empty()){
        cout << "Gates with floating fanin(s): ";
        for(auto i=_floatIdList.begin(); i!=_floatIdList.end(); ++i){
            cout << *i ;
            if(i!=_floatIdList.end()-1) cout << " ";
        }
        cout << endl;
    }
    if(!_notUsedIdList.empty()){
        cout << "Gates defined but not used  : ";
        for(auto i=_notUsedIdList.begin(); i!=_notUsedIdList.end(); ++i){
            cout << *i ;
            if(i!=_notUsedIdList.end()-1) cout << " ";
        }
    }
}

void
CirMgr::printFECPairs() const
{
    size_t first_id = 0;
    for(unsigned i = 0 ; i < nFECgroup ; ++i)
    {
        cout << "[" << i << "]";
        for(size_t j = 0, n = _realFECgroup.size() ; j < n ; ++j)
        {
            if(_realFECgroup[j].size() <= 1) continue;
            first_id = _gateList[ ( _realFECgroup[j][0] ) ]->_simVal;
            for(size_t k = 0 ; k < _realFECgroup[j].size() ; ++k)
            {
                cout << " ";
                if( _gateList[ ( _realFECgroup[j][k] ) ]->_simVal != first_id ) cout << "!";
                cout << ( _realFECgroup[j][k] );
            }

        }
        cout << endl;
    }
    
    /*
    for(size_t i = 0, n = _Keys_Idlistptr.size() ; i < n ; ++i)
    {
        cout << "[" << i << "]";
        first_id = _gateList[ ( _Keys_Idlistptr[i]->at(0) ) ]->_simVal;
        for(size_t j = 0, n = (*_Keys_Idlistptr[i]).size() ; j < n ; ++j)
        {
            cout << " ";
            if( _gateList[ ( _Keys_Idlistptr[i]->operator[](j) ) ]->_simVal != first_id ) cout << "!";
            cout << ( _Keys_Idlistptr[i]->operator[](j) );
        }
        cout << endl;
    }
    */
    
}

void
CirMgr::writeAag(ostream& outfile) const
{
    int cnt=0;
    for (auto i: _dfsList) {
        if(i->getTypeStr()=="AIG")
            cnt+=1;
    }
    outfile << "aag" << " ";
    outfile << _miloa[0] << " " << _miloa[1] << " " <<_miloa[2] << " "
            << _miloa[3] << " " << cnt;
    for (auto i: _piIdList){
        outfile << endl;
        outfile << 2*i;
    }
    for (int i=0; i<_miloa[3]; ++i){
        outfile << endl;
        CirGate* thisG = _gateList[i+_miloa[0]+1];
        outfile << 2*(thisG->getInGate()->_id) + CirGate::isInv(thisG->_faninList[0]);  
    }
    for (auto i: _dfsList){
        if(i->getTypeStr()=="AIG"){
            outfile << endl;
            unsigned var1 = 2*i->getInGate(0)->_id + CirGate::isInv(i->_faninList[0]) ;
            unsigned var2 = 2*i->getInGate(1)->_id + CirGate::isInv(i->_faninList[1]) ;
            outfile << 2*i->_id << " " << var1 << " " << var2;
        }
    }
    cnt = 0;
    for (auto i: _piIdList){
        if(!(_gateList[i]->_symbol.empty())){
            outfile << endl;
            outfile << "i" << cnt << " " << _gateList[i]->_symbol;
        }
        ++cnt;
    }    
    for (int i=0; i<_miloa[3]; ++i){
        CirGate* thisG = _gateList[i+_miloa[0]+1];
        if(!(thisG->_symbol.empty())){
            outfile << endl;
            outfile << "o" << i << " " << thisG->_symbol;
        }
    }
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
    unsigned g_miloa[5] = {0};
    GateList gDfs;
    CirGate::setGlobalRef();
    g->dfsTraversal();
    IdList g_pi;
    IdList g_aig;

    g_miloa[0] = gDfs[0]->_id;
    for(unsigned i = 0 ; i < gDfs.size() ; ++i)
    {
        if(gDfs[i]->getTypeStr() == "AIG")
        {
            g_miloa[4]++;
            g_aig.push_back(gDfs[i]->_id);
        }
        else if(gDfs[i]->getTypeStr() == "PI")
        {
            g_miloa[1]++;
            g_pi.push_back(gDfs[i]->_id);
        }

        if(gDfs[i]->_id > g_miloa[0])
        {
            g_miloa[0] = gDfs[i]->_id;
        }
    } 
    g_miloa[3] = g->_fanoutList.size();
    
    outfile << "aag" << " " << g_miloa[0] << " " << g_miloa[1] << " " << g_miloa[2] << " " << g_miloa[3]
        << " " << g_miloa[4] << '\n';
    for(unsigned i = 0 ; i < g_pi.size() ; ++i)
    {
        outfile << g_pi[i] * 2 << '\n';
    }
    for(unsigned i = 0 ; i < g->_fanoutList.size() ; ++i)
    {
        outfile << g->getFanout(i)->_id << '\n';
    }
    for(unsigned i = 0 ; i < g_aig.size() ; ++i)
    {
        outfile << g_aig[i] / 2 << " ";
        if(_gateList[g_aig[i]]->getInInv(0))
        {
            outfile << _gateList[g_aig[i]]->_id * 2 + 1 << " ";
        }
        else
        {
            outfile << _gateList[g_aig[i]]->_id * 2 << " ";
        }

        if(_gateList[g_aig[i]]->getInInv(1))
        {
            outfile << _gateList[g_aig[i]]->_id * 2 + 1 << " ";
        }
        else
        {
            outfile << _gateList[g_aig[i]]->_id * 2 << " ";
        }
    }    
}

CirGate* 
CirMgr::getGate(unsigned gid) const { 
    if( _gateList[gid] == 0 ) return 0;
    if( gid>(unsigned)_miloa[0]+_miloa[3] || _gateList[gid]->getTypeStr()=="UNDEF") 
        return 0;
    return _gateList[gid];
}

/*******************************/
/*   private member function   */
/*******************************/

bool
CirMgr::readHeader(ifstream& ifs)
{
    ifs.getline(buf, 1024);
    vector<string> headerList;
    if(!checkSpace(buf)) return false;
    if(!lexOptions(buf, headerList, 6)){
        if(headerList.size()>6){
            colNo = strstr(buf, headerList[5].c_str())-buf + headerList[5].length();
            return parseError(MISSING_NEWLINE);
        }
        errMsg = "number of variables";
        return parseError(MISSING_NUM);
    }
    if(headerList[0]!="aag"){
        errMsg = headerList[0];
        return parseError(ILLEGAL_IDENTIFIER);
    }
    for (int i = 1; i < 6; ++i) {
        if(!myStr2Int(headerList[i], _miloa[i-1]) || _miloa[i-1]<0){
            string tmp;
            switch (i){
                case 1: tmp = "variables";break;
                case 2: tmp = "inputs";break;
                case 3: tmp = "latches";break;
                case 4: tmp = "ouputs";break;
                case 5: tmp = "AIGs";break;
            };
            errMsg = "number of " + tmp + "(" + headerList[i] + ")";
            return parseError(ILLEGAL_NUM);
        }
    }
    if(_miloa[2]!=0){ 
        errMsg = "latches";
        return parseError(ILLEGAL_NUM);
    }
    if(_miloa[0] < _miloa[1]+_miloa[4]){
        errMsg = "num of variables";
        return parseError(NUM_TOO_SMALL);
    }
    _gateList.resize(_miloa[0]+_miloa[3]+1);
    _piIdList.resize(_miloa[1]);
    for (int i = 0; i < _miloa[0]+_miloa[3]+1; ++i) 
        _gateList[i] = 0;
    ++lineNo; _gateList[0] = const0;
    return true;
}

bool
CirMgr::readIO(ifstream& ifs, unsigned flag) // input(0),output(1)
{
    if(_miloa[2*flag+1]==0) return true;
    for (int i = 0; i < _miloa[2*flag+1]; ++i) {
        IdList varList;
        int idVar = 0;
        string cflag = flag?"PO":"PI" ;
        if(ifs.getline(buf, 1024)){
            // Checking syntax
            if(strcmp(buf, "")==0){
                errMsg = cflag;
                return parseError(MISSING_DEF);
            }
            if(buf[0]==' ')
                return parseError(EXTRA_SPACE);
            if(!myStr2Int(buf, idVar) || idVar<0){
                errMsg = cflag + " literal ID(" + string(buf) + ")"; 
                return parseError(ILLEGAL_NUM);
            }
            // Checking semantics
            errInt = idVar;
            if(idVar<=1 && !flag)
                return parseError(REDEF_CONST); 
            if((idVar/2)>_miloa[0])
                return parseError(MAX_LIT_ID);
            if( !flag &&  _gateList[idVar/2]!=0 ){
                errGate = _gateList[idVar/2];
                return parseError(REDEF_GATE);
            }
            else if(idVar%2 && !flag){
                errMsg = cflag;
                return parseError(CANNOT_INVERTED);
            }
            varList.push_back(idVar);
            if(!flag){
                _gateList[ID(idVar)] = new PiGate(ID(idVar), lineNo+1);
                _piIdList[i] = (ID(idVar));
            }
            else
                _gateList[_miloa[0]+i+1] = new PoGate(_miloa[0]+i+1, lineNo+1, varList);
        }
        else{
            errMsg = cflag;
            return parseError(MISSING_DEF);
        } 
        ++lineNo;
    }
    return true;
}


bool
CirMgr::readAig(ifstream& ifs)
{
    if(_miloa[4]==0) return true;
    for (int i = 0; i < _miloa[4]; ++i) {
        vector<string> options;
        IdList varList;
        int idVar = 0;
        if(ifs.getline(buf, 1024)){
            // Checking syntax
            if(!checkSpace(buf))
                return false;
            else if(!lexOptions(buf,options, 3)){
                size_t size = options.size();
                if(size>3){
                    colNo = options[1].length()+options[2].length()+options[3].length()+2;
                    return parseError(MISSING_NEWLINE);
                } 
                else if(size>=1){
                    if(size==1)
                        colNo = options[1].length();
                    else
                        colNo = options[1].length()+options[2].length()+1;
                    errMsg = "AIG input Literal ID";
                    return parseError(MISSING_NUM);
                }
                errMsg = "AIG";
                return parseError(MISSING_DEF);
            }
            // Checking semantics
            for (int i = 0; i < 3; ++i) {
                int tmp;
                if(!myStr2Int(options[i], tmp) || tmp<0){
                    errInt = tmp;
                    string flag = (i==0)? "AIG": "AIG input";
                    errMsg = flag + " literal ID(" + options[i] +")";
                    return parseError(ILLEGAL_NUM);
                }
                if(tmp/2 > _miloa[0]) 
                    return parseError(MAX_LIT_ID);
                errInt = tmp;
                if(i==0){
                    if(tmp<1)
                        return parseError(REDEF_CONST);
                    if(tmp%2){
                        errMsg = "AIG gate";
                        return parseError(CANNOT_INVERTED);
                    }
                    if( _gateList[ID(tmp)]!=0 ){
                        errGate = _gateList[ID(tmp)];
                        return parseError(REDEF_GATE);
                    }
                    idVar = tmp;
                }
                else
                    varList.push_back(tmp);
                colNo+=options[i].length()+1;
            }
            _gateList[ID(idVar)] = new AigGate(ID(idVar), lineNo+1, varList);
        }
        else{
            errMsg = "AIG";
            return parseError(MISSING_DEF);
        }
        ++lineNo;
    }
    return true;
}

bool 
CirMgr::readSymbol(ifstream& ifs){
    string bufStr;
    while(getline(ifs, bufStr)){
        if(bufStr[0]==' ')
            return parseError(EXTRA_SPACE);
        if(bufStr[0]=='c'){
            strcpy(buf, bufStr.c_str());
            return true;
        }
        if(bufStr[0]=='i' || bufStr[0]=='o'){
            colNo++; string tmp; int idx;
            bool isI = bufStr[0]=='i';
            int maxIdx = isI ? _miloa[1]: _miloa[3];
            if(bufStr.find(' ')==string::npos){
                colNo = bufStr.length()-1;
                return parseError(MISSING_SPACE);
            }
            tmp = bufStr.substr(1, bufStr.find(' ')-1);
            if(!myStr2Int(tmp, idx) || idx<0){
                errMsg = "symbol index(" + tmp + ")";
                return parseError(ILLEGAL_NUM);
            }
            if( idx>=maxIdx ){
                errInt = idx;
                errMsg = (isI?"PI":"PO") + string(" index") ;
                return parseError(NUM_TOO_BIG);
            }
            colNo+=tmp.length(); tmp = bufStr.substr(bufStr.find(' ')+1);
            if(tmp.empty()){
                errMsg = "symbolic name";
                return parseError(MISSING_IDENTIFIER);
            }
            if(myIsPrint(tmp)){
                if(isI && !_gateList[_piIdList[idx]]->setSymbol(tmp)){
                    errMsg = "i"; errInt = idx;
                    return parseError(REDEF_SYMBOLIC_NAME);
                }
                if(!isI && !_gateList[_miloa[0]+idx+1]->setSymbol(tmp)){
                    errMsg = "o"; errInt = idx;
                    return parseError(REDEF_SYMBOLIC_NAME);
                }
                colNo = 0; lineNo++;
            }
            else return false;
        }
        else{
            errMsg = bufStr[0];
            return parseError(ILLEGAL_SYMBOL_TYPE);
        }
    }
    return true;
}
bool 
CirMgr::readComment(ifstream& ifs){
    if(!ifs) return true;
    if(strlen(buf)>1) {return false;}
    return true;
}

void
CirMgr::connect(){
    int cnt = 1;
    for(auto i=_gateList.begin()+1; i!=_gateList.end(); ++i, ++cnt){
        if(*i)
            (*i)->buildConnect();
    }
}

void 
CirMgr::genDFSList(){
    CirGate::setGlobalRef();
    for(int i=0; i<_miloa[3]; ++i){
        if( _gateList[i+_miloa[0]+1]->getTypeStr()=="UNDEF" )
            break;
        _gateList[i+_miloa[0]+1]->dfsTraversal();
    }
}

void
CirMgr::genNotUsedList(){
    for(int i=1; i<(int)_gateList.size(); ++i){
        if(!_gateList[i]) continue;
        if(_gateList[i]->defNotUsed())
            _notUsedIdList.push_back(i);
    }
}

void 
CirMgr::reset(){
    for (int i = 0; i < 5; i++) 
        _miloa[i] = 0;
    for(size_t i=1; i<_gateList.size(); ++i){
        if(!_gateList[i])
            delete _gateList[i];
    }
    resetStaticVar();
};

