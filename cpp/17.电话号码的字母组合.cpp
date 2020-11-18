/*
 * @lc app=leetcode.cn id=17 lang=cpp
 *
 * [17] 电话号码的字母组合
 */

// @lc code=start
class Solution {
public:
    map<char, string> dict = {{'2', "abc"}, {'3', "def"}, {'4', "ghi"}, {'5', "jkl"}, {'6', "mno"}, {'7', "pqrs"}, {'8', "tuv"}, {'9', "wxyz"}};
    vector<string> letterCombinations(string digits) {
        vector<string> ans;
        int len = digits.size();
        if(len == 0) return ans;
        dfs(ans, digits, "", len, 0);
        return ans;
    }
    void dfs(vector<string> &ans, string digits, string temp, int len, int index) {
        if(index == len) {
            ans.push_back(temp);
            return;
        }
        for(int i = 0; i < dict[digits[index]].size(); i++) {
            temp.push_back(dict[digits[index]][i]);
            dfs(ans, digits, temp, len, index+1);
            temp.pop_back();
        }
        return;
    }
};
// @lc code=end

