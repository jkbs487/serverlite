/*
 * @lc app=leetcode.cn id=14 lang=cpp
 *
 * [14] 最长公共前缀
 */

// @lc code=start
class Solution {
public:
    string longestCommonPrefix(vector<string>& strs) {
        string ans;
        if(strs.empty()) return ans;
        int len = INT_MAX;
        auto min = [](int a, int b) { return a < b ? a : b; };
        for(string str : strs)
            len = min(len, str.size());
        for(int i = 0; i < len; i++) {
            for(int j = 0; j < strs.size(); j++) {
                if(j == 0) ans.push_back(strs[j][i]);
                if(strs[j][i] != ans[i]) {
                    ans.pop_back();
                    break;
                }
            }        
        }
        return ans;
    }
};
// @lc code=end

