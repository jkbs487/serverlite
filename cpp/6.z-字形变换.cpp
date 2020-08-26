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
        vector<string> arr(numRows);
        int len = s.size();
        for(int i = 0; i < len; ++i) {
            int div = i / (numRows - 1);
            int rem = i % (numRows - 1);
            (div & 1 ? arr.at(numRows - rem - 1) : arr.at(rem)) += s.at(i);
        }
        for(const auto a : arr) ans += a;
        return ans;
    }
};
// @lc code=end

