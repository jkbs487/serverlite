/*
 * @lc app=leetcode.cn id=5 lang=cpp
 *
 * [5] 最长回文子串
 */

// @lc code=start
class Solution {
public:
    string longestPalindrome(string s) {
        int len = s.size();
        vector<vector<int>> dp(len, vector<int>(len));
        for(int i = 0; i < len; i++) {
            for(int j = 0; j < len; j++) {
                if(i == j) dp[i][j] = 1;
                if(i+1 == j && s[i+1] == s[j])
                    dp[i+1][j] = 1;
                
            }
        }     
    }
};
// @lc code=end

