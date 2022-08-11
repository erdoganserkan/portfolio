/*
 * solution.cpp
 *
 *  Created on: Aug 11, 2022
 *      Author: serkan
 */

#include <bits/stdc++.h>

using namespace std;

string ltrim(const string &);
string rtrim(const string &);
vector<string> split(const string &);

void printvec(const vector<int>& AA, int line) {
    cout << "[" << line << "]:""less-sweeties: (";
    for(const auto& ix: AA)
        cout << ix << ",";
    cout << '\n';
}

/*
 * Complete the 'cookies' function below.
 *
 * The function is expected to return an INTEGER.
 * The function accepts following parameters:
 *  1. INTEGER k
 *  2. INTEGER_ARRAY A
 */

int cookies(int k, vector<int> A) {
    int iteration_cnt{};

    vector<int> AA;
    AA.reserve(A.size());

    std::priority_queue<int, std::vector<int>, std::greater<int>> q2;
    for(auto ix:A)
        if(ix<k)
            q2.push(ix);
    if((1 == q2.size()) && (1 < A.size()))
        q2.push(k);

    auto lm = [&](int x, int y){
        iteration_cnt++;
        long long int res = x + (2ULL*y);
        cout << "lm(" << x << "," << y <<")=" << res << '\n';
        return res;
    };

    long long int res;
    for( ; !q2.empty() ; ) {
        int small = q2.top();
        q2.pop();
        int big;
        if(!q2.empty()) {
            big = q2.top();
            q2.pop();
        } else {
            if(k > res) {
                iteration_cnt = -1;
                break;
            }
            else
                big = res;
        }
        res = lm(small, big);
        if(res < k)
            q2.push(res);
        //cout << "size:" << q2.size() << '\n';
    }

    cout << "res(" << iteration_cnt << ")\n";
    return iteration_cnt;
}

int main()
{
    ofstream fout(getenv("OUTPUT_PATH"));

    string first_multiple_input_temp;
    getline(cin, first_multiple_input_temp);

    vector<string> first_multiple_input = split(rtrim(first_multiple_input_temp));

    int n = stoi(first_multiple_input[0]);

    int k = stoi(first_multiple_input[1]);

    string A_temp_temp;
    getline(cin, A_temp_temp);

    vector<string> A_temp = split(rtrim(A_temp_temp));

    vector<int> A(n);

    for (int i = 0; i < n; i++) {
        int A_item = stoi(A_temp[i]);

        A[i] = A_item;
    }

    int result = cookies(k, A);

    fout << result << "\n";

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

vector<string> split(const string &str) {
    vector<string> tokens;

    string::size_type start = 0;
    string::size_type end = 0;

    while ((end = str.find(" ", start)) != string::npos) {
        tokens.push_back(str.substr(start, end - start));

        start = end + 1;
    }

    tokens.push_back(str.substr(start));

    return tokens;
}



