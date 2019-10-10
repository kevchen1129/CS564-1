//
// File: wl.h
//
//  Description: A word locator program (wl.cpp) that allow a user to
//               check if a specified (re)occurrence of a specified
//               query word appears in the input text file.
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

#include "wl.h"

using namespace std;

/**
 * Stores every word loaded from word file to wordList
 */
vector<string> wordList;

/**
 * Seperate the line of input string from user into tokens
 * and push them into a string vector.
 */
void User_Input()
{
   vector<string> inputTokens;
   string userInput;

   while (true)
   {
      cout << ">";

      inputTokens.clear();

      getline(cin, userInput);

      // user only hit enter or user input only space
      if ((int)userInput.length() == 0 || allSpace(userInput))
      {
         continue;
      }

      stringstream check(userInput);

      string tok;

      while (getline(check, tok, ' '))
      {
         if ((int)tok.size() == 0)
         {
            continue;
         }
         inputTokens.push_back(tok);
      }

      if ((int)inputTokens.size() == 0)
      {
         continue;
      }
      else if (Check_Command(inputTokens))
      {
         break;
      }
   }
}

bool allSpace(string input)
{
   for (int i = 0; i < (int)input.size(); i++)
   {
      if (input[i] != ' ')
      {
         return false;
      }
   }
   return true;
}

/**
 * Read file for input, if the file does not exist, print "error"
 * if the word contains character other that number and letter, 
 * split the word into different words and push into seperate 
 * string vector.
 */
void Read_File(const string input_filename)
{
   ifstream file;
   file.open(input_filename);

   // if the file did not open properly
   if (file.fail())
   {
      cout << "ERROR: Invalid command" << endl;
   }

   // tokenize every word in file into single input
   string word;
   while (file >> word)
   {
      // change invalid characters in white space
      word = convertCharacter(word);
      stringstream check(word);
      string intermediate;

      //break  words with white space and store to wordList
      while (check >> intermediate)
      {
         wordList.push_back(intermediate);
      }
   }
   // for (int i = 0; i < (int)wordList.size(); i++)
   // {
   //    cout << wordList[i] << endl;
   // }
   file.close();
}

/**
 * A string parameter that was passed and convert each character
 * in a word. Finally, return lowercase word of that string.
 */
string convertCharacter(string s)
{
   for (int i = 0; i < (int)s.size(); i++)
   {
      if ((s[i] >= 'A' && s[i] <= 'Z') ||
          (s[i] >= 'a' && s[i] <= 'z') ||
          (s[i] == '\'') || (s[i] >= '0' && s[i] <= '9'))
      {
         if (s[i] >= 'A' && s[i] <= 'Z')
         {
            s[i] = (char)(s[i] + 32);
         }
         else
         {
            s[i] = s[i];
         }
      }
      else
      {
         s[i] = ' ';
      }
   }
   return s;
}

/**
 * A user input token string vector parameter passes to this function
 * and meet the corresponding functions, load, locate, new, end.
 * If the input does not meet the commands, prints out "Error".
 * return true, if end program
 */
bool Check_Command(vector<string> inputTokens)
{
   if ((int)inputTokens.size() == 0)
   {
      return false;
   }
   else
   {
      enum INPUT_OPTION
      {
         INVALID,
         LOAD,
         LOCATE,
         NEW,
         END
      };

      // mapping the user inputs string to correspending enum commands
      map<string, INPUT_OPTION> string_to_input;
      string_to_input["load"] = LOAD;
      string_to_input["locate"] = LOCATE;
      string_to_input["new"] = NEW;
      string_to_input["end"] = END;

      string inputCommand;

      inputCommand = inputTokens[0];

      for (int i = 0; i < (int)inputCommand.size(); i++)
      {
         if ((inputCommand[i] >= 'A' && inputCommand[i] <= 'Z') ||
             (inputCommand[i] >= 'a' && inputCommand[i] <= 'z'))
         {
            if (inputCommand[i] >= 'A' && inputCommand[i] <= 'Z')
            {
               inputCommand[i] = (char)(inputCommand[i] + 32);
            }
         }
      }

      INPUT_OPTION command = string_to_input.find(inputCommand)->second;

      switch (command)
      {
      case LOAD:
      {
         if ((int)inputTokens.size() != 2)
         {
            cout << "ERROR: Invalid command" << endl;
            break;
         }
         else
            Command_LOAD(inputTokens[1]);
      }
      break;

      case LOCATE:
      {
         if ((int)inputTokens.size() != 3)
         {
            cout << "ERROR: Invalid command" << endl;
            break;
         }
         else
         {
            string numStr = inputTokens[2];

            stringstream sstream(numStr);

            int n = 0;
            sstream >> n;

            if (n <= 0)
            {
               command = INVALID;
               cout << "ERROR: Invalid command" << endl;
            }
            else
               Command_LOCATE(inputTokens[1], n);
         }
      }
      break;

      case NEW:
      {
         if ((int)inputTokens.size() != 1)
         {
            cout << "ERROR: Invalid command" << endl;
            break;
         }
         else
         {
            Command_NEW();
         }
      }
      break;

      case END:
      {
         if ((int)inputTokens.size() != 1)
         {
            cout << "ERROR: Invalid command" << endl;
         }
         else
         {
            return Command_END();
         }
      }
      break;

      case INVALID: // fall through to default
      default:

         cout << "ERROR: Invalid command" << endl;
         return false;
      }
      return false;
   }
}

/**
 * Find the location of a word with number of that word counting from
 * the beginning of the first, and print the number out
 * If the word does not exist, prints "No matching entry".
 */
void Command_LOCATE(string word, int number)
{
   int count = number;

   string searhWord = word;
   transform(searhWord.begin(), searhWord.end(), searhWord.begin(), ::tolower);

   vector<string>::iterator itr = find(wordList.begin(), wordList.end(), searhWord);

   //counting distances from the first encount of the location
   // if the number user assigned is greater than 1, add on the
   // distance and countinue to count
   int dis = 0;
   if (itr != wordList.cend())
   {
      dis += distance(wordList.begin(), itr) + 1;
      while (count != 1)
      {
         vector<string>::iterator itr1 = itr;
         count--;
         itr = find(wordList.begin() + dis, wordList.end(), searhWord);
         dis += distance(itr1, itr);
      }
      if (dis > (int)wordList.size())
      {
         cout << "No matching entry\n";
      }
      else
         // cout << "Element present at index " << dis << "\n";
         cout << dis << "\n";
   }
   else
   {
      cout << "No matching entry\n";
   }
}

/**
 * Remove all the words that were stored when loaded
 */
void Command_NEW()
{
   wordList.clear();
}

/**
 * Return true to end the entire program
 */
bool Command_END()
{
   return true;
}

/**
 * Pass the name of the file to Read_File function
 */
void Command_LOAD(string filename)
{
   Read_File(filename);
}

/**
 * main functon calls User_Input function
 */
int main()
{
   try
   {
      User_Input();
   }
   catch (const exception &e)
   {
   }

   return 0;
}