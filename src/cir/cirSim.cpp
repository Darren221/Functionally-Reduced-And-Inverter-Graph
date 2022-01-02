/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include <utility>
#include <unordered_map>


using namespace std;
 
// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions
 
/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{

}

void
CirMgr::fileSim(ifstream& patternFile)
{
  string line;
  size_t simNum = 0;
  bool IsValid = true;
  vector<string> in_pattern(64);
  _simCount = 63; //
  _lineNum = 0;
  bool isSimed = false;
  while(patternFile >> line)
  {
    //cerr <<_lineNum;
    in_pattern[_lineNum % 64] = line;
    _lineNum++;

    if(isValid(line) == false)
    {
      //
      IsValid = false;
      break;
    }
    //存滿64bit sim一次, size_t 每次 <<1 size_t剛好有64bit，要sim的時候再bitwise
    //into PI
    //cerr <<_simCount <<endl;
    for(int i = 0 ; i < _miloa[1] ; ++i)
    {
      _gateList[_piIdList[i]]->_simVal += ((size_t)1 << (size_t)_simCount) * (size_t)(line[i] - '0'); //如果乘0仍會讓該位有值??
    }
    if(_simCount == 0) _simCount = 63;
    _simCount--;

    //每次sim就把這個_simVal往下推，只要bitwise做對結果就會對
    if(_simCount % 64 == 0)//滿64才做sim
    {
      //sim
      simAllGate();
      simNum += 64;
      //writesimlog
      if(_simLog != 0 ) { writeSimLog(); } //&& valid ??
      //getFec
      buildFECs(isSimed);
      isSimed = true;
    }

  }
  //sim the left 剩下的不滿64的
  if(_lineNum % 64 != 0 && IsValid == true)
  {
    //cout <<_lineNum;
    //cerr<<_simCount<<endl;
    while(_simCount >= 0)
    {
      //cerr<<_simCount<<endl;
      for(int i = 0 ; i < _miloa[1] ; ++i)
      {
        _gateList[_piIdList[i]]->_simVal += ((size_t)1 << (size_t)_simCount) * ((size_t)0);
      }
      _simCount--;
    }
    simAllGate();
    simNum += _lineNum % 64;
    if(_simLog != 0 && IsValid == true) { writeSimLog(); }
    buildFECs(isSimed);
    isSimed = true;
  }
  if(IsValid == true)
  {
    fecs_sort();
    //fecMap_sort();
  }
  
  cout << simNum << " patterns simulated." << endl;
  patternFile.close();
}

void //印出來只要印到_simCount 不用印補零
CirMgr::writeSimLog()
{
  //string po_line = "";
  size_t extractor = ( (size_t)1 << 63 );
  if(_lineNum % 64 == 0)
  {
    size_t i = 63;
    for(int k = 63 ; k >= 0 ; --k)//each line
    {
      //PI
      for(unsigned j = 0 ; j < _miloa[1] ; ++j)
      {
        *_simLog << ( ( (_gateList[_piIdList[j]]->_simVal) & extractor) >> i );
      }
      *_simLog << " ";
      //PO
      for(unsigned j = _miloa[0]+1 ; j <= _miloa[0] + _miloa[3] ; ++j)
      {
        *_simLog <<  ( ( (_gateList[j]->_simVal) & extractor) >> i );
      }
      *_simLog << '\n';

      extractor = extractor >> 1;
      i--;
    }
  }
  else if(_lineNum % 64 != 0)
  {
    //size_t i = _lineNum % 64;
    size_t i = 63;
    for(int k = _lineNum % 64 ; k > 0 ; --k) //why 一次少兩個??
    {
      //PI
      for(unsigned j = 0 ; j < _miloa[1] ; ++j)
      {
        *_simLog << ( ( (_gateList[_piIdList[j]]->_simVal) & extractor) >> i );
      }
      *_simLog << " ";
      //PO
      for(unsigned j = _miloa[0]+1 ; j <= _miloa[0] + _miloa[3] ; ++j)
      {
        *_simLog <<  ( ( (_gateList[j]->_simVal) & extractor) >> i );
      }
      //cerr<<k;
      *_simLog << '\n';
      extractor = extractor >> 1;
      i--;
    }
  }
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
bool
CirMgr::isValid(string &line)
{
  if(line.size() != _miloa[1])
  {
    cerr << endl << "Error: Pattern(" << line << ") length(" << line.size() 
        << ") does not match the number of inputs("<< _miloa[1] << ") in a circuit!!" << endl;
    return false;
  }
  for(unsigned i = 0 ; i < line.size() ; ++i)
  {
    if(line[i] != '0' && line[i] != '1')
    {
      cerr << endl << "Error: Pattern(" << line << ") contains a non-0/1 character(" << line[i] << ")." << endl;
      return false;
    }
  }
  return true;
}

void
CirMgr::simAllGate()
{
  for(unsigned i = 0, n = _dfsList.size() ; i < n ; ++i)
  {
    if(_dfsList[i]->getTypeStr() == "PO" || _dfsList[i]->getTypeStr() == "AIG")
    {
      _dfsList[i]->simulate();
    }
  }
}

void
CirMgr::buildFECs(bool &isSimed)
{
  nFECgroup = 0;
  if(isSimed == false)
  {
    //IdList init; 
    unordered_map<size_t, IdList> tmp_FECgroup;
    unordered_map<size_t, IdList>::iterator tmp_FECgroupIt;
    for(unsigned i = 0, n = _dfsList.size() ; i < n ; ++i)
    {
      if(_dfsList[i]->getTypeStr() == "AIG" || _dfsList[i]->getTypeStr() == "CONST")
      {
        //init.push_back(_dfsList[i]->_id);
        if(tmp_FECgroup.find( hashFECkey( _dfsList[i]->_simVal ) ) == tmp_FECgroup.end()) 
        {
          IdList fecs;
          fecs.push_back(_dfsList[i]->_id);
          std::pair <size_t, IdList> fec_group ( hashFECkey(_dfsList[i]->_simVal), fecs );
          tmp_FECgroup.insert(fec_group);
        }
        else
        {
          tmp_FECgroup[ hashFECkey( _dfsList[i]->_simVal ) ].push_back(_dfsList[i]->_id);
        } 
      }
    }
    for(tmp_FECgroupIt = tmp_FECgroup.begin() ; tmp_FECgroupIt != tmp_FECgroup.end() ; ++tmp_FECgroupIt)
    {
      _realFECgroup.push_back(tmp_FECgroupIt->second);
    }
  }
  else//simed, divide
  { 
    vector<unordered_map<size_t, IdList>> new_realFEC;

    unordered_map<size_t, IdList>::iterator tmp_FECgroupIt;
    unordered_map<size_t, IdList> tmp_FECgroup;
    for(size_t i = 0 ; i < _realFECgroup.size() ; ++i)
    {
      tmp_FECgroup.clear();
      
      //size_t first_hashKey = hashFECkey( _gateList[ _realFECgroup[i][0] ]->_simVal );
      for(size_t j = 0 ; j < _realFECgroup[i].size() ; ++j)
      {
        if(_realFECgroup[i].size() <= 1) continue;
        //if( hashFECkey( _gateList[ _realFECgroup[i][j] ]->_simVal) != first_hashKey )//not FEC, should be divided
        //{
          if(tmp_FECgroup.find( hashFECkey( _gateList[ _realFECgroup[i][j] ]->_simVal ) ) == tmp_FECgroup.end())
          {
            IdList fecs;
            fecs.push_back(_realFECgroup[i][j]);
            std::pair <size_t, IdList> fec_group ( hashFECkey(_gateList[ _realFECgroup[i][j] ]->_simVal), fecs );
            tmp_FECgroup.insert(fec_group);
          }
          else
          {
            tmp_FECgroup[ hashFECkey( _gateList[ _realFECgroup[i][j] ]->_simVal ) ].push_back(_realFECgroup[i][j]);
          }
          //divide
          //_realFECgroup[i].erase(_realFECgroup[i].begin() + j);
        //}
        //else nothing??
      }
      
      new_realFEC.push_back(tmp_FECgroup);
    }
    _realFECgroup.clear();
    for(unsigned i = 0, n = new_realFEC.size() ; i < n ; ++i)
    {
      for(tmp_FECgroupIt = new_realFEC[i].begin() ; tmp_FECgroupIt != new_realFEC[i].end() ; ++tmp_FECgroupIt)
      {
        _realFECgroup.push_back(tmp_FECgroupIt->second);
      } 
    }
      /*
      _realFECgroup.erase(_realFECgroup.begin() + i);
      _realFECgroup.erase(_realFECgroup.begin() + i);
      for(tmp_FECgroupIt = tmp_FECgroup.begin() ; tmp_FECgroupIt != tmp_FECgroup.end() ; ++tmp_FECgroupIt)
      {
        _realFECgroup.push_back(tmp_FECgroupIt->second);
      }
      */
    
    /*
    for(tmp_FECgroupIt = tmp_FECgroup.begin() ; tmp_FECgroupIt != tmp_FECgroup.end() ; ++tmp_FECgroupIt)
    {
      _realFECgroup.push_back(tmp_FECgroupIt->second);
    }
    */
    /*
    for(_fecMapIt = _fecMap.begin() ; _fecMapIt != _fecMap.end() ; ++_fecMapIt)
    {
      for(size_t i = 0, n = _fecMapIt->second.size() ; i < n ; ++i) //iterate Idlist
      {

        unordered_map <size_t, IdList> tmp_Idmap; //永遠只會用到map size == 1?? 可改成用pair 如此也不用find 直接==
        
        if( tmp_Idmap.find( hashFECkey(_gateList[ (*_fecMapIt).second[i] ]->_simVal ) ) == tmp_Idmap.end() ) //not found, not FEC, should be divided
        {
          if(tmp_Idmap.size() == 0)//first time, still insert it
          {
            tmp_Idmap.emplace( hashFECkey(_gateList[ (*_fecMapIt).second[i] ]->_simVal ), (*_fecMapIt).second[i] );
          }
          else //divide
          {
            //erase original
            _fecMapIt->second.erase(_fecMapIt->second.begin() + i);
            //create new fec group
            IdList fecs;
            fecs.push_back( (*_fecMapIt).second[i] );
            std::pair <size_t, IdList> fec_group ( hashFECkey( _gateList[ (*_fecMapIt).second[i] ]->_simVal ), fecs );
            _fecMap.insert(fec_group); //邊找map大小卻邊改變？有辦法仍找到新增的東西？
            //_fecMap.emplace(hashFECkey(_gateList[ (*_fecMapIt).second[i] ]->_simVal ), (*_fecMapIt).second[i] );
          }
        }
        else //found
        {
          //nothing??
        }
      }
    }
    */
  }
  
  for(size_t i = 0 ; i < _realFECgroup.size() ; ++i)
  {
    if(_realFECgroup[i].size() > 1){
      nFECgroup++;
      //_realFECgroup.erase(_realFECgroup.begin() + i);
    } 
  }
  
  //cout << "Total #FEC Group = " << _fecMap.size() << endl;
  cout << "Total #FEC Group = " << nFECgroup << endl;
}

void
CirMgr::fecs_sort()
{
  /*
  for(_fecMapIt = _fecMap.begin() ; _fecMapIt != _fecMap.end() ; ++_fecMapIt)
  {
    sort( _fecMapIt->second.begin(), _fecMapIt->second.end() );
    if(_fecMapIt->second.size() > 1)
    {
      _Keys_Idlistptr.push_back( &(_fecMapIt->second) ); //#*_Keys_Idlistptr[idx] == _fecMap[idx]
    }
  }
  */
  for(size_t i = 0 ; i < _realFECgroup.size() ; ++i)
  {
    //if(_realFECgroup[i].size() <= 1) _realFECgroup.erase(_realFECgroup.begin() + i);
    sort( _realFECgroup[i].begin(), _realFECgroup[i].end() );
  }
  for(size_t i = 0 ; i < _realFECgroup.size() ; ++i)
  {
    sort( _realFECgroup.begin(), _realFECgroup.end(), compare_comp);
  }
}
/*
void
CirMgr::fecMap_sort()
{
  
  sort(_Keys_Idlistptr.begin(), _Keys_Idlistptr.end(), compare_comp);
  //_hashFecKeys
}
*/

bool
CirMgr::compare_comp(IdList &l, IdList &r)
{
  return ( l[0] < r[0] );
}

size_t
CirMgr::hashFECkey(size_t &simVal)
{
  if(~simVal > simVal) return simVal;
  else return ~simVal;
}