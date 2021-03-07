// Source : https://leetcode-cn.com/problems/palindrome-partitioning/
// Author : jkbs487
// Date   : 2021-03-07

/***************************************************************************************************** 
 *
 * Given a string s, partition s such that every substring of the partition is a palindrome. Return 
 * all possible palindrome partitioning of s.
 * 
 * A palindrome string is a string that reads the same backward as forward.
 * 
 * Example 1:
 * Input: s = "aab"
 * Output: [["a","a","b"],["aa","b"]]
 * Example 2:
 * Input: s = "a"
 * Output: [["a"]]
 * 
 * Constraints:
 * 
 * 	1 <= s.length <= 16
 * 	s contains only lowercase English letters.
 ******************************************************************************************************/

//backtrack + map
class Solution {
private:
    vector<vector<string>> ans;
    vector<string> cur;
    map<string, bool> vis;
public:
    vector<vector<string>> partition(string s) {
        backtrack(s, 0);
        return ans;
    }

    bool isPalindrome(string s) {
        if (vis.count(s) > 0) return vis[s];
        int first = 0, last = s.size() - 1;
        while (first < last) {
            if(s[first++] != s[last--]) {
                vis[s] = false;
                return false;
            } 
        }
        vis[s] = true;
        return true;
    }

    void backtrack(string s, int index) {
        int len = s.size();
        if (index == len) {
            ans.push_back(cur);
            return;
        }
        for (int i = 1; i <= len-index; i++) {
            string temp = s.substr(index, i);
            if(isPalindrome(temp)) {
                cur.push_back(temp);
                backtrack(s, index+i);
                cur.pop_back();
            }
        }
        return;        
    }
};

//backtrace + dynamic programming pre-treatment
class Solution {
private:
    vector<vector<string>> ans;
    vector<string> cur;
    vector<vector<int>> isPalindrome;
public:
    vector<vector<string>> partition(string s) {
        int len = s.size();
        isPalindrome.assign(len, vector<int>(len, 1));
        for (int i = len - 1; i >= 0; --i) {
            for (int j = i + 1; j < len; ++j) {
                isPalindrome[i][j] = (s[i] == s[j]) && isPalindrome[i + 1][j - 1];
            }
        }
        backtrack(s, 0);
        return ans;
    }
   
    void backtrack(string s, int index) {
        int len = s.size();
        if (index == len) {
            ans.push_back(cur);
            return;
        }
        for (int i = index; i < len; i++) {
            string temp = s.substr(index, i-index+1);
            if(isPalindrome[index][i]) {
                cur.push_back(temp);
                backtrack(s, i+1);
                cur.pop_back();
            }
        }
        return;        
    }
};
