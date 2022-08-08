#include <bits/stdc++.h>
#include <map>

using namespace std;

string ltrim(const string &);
string rtrim(const string &);

enum class bracket_t {
    L_SQUARE_BR = '[',
    L_OPENNING_BR = '(',
    L_CURLY_BR = '{',
    R_SQUARE_BR = ']',
    R_OPENNING_BR = ')',
    R_CURLY_BR = '}'
};

void printvec(vector<bracket_t> brs) {
	for(const auto& ch : brs) {
		const char chx = static_cast<char>(ch);
		cout << chx << '\n';
	}
}

/*
 * Complete the 'isBalanced' function below.
 *
 * The function is expected to return a STRING.
 * The function accepts STRING s as parameter.
 */

string isBalanced(string s, bool dlog) {
    vector<bracket_t> brs;
    brs.reserve(s.length()*2);

    map<char, char> brmap {pair<char,char>('(',')'), pair<char,char>('[', ']'), pair<char,char>('{','}')};

    for(auto ch : s) {
        switch(static_cast<bracket_t>(ch)) {
            case bracket_t::L_SQUARE_BR:
                brs.push_back(static_cast<bracket_t>(ch));
                if(dlog)
                	cout << "[ pushed\n";
                break;
            case bracket_t::L_OPENNING_BR:
                brs.push_back(static_cast<bracket_t>(ch));
                if(dlog)
                	cout << "( pushed\n";
                break;
            case bracket_t::L_CURLY_BR:
                brs.push_back(static_cast<bracket_t>(ch));
                if(dlog)
                	cout << "{ pushed\n";
                break;
            case bracket_t::R_SQUARE_BR:
                if(bracket_t::L_SQUARE_BR == *(brs.end()-1))
                	brs.erase(brs.end()-1);
                if(dlog) {
                	cout << "[ removed\n";
                	printvec(brs);
                }
                break;
            case bracket_t::R_OPENNING_BR:
                if(bracket_t::L_OPENNING_BR == *(brs.end()-1))
                	brs.erase(brs.end()-1);
                if(dlog) {
                	cout << "( removed\n";
                	printvec(brs);
                }
                break;
            case bracket_t::R_CURLY_BR:
                if(bracket_t::L_CURLY_BR == *(brs.end()-1))
                	brs.erase(brs.end()-1);
                if(dlog) {
                	cout << "{ removed\n";
                	printvec(brs);
                }
                break;
            default:
                //cout << "ERROR\n";
                break;
        }

    }
    if(0 == brs.size())
    	return "YES";
    else
    	return "NO";
}


int main()
{
    ofstream fout("output.txt");

    string t_temp;
    getline(cin, t_temp);

    int t = stoi(ltrim(rtrim(t_temp)));
    cout << "string count(" << t << ")\n";

    for (int t_itr = 0; t_itr < t; t_itr++) {
        string s;
        getline(cin, s);

        cout << "input[" << t_itr << "]:(" << s << ")\n";
        string result = isBalanced(s, (15 == t_itr) ? true : false);

        fout << result << "\n";
        cout  << result << "\n";
    }

    fout.close();

    return 0;
}

string ltrim(const string &str) {
    string s(str);

    s.erase(
        s.begin(),
        find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace)))
    );

    return s;
}

string rtrim(const string &str) {
    string s(str);

    s.erase(
        find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(),
        s.end()
    );

    return s;
}

