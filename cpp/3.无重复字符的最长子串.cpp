/*
 * @lc app=leetcode.cn id=3 lang=cpp
 *
 * [3] 无重复字符的最长子串
 * 关键字：字符串，滑动窗口
 */

// @lc code=start
class Solution {
public:
    int lengthOfLongestSubstring(string s) {
        int ans = 0;
        int left = 0, right = 0;
        unordered_set<char> pool;
        if(s.empty()) return 0;
        while(right < s.size()){
            if(pool.find(s[right]) == pool.end()){
                pool.insert(s[right++]);
                ans = max(right-left, ans);
            }
            else{
                pool.erase(s[left]);
                left++;
            }
        }
        return ans;
    }
};
// @lc code=end

