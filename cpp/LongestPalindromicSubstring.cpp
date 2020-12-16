// Source : https://leetcode-cn.com/problems/longest-palindromic-substring/
// Author : jkbs487
// Date   : 2020-12-15

/***************************************************************************************************** 
 *
 * Given a string s, return the longest palindromic substring in s.
 * 
 * Example 1:
 * 
 * Input: s = "babad"
 * Output: "bab"
 * Note: "aba" is also a valid answer.
 * 
 * Example 2:
 * 
 * Input: s = "cbbd"
 * Output: "bb"
 * 
 * Example 3:
 * 
 * Input: s = "a"
 * Output: "a"
 * 
 * Example 4:
 * 
 * Input: s = "ac"
 * Output: "a"
 * 
 * Constraints:
 * 
 * 	1 <= s.length <= 1000
 * 	s consist of only digits and English letters (lower-case and/or upper-case),
 ******************************************************************************************************/

class Solution {
public:
    string longestPalindrome(string s) {
        int len = s.size();
        string ans;
        vector<vector<int>> dp(len, vector<int>(len));
        for (int l = 0; l < len; ++l) {
            for (int i = 0; i + l < n; ++i) {
                int j = i + l;
                if (l == 0) 
                    dp[i][j] = 1;
                else if (l == 1)
                    dp[i][j] = (s[i] == s[j]);
                else 
                    dp[i][j] = (s[i] == s[j] && dp[i + 1][j - 1]);
                if (dp[i][j] && l + 1 > ans.size()) 
                    ans = s.substr(i, l + 1);
            }
        }
        return ans;
    }
};
