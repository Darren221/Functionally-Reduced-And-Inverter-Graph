/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"
#include <unordered_map>
#include <utility>
 
using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash() //miloa a包含float不包含undef??
{
  unordered_map<size_t, CirGate*> hash_fanin; //(_miloa[0]*2); //setsize??
  unordered_map<size_t, CirGate*>::iterator it;
  //typedef pair<CirGate*, CirGate*> fanin_pair;
  
  for(unsigned i = 0, n = _dfsList.size() ; i < n ; ++i)
  {
    if(_dfsList[i]->getTypeStr() != "AIG") continue;

    pair<size_t, CirGate*> in_pair (genHashKey(_dfsList[i]), _dfsList[i]);

    if(hash_fanin.find(genHashKey(_dfsList[i])) == hash_fanin.end()) //not found, can be inserted
    {
      hash_fanin.insert(in_pair);
      //do nothing
    }
    else //found 
    {
      it = hash_fanin.find(genHashKey(_dfsList[i])); //the one already in hash table
      //merge "it(iterator)" with _dfsList[i]
      CirGate* g = _dfsList[i];
      //fanin disconnected from g
      for(unsigned j = 0, n = g->getFanin(0)->_fanoutList.size() ; j < n ; ++j)
      {
        if(g->getFanin(0)->getFanout(j) == g) g->getFanin(0)->_fanoutList.erase(g->getFanin(0)->_fanoutList.begin() + j);
      }
      for(unsigned j = 0, n = g->getFanin(1)->_fanoutList.size() ; j < n ; ++j)
      {
        if(g->getFanin(1)->getFanout(j) == g) g->getFanin(1)->_fanoutList.erase(g->getFanin(1)->_fanoutList.begin() + j);
      }
      //fanout disconnected from g
      for(unsigned j = 0, n = g->_fanoutList.size() ; j < n ; ++j)
      { 
        if(g->getFanout(j)->getFanin(0) == g)
        {
          if(g->getFanout(j)->getInInv(0))
            g->getFanout(j)->setFanin(it->second, 0, true);
          else
            g->getFanout(j)->setFanin(it->second, 0, false);
        }
        else if(g->getFanout(j)->getFanin(1) == g)
        {
          if(g->getFanout(j)->getInInv(1))
            {
              // CirGate* tem = it->second;
              // CirGate::setInv(tem,1);
              // tem = (CirGate*)( size_t(tem) | (0x1) );
              // cerr<<&tem<<' '<<(CirGate::isInv(tem))<<"hi"<<endl;
              // g->getFanout(j)->_faninList.pop_back();
              // g->getFanout(j)->_faninList.push_back(tem);
              
              g->getFanout(j)->setFanin(it->second, 1, true);
            }
          else
            g->getFanout(j)->setFanin(it->second, 1, false);
        } 
         
      }
      cout << "Strashing: " << it->second->_id << " merging ";
      cout << g->_id << "..."<< endl;
      //disconnect g, and remove g
      g->_faninList[0] = 0;
      g->_faninList[1] = 0;
      g->_fanoutList.clear();
      _gateList[g->_id] = 0;
      /*
      for(unsigned aig_idx = 0, n = _aig.size() ; aig_idx < n ; ++aig_idx)
      {
        if(_aig[aig_idx] == g->_id) _aig.erase(_aig.begin() + aig_idx);
      }
      */
      
      _miloa[4]--;
      delete g;
    }
  }
  //update _aig
  //_miloa[4] = _aig.size();
  //reconstruct dfs
  _dfsList.clear();
  genDFSList();
  vector<bool> dfs_gatelist(_miloa[0] + _miloa[3] + 1, false);
      for(unsigned i = 0 ; i < _dfsList.size() ; ++i)
      {
        dfs_gatelist[_dfsList[i]->_id] = true;
      }
      for(unsigned i = 0 ; i < _floatIdList.size() ; ++i)
      {
        if(dfs_gatelist[ _floatIdList[i] ] == false) _floatIdList.erase(_floatIdList.begin() + i);
      }
}

void
CirMgr::fraig()
{
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
size_t CirMgr::genHashKey(CirGate *&g)
{
  size_t a = 0;
  size_t b = 0;
  bool sel = false;

  if (g->getFanin(1) < g->getFanin(0)) sel = true; // swap fanin 1, fanin 2
  a = g->getFanin(sel)->_id;
  b = g->getFanin(!sel)->_id;

  return (a << 32) + (b << 2) + (g->isInv(g->_faninList[!sel]) << 1) + g->isInv(g->_faninList[sel]); /*_inv[!sel]*/
}
