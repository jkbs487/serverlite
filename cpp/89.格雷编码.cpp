/*
 * @lc app=leetcode.cn id=89 lang=cpp
 *
 * [89] 格雷编码
 */

// @lc code=start
class Solution {
public:
    vector<int> grayCode(int n) {
        vector<int> res;
        for(int i = 0; i < 1<<n; i++){
            res.push_back(i ^ i>>1);
        }
        return res;
    }
};
// @lc code=end

