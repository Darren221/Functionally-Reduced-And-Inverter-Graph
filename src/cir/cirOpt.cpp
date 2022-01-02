/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
 
/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()//_floatIdList _notUsedIdList要更新?? 只跟print -float有關！ //不可刪const因為是static??
{
  IdList rm;
  
  vector<bool> dfs_gatelist(_miloa[0] + _miloa[3] + 1, false);
  for(unsigned i = 0 ; i < _dfsList.size() ; ++i)
  {
    dfs_gatelist[_dfsList[i]->_id] = true;
  }
  for(unsigned i = 0 ; i < dfs_gatelist.size() ; ++i)
  {
    if(_gateList[i] == 0) continue;
    //if(_gateList[i]->getTypeStr() != "PI" && dfs_gatelist[i] == false) 
    if(dfs_gatelist[i] == false) rm.push_back(i);
  }

  for(unsigned i = 0 ; i < rm.size() ; ++i)
  {
    //remove _gateList[rm[i]]
    if(_gateList[rm[i]]->getTypeStr() != "PI")
    {
      if(_gateList[rm[i]]->getTypeStr() == "AIG")
      {
        if(_gateList[rm[i]]->getFanin(0) != 0)
        {
          for(unsigned j = 0, n = _gateList[rm[i]]->getFanin(0)->_fanoutList.size() ; j < n ; ++j)
          {
            
            if(_gateList[rm[i]]->getFanin(0)->getFanout(j) == _gateList[rm[i]])
              _gateList[rm[i]]->getFanin(0)->_fanoutList.erase(_gateList[rm[i]]->getFanin(0)->_fanoutList.begin() + j);
          }
          _gateList[rm[i]]->_faninList[0] = 0; //remove fanin
        }

        if(_gateList[rm[i]]->getFanin(1) != 0)
        {
          //remove fanin's fanout(me)
          for(unsigned j = 0, n = _gateList[rm[i]]->getFanin(1)->_fanoutList.size() ; j < n ; ++j)
          {
            if(_gateList[rm[i]]->getFanin(1)->getFanout(j) == _gateList[rm[i]])
              _gateList[rm[i]]->getFanin(1)->_fanoutList.erase(_gateList[rm[i]]->getFanin(1)->_fanoutList.begin() + j);
          }
          _gateList[rm[i]]->_faninList[1] = 0; //remove fanin
        }
        /*
        for(unsigned i = 0, n = _aig.size() ; i < n ; ++i)
        {
          if(_aig[i] == _gateList[rm[i]]->_id)
          {
            _aig.erase(_aig.begin() + i);
            break;
          } 
        }
        */
        cout << "Sweeping: AIG(" << rm[i] << ") removed..." << endl;
        delete _gateList[rm[i]]; //??
        _gateList[rm[i]] = 0; //remove from gateList
        _miloa[4]--;
      }
      else if(_gateList[rm[i]]->getTypeStr() == "UNDEF")
      {
        if(_gateList[rm[i]]->_faninList.size() > 0)
        {
          if(_gateList[rm[i]]->getFanin(0) != 0)
          {
            for(unsigned j = 0, n = _gateList[rm[i]]->getFanin(0)->_fanoutList.size() ; j < n ; ++j)
            {
              
              if(_gateList[rm[i]]->getFanin(0)->getFanout(j) == _gateList[rm[i]])
                _gateList[rm[i]]->getFanin(0)->_fanoutList.erase(_gateList[rm[i]]->getFanin(0)->_fanoutList.begin() + j);
            }
            _gateList[rm[i]]->_faninList[0] = 0; //remove fanin
          }
        }
        if(_gateList[rm[i]]->_faninList.size() > 1)
        {
          if(_gateList[rm[i]]->getFanin(1) != 0)
          {
            //remove fanin's fanout(me)
            for(unsigned j = 0, n = _gateList[rm[i]]->getFanin(1)->_fanoutList.size() ; j < n ; ++j)
            {
              if(_gateList[rm[i]]->getFanin(1)->getFanout(j) == _gateList[rm[i]])
                _gateList[rm[i]]->getFanin(1)->_fanoutList.erase(_gateList[rm[i]]->getFanin(1)->_fanoutList.begin() + j);
            }
            _gateList[rm[i]]->_faninList[1] = 0; //remove fanin
          }
        }

        cout << "Sweeping: UNDEF(" << rm[i] << ") removed..." << endl;
        delete _gateList[rm[i]]; //??
        _gateList[rm[i]] = 0; //remove from gateList
      }
      /*
      else if(_gateList[rm[i]]->getTypeStr() == "CONST")
      {
        for(unsigned i = 0, n = _gateList[rm[i]]->_fanoutList.size() ; i < n ; ++i)
        {
          if(_gateList[rm[i]]->getFanout(i)->getFanin(0) == _gateList[rm[i]]) _gateList[rm[i]]->getFanout(i)->setFanin(0,0,false);
          else  _gateList[rm[i]]->getFanout(i)->setFanin(0,1,false);
        }
        cout << "Sweeping: CONST(" << rm[i] << ") removed..." << endl; //??
        _gateList[rm[i]]->_fanoutList.clear();
        delete _gateList[rm[i]];
        _gateList[rm[i]] = 0;
      }
      */
    }
  }
  //_miloa[4] = _aig.size();
  //_miloa[3] = _poList.size(); //would output be swept out??
  _notUsedIdList.clear();
  genNotUsedList();
  
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...//??
void
CirMgr::optimize()
{
  for(unsigned i = 0, n = _dfsList.size() ; i < n ; ++i)
  {
    CirGate* g = _dfsList[i];
    
    if(g->getTypeStr() == "PI" || g->getTypeStr() == "CONST" || g->getTypeStr() == "PO"){
      continue;
    } 
    if(g->_faninList.size() == 0) continue;
    
    if(g->getFanin(0)->getTypeStr() == "CONST")
    {
      //cerr<<"const-0";
      if(g->isInv(g->_faninList[0]) == true) //const1 //caseA //g->isInv(g->_faninList[0]) //g->isInv(g->_faninList[1])
      { 
        replace(g->getFanin(0), g->getFanin(1), g);
        cout << "Simplifying: " << g->getFanin(1)->_id << " merging ";
        if(g->getInInv(1)) cout << "!";
        cout << g->_id << "..."<< endl;
        //!const0's fanout?? already considered
      }
      else //const0 //caseB
      { 
        replace(g->getFanin(1), g->getFanin(0), g);
        cout << "Simplifying: " << g->getFanin(0)->_id << " merging ";
        if(g->getInInv(0)) cout << "!";
        cout << g->_id << "..."<< endl;
      }
      removeGate(g);
      //reconstruct dfsList->final
    }
    else if(g->getFanin(1)->getTypeStr() == "CONST")
    {
      //cerr<<"const-1";
      if(g->getInInv(1) == true) //const1 //caseA
      {
        replace(g->getFanin(1), g->getFanin(0), g);
        cout << "Simplifying: " << g->getFanin(0)->_id << " merging ";
        if(g->getInInv(0)) cout << "!";
        cout << g->_id << "..."<< endl;
      }
      else //const0 //caseB
      {
        replace(g->getFanin(0), g->getFanin(1), g);
        cout << "Simplifying: " << g->getFanin(1)->_id << " merging ";
        if(g->getInInv(1)) cout << "!";
        cout << g->_id << "..."<< endl;
      }
      removeGate(g);
      //reconstruct dfsList->final
    }
    else if(g->getFanin(0) == g->getFanin(1)) 
    {
      if(g->getInInv(0) == g->getInInv(1)) //caseC
      {
        //cerr<<"caseC";
        //replace(g->getFanin(0), g->getFanin(1), g);
        for(unsigned j = 0, n = g->_fanoutList.size() ; j < n ; ++j)
        {
          if(g->getFanout(j)->getFanin(0) == g)
          {
            g->getFanout(j)->setFanin(g->getFanin(0), 0, false);
          }
          else if(g->getFanout(j)->getFanin(1) == g)
          {
            g->getFanout(j)->setFanin(g->getFanin(0), 1, false);
          }
          g->getFanin(0)->_fanoutList.pop_back();
          g->getFanin(0)->_fanoutList.pop_back();//clear all fanout, reset
          //inv or not
          if(g->getOutInv(j))
          {
            g->getFanin(0)->setFanout(g->getFanout(j), true);
          }
          else
          {
            g->getFanin(0)->setFanout(g->getFanout(j), false);
          }
        }
        //g->getFanin(0)->_faninList
        //g->getFanin(0)->getFanin(0)
        cout << "Simplifying: " << g->getFanin(0)->_id << " merging ";
        if(g->getInInv(0)) cout << "!";
        cout << g->_id << "..." << endl;
        removeGate(g);
      }
      else if(g->getInInv(0) != g->getInInv(1)) //caseD
      {
        cerr<<"caseD";
        //replace both fanins of g with const0
        CirGate* newConst0 = new PiGate(0, 0);
        for(unsigned j = 0, n = g->getFanin(0)->_fanoutList.size() ; j < n ; ++j) //which outlist idx of _faninList[0](==_faninList[1]) is g
        {
          if(g->getFanin(0)->getFanout(j) == g) g->getFanin(0)->_fanoutList.erase(g->getFanin(0)->_fanoutList.begin() + j);
        }
        for(unsigned j = 0, n = g->_fanoutList.size() ; j < n ; ++j)
        {
          newConst0->_fanoutList.push_back(g->_fanoutList[j]); //a is connected to fanouts of the gate to be removed
          //fanouts of the gate to be removed are connected to a
          if(g->getFanout(j)->getFanin(0) == g)
          {
            g->getFanout(j)->setFanin(newConst0, 0, false);
            //i_out->_faninList[0] = newConst0;
          } 
          else if(g->getFanout(j)->getFanin(1) == g)
          {
            g->getFanout(j)->setFanin(newConst0, 1, false);
            //_faninList[1] = newConst0;
          } 
        }
          //auto &i_out : g->_fanoutList)  
        //{ 
        //}
        removeGate(g);//update miloa[4] included
        cout << "Simplifying: " << 0 << " merging ";
        //if(g->getInInv(1)) cout << "!";
        cout << g->_id << "..." << endl;
      }
    }
  }
  //final steps
  //_output??
  //_miloa[4] = _aig.size(); 
  //dfsTraversal(_output);
  _dfsList.clear();
  genDFSList();
  _notUsedIdList.clear();
  genNotUsedList();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
void CirMgr::replace(CirGate* const &remove, CirGate* const &remain, CirGate* &g)
{
  for(unsigned j = 0, n = remain->_fanoutList.size() ; j < n ; ++j) //which outlist idx of a is g
  {
    if(remain->getFanout(j) == g){
      remain->_fanoutList.erase(remain->_fanoutList.begin() + j);
    } 
  }
  for(unsigned j = 0, n = remove->_fanoutList.size() ; j < n ; ++j) //which outlist idx of con0 is g
  {
    if(remove->getFanout(j) == g) remove->_fanoutList.erase(remove->_fanoutList.begin() + j);
  }
  for(unsigned j = 0, n = g->_fanoutList.size() ; j < n ; ++j)
  {
    remain->setFanout(g->getGate(g->_fanoutList[j]), g->isInv(g->_fanoutList[j]));
    //_fanoutList.push_back(g->_fanoutList[j]); //a is connected to fanouts of the gate to be removed
    //fanouts of the gate to be removed are connected to a
    if(g->getFanout(j)->getFanin(0) == g)
    { 
      g->getFanout(j)->setFanin(remain, 0, false);
      //g->_fanoutList[j]->_faninList[0] = remain;
      if(g->getFanin(0) == remain) g->getFanout(j)->setInv(g->getFanout(j)->getFanin(0), g->isInv(g->_faninList[0]));//g->_fanoutList[j]->_inv[0] = g->isInv(g->_faninList[0]);
      else g->getFanout(j)->setInv(g->getFanout(j)->getFanin(0), g->isInv(g->_faninList[1]));// = g->isInv(g->_faninList[1]);
    } 
    else if(g->getFanout(j)->getFanin(1) == g)
    {
      g->getFanout(j)->setFanin(remain, 1, false);
      //_faninList[1] = remain;
      if(g->getFanin(0) == remain) g->getFanout(j)->setInv(g->getFanout(j)->getFanin(1), g->isInv(g->_faninList[0]));//g->_fanoutList[j]->_inv[1] = g->isInv(g->_faninList[0]);
      else g->getFanout(j)->setInv(g->getFanout(j)->getFanin(1), g->isInv(g->_faninList[1]));// = g->isInv(g->_faninList[1]);
    } 
  }
    //auto &i_out : g->_fanoutList)  g->_fanoutList[j]
  //{   
  //}
}
void CirMgr::removeGate(CirGate* &g)
{
  //remove gate
  g->_faninList[0] = 0;
  g->_faninList[1] = 0;
  g->_fanoutList.clear();
  _gateList[g->_id] = 0;
  //update _aig
  _miloa[4]--; //update miloa
  /*
  for(unsigned aig_idx = 0, n = _aig.size() ; aig_idx < n ; ++aig_idx)
  {
    if(_aig[aig_idx] == g->_id) _aig.erase(_aig.begin() + aig_idx);
  }
  */
  delete g;
}