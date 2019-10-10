//
// File: wl.h
// 
//  Description: An interface that lists down all the functions in wl.cpp
//  Student Name: Yiyang Lin
//  UW Campus ID: 9080288724
//  Email: ylin363@wisc.edu

#include <map>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <locale>

using namespace std;

void User_Input();
bool Check_Command(vector<string> inputTokens);
void Read_File(const string input_filename);
void Command_LOCATE(string word, int number);
void Command_NEW();
bool Command_END();
void Command_LOAD(string filename);
string convertCharacter(string s);
bool allSpace(string input);