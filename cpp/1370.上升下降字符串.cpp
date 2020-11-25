/*
 * @lc app=leetcode.cn id=1370 lang=cpp
 *
 * [1370] 上升下降字符串
 * 桶排序思想
 */

// @lc code=start
class Solution {
public:
    string sortString(string s) {
        string ans;
        map<char, int> pool;
        for(char c : s) pool[c]++;
        while(ans.size() < s.size()) {
            for(auto it = pool.begin(); it != pool.end(); it++) {
                if(it->second > 0) ans += it->first;
                it->second--;
            }
            for(auto it = pool.rbegin(); it != pool.rend(); it++) {
                if(it->second > 0) ans += it->first;
                it->second--;
            }
        }
        return ans;
    }
};
// @lc code=end

