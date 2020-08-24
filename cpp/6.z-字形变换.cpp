/*
 * @lc app=leetcode.cn id=6 lang=cpp
 *
 * [6] Z 字形变换
 */

// @lc code=start
class Solution {
public:
    string convert(string s, int numRows) {
        string ans;
        if(numRows <= 1) return s.empty() ? ans : s;
        vector<string> vec(numRows);
        for(int i = 0; i < s.size(); ++i)
        {
            int quo = i / (numRows - 1);
            int rem = i % (numRows - 1);
            (quo & 1 ? vec.at(numRows - rem - 1) : vec.at(rem)) += s.at(i);
        }
        for(const auto x : vec) ans += x;
        return ans;
    }
};
// @lc code=end

